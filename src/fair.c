#include "sched.h"
#include "rbtree.h"

#define WMULT_CONST		(~0U)
#define WMULT_SHIFT 	32

/* 최소 schedule 시간으로 latency와 time slice값은 다름 
	(weight값을 기반으로 latency를 쪼개 task마다 time slice를 할당) 
	( default : 6ms , units: nanoseconds */
unsigned int sysctl_sched_latency			= 6000000ULL;

/* task당 scheduling 최소 단위 
 	( default : 0.75ms , units: nanoseconds) */
unsigned int sysctl_sched_min_granularity	= 750000ULL;

/* This value is kept at sysctl_sched_latency/sysctl_sched_min_granularity */
static unsigned int sched_nr_latency = 8;

/* load weight 값을 inc만큼 추가하고 inv_weight 값은 0으로 reset */
static inline void update_load_add(struct load_weight *lw, unsigned long inc) 
{
	lw->weight += inc;
	lw->inv_weight = 0;
}

/* se sched_entity의 time slice를 산출하여 return 
	( time slice = periods * (weight/tot_weight) 으로 계산하는데 
	이 때, x/tot_weight는 성능향상을 목적으로 x*(2^32/tot_weight)>>32 로 계산 )*/
static u64 sched_slice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	u64 slice = __sched_period(cfs_rq->nr_running + !se->on_rq);
	
	//아직 task group을 사용하지 않을 것이므로 for_each_sched_entity를 통해 루프를 돌면서 실행하지 않음
	struct load_weight *load;
	struct load_weight lw;

	/* linux에서는 cfs_rq_of 함수를 이용해 task가 실행 중인 cpu의 rq->cfs_rq를 구해야하지만
		현재 rpi os version에서는 하나의 cpu만 이용하므로 cfs_rq도 하나만 존재하게됨 */
	//******cfs_rq구조체가 task마다 존재할 경우 task에 맞는 cfs_rq구조체를 가져와야함!!!

	load = &cfs_rq->load;

	/* 현재 프로세스가 cpu에서 실행중인 경우 se->on_rq가 0이 됨
		CFS는 실행 중인 프로세스를 run queue에서 제거하므로 전체 시간 및 load 계산 시 
		이를 다시 추가해서 계산해 주어야 함 */
	if(unlikely(!se->on_rq)) {
		lw = cfs_rq->load;

		update_load_add(&lw, se->load.weight);
		load = &lw;
	}
	slice = __calc_delta(slice, se->load.weight, load);
	
	return slice;
}

/* 실행중인 task값을 기준으로 period 값을 구함 
	period = (nr_running <= sched_nr_latency)? 
		sched_latency : sched_mingranularity * nr_running  */
static u64 __sched_period(unsigned long nr_running) 
{
	u64 period = sysctl_sched_latency;
	unsigned long nr_latency = sched_nr_latency;

	if(unlikely(nr_running > nr_latency)) {
		period = sysctl_sched_min_granularity;
		period *= nr_running;
	}

	return period;
}

/* __calc_delta(x, w, wt)라고 할 때 'x*(w/wt) = '에 대한 계산을 성능향상을 위해 
	'x*w*(2^32/wt)>>32 = '로 변경하여 계산해줌 */
static u64 __calc_delta(u64 delta_exec, unsigned long weight, struct load_weight *lw)
{
	//u64 fact = scale_load_down(weight); //64bit 아키텍처에서 weight에 대한 해상도를 조절하는 부분 구현 x
	u64 fact = weight;
	int shift = WMULT_SHIFT;

	__update_inv_weight(lw);

	fact = (u64) (u32)fact * lw->inv_weight;

	return (delta_exec*fact) >> shift;
}

/* lw->inv_weight값을 0xffff_ffff/lw->weight 값으로 변경 
	( 'delta_exec/weight = '와 같이 나눗셈이 필요할 때 나눗셈 대신 'delta_exec*wmult>>32'를 사용 
		wmult값을 만들기 위해 '2^32 / weight' 값으로 변경해줌 ) */
static void __update_inv_weight(struct load_weight *lw) 
{
	unsigned long w;

	if(likely(lw->inv_weight))
		return;

	w = lw->weight;

	if(unlikely(w >= WMULT_CONST))
		lw->inv_weight = 1;
	else if(unlikely(!w)) 
		lw->inv_weight = WMULT_CONST;
	else
		lw->inv_weight = WMULT_CONST / w;
}

/* vruntime = execution time * (weight-0 / weight) 로 계산 
	( se가 weight-0일 경우 time slice값을 바로 리턴해줄 수 있음 ) */
static inline u64 calc_delta_fair(u64 delta, struct sched_entity *se)
{
	if(unlikely(se->load.weight != NICE_0_LOAD))
		delta = __calc_delta(delta, NICE_0_LOAD, &se->load);

	return delta;
}

/* sched_entity의 load에 해당하는 time slice 값을 산출하고 이를 통해 vruntime 값을 계산 */
static u64 sched_vslice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	return calc_delta_fair(sched_slice(cfs_rq, se), se);
}

/* max_vruntime과 vruntime 중 더 큰 값을 리턴 */
static inline u64 max_vruntime(u64 max_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - max_vruntime);
	if(delta > 0) 
		max_vruntime = vruntime;
	
	return max_vruntime;
}

/* min_vruntime과 vruntime 중 더 작은 값을 리턴 */
static inline u64 min_vruntime(u64 min_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - min_vruntime);
	if(delta < 0)
		min_vruntime = vruntime;

	return min_vruntime;
}

/* 현재 실행중인 task와 RB트리의 가장 왼쪽노드 task 중 vruntime이 더 작은 값으로 cfs_rq의 min_vruntime을 update */
static void update_min_vruntime(struct cfs_rq *cfs_rq)
{
	struct sched_entity *curr = cfs_rq->curr;
	struct rb_node *leftmost = rb_first_cached(&cfs_rq->tasks_timeline);

	u64 vruntime = cfs_rq->min_vruntime;

	if(curr) {
		/* on_rq가 0인 경우 curr가 CPU에서 실행 중인 경우를 의미 */
		if(curr->on_rq)		
			vruntime = curr->vruntime;
		else 	
			curr = NULL;
	}
	if(leftmost) {
		struct sched_entity *se;
		se = rb_entry(leftmost, struct sched_entity, run_node);

		if(!curr) 
			vruntime = se->vruntime;
		else 
			vruntime = min_vruntime(vruntime, se->vruntime); 		//RB트리의 가장 왼쪽 노드 task와 현재 런큐에 있는 task 중 vruntime 값이 작은 것 선택
	}

	cfs_rq->min_vruntime = max_vruntime(cfs_rq->min_vruntime, vruntime);
	/* 여기서 더 큰 값을 고르는 이유는 작은 값을 고른다면 언제나 min_vruntime이 동일한 값으로 유지되거나 감소하게 될 것이기 때문 
		다른 task들의 vruntime은 지속적으로 증가하므로 여기에 맞추어 min_vruntime도 증가해야함 
		(min_vruntime은 새로 fork된 task에 할당되어 다음에 선택될 것을 보장하며 새로운 task가 선점되기까지 비합리적으로 오랜 시간동안 실행되지 않도록 함) */

}

/* task의 실행시간을 기반으로 vruntime값을 update하고 cfs_rq의 min_vruntime을 update */
static void update_curr(struct cfs_rq *cfs_rq)
{
	struct sched_entity *curr = cfs_rq->curr;
	u64 now = cfs_rq->clock_task;
	u64 delta_exec;

	if(unlikely(!curr))
		return;

	curr->exec_start = now;

	curr->sum_exec_runtime += delta_exec;
	
	curr->vruntime += calc_delta_fair(delta_exec, curr);
	update_min_vruntime(cfs_rq);

	//account_cfs_rq_runtime(); //bandwidth 사용시에만 필요 ????
}

/* 현재 task에 resched 요청 플래그를 설정함 
	(linux kernel에서는 core.c에 존재) */
void resched_curr(struct sched_entity *se)
{
	struct task_struct *curr = container_of(se, struct task_struct, se);

	if(test_tsk_need_resched(curr))
		return;

	/* curr의 런큐가 현재 사용중인 cpu인지 확인하고 polling signal?을 보내는 부분 제외
		(현재 RPI OS version에서는 0번 core만 사용하기 때문) */

	set_tsk_need_resched(curr);
	
	return;
}

/* 현재 task가 선점이 필요한지 확인함 다음 2가지 경우에 선점이 일어남
	1. 현재 task의 time slice를 모두 소모한 경우 (이 경우 vruntime이 아닌 일반적인 시간을 사용)
	2. 더 작은 vruntime을 가지는 task가 있고 그 차이가 임계값보다 큰 경우 */
static void check_preempt_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr) 
{
	unsigned long ideal_runtime, delta_exec;
	struct sched_entity *se;
	u64 delta;

	ideal_runtime = sched_slice(cfs_rq, curr);
	delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
	if(delta_exec > ideal_runtime) {			//time slice를 모두 소진한 경우 
		resched_curr(curr);
		//clear_buddies()
		return;
	}

	if(delta_exec < sysctl_sched_min_granularity)
		return;

	se = __pick_first_entry(cfs_rq);
	delta = curr->vruntime - se->vruntime;

	if(delta < 0)		//현재 task의 vruntime이 가장 작은 경우 
		return;

	if(delta > ideal_runtime)		//현재 task보다 더 작은 vruntime이 존재하고 그 차이가 임계값보다 큰 경우 
		resched_curr(curr);
}

static void entity_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr)
{
	/* Update run-time statistics of the 'current' */
	update_curr(cfs_rq);

	/* 현재 고정 스케줄러 틱만 사용한다고 가정하여 hrtick을 사용하는 경우 pass */

	if(cfs_rq->nr_running > 1)
		check_preempt_tick(cfs_rq, curr);
}

/* CFS 스케줄러 틱 호출시마다 수행해야 할 일을 처리 */
static void task_tick_fair(struct task_struct *curr) 
{
	struct cfs_rq *cfs_rq;
	struct sched_entity *se = &curr->se;

	cfs_rq = se->cfs_rq;
	entity_tick(cfs_rq, se);
}
