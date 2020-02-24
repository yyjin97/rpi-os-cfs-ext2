#include "fair.h"
#include "kernel.h"
#include "rbtree.h"

#define WMULT_CONST		(~0U)
#define WMULT_SHIFT 	32

/* 최소 schedule 시간으로 latency와 time slice값은 다름 
	(weight값을 기반으로 latency를 쪼개 task마다 time slice를 할당) 
	( default : 6ms , units: nanoseconds */
//unsigned int sysctl_sched_latency			= 6000000ULL;
unsigned int sysctl_sched_latency			= 64ULL;

/* task당 scheduling 최소 단위 
 	( default : 0.75ms , units: nanoseconds) */
//unsigned int sysctl_sched_min_granularity	= 750000ULL;
unsigned int sysctl_sched_min_granularity = 8ULL;

/* This value is kept at sysctl_sched_latency/sysctl_sched_min_granularity */
//unsigned int sched_nr_latency = 8;
unsigned int sched_nr_latency = 8;

/* After fork, child runs first. If set to 0 (default) then
	parent will (try to) run first. */
unsigned int sysctl_sched_child_runs_first = 1;

/* load weight 값을 inc만큼 추가하고 inv_weight 값은 0으로 reset */
static inline void update_load_add(struct load_weight *lw, unsigned long inc) 
{
	lw->weight += inc;
	lw->inv_weight = 0;
}

/* load weight 값을 dec만큼 감소시키고 inv_weight 값은 0으로 reset */
static inline void update_load_sub(struct load_weight *lw, unsigned long dec)
{
	lw->weight -= dec;
	lw->inv_weight = 0;
}


/* lw->inv_weight값을 0xffff_ffff/lw->weight 값으로 변경 
	( 'delta_exec/weight = '와 같이 나눗셈이 필요할 때 나눗셈 대신 'delta_exec*wmult>>32'를 사용 
		wmult값을 만들기 위해 '2^32 / weight' 값으로 변경해줌 ) */
void __update_inv_weight(struct load_weight *lw) 
{
	unsigned long w;

	if(lw->inv_weight)
		return;

	w = lw->weight;

	if(w >= WMULT_CONST)
		lw->inv_weight = 1;
	else if(!w)
		lw->inv_weight = WMULT_CONST;
	else
		lw->inv_weight = WMULT_CONST / w;
}

/* __calc_delta(x, w, wt)라고 할 때 'x*(w/wt) = '에 대한 계산을 성능향상을 위해 
	'x*w*(2^32/wt)>>32 = '로 변경하여 계산해줌 (커널이 나눗셈 연산이 느린것을 감안해 곱하기와 shift연산으로 변환) */
u64 __calc_delta(u64 delta_exec, unsigned long weight, struct load_weight *lw)
{
	u64 fact = scale_load_down(weight);
	int shift = WMULT_SHIFT;

	__update_inv_weight(lw);

	fact = (u64) (u32)fact * lw->inv_weight;

	/*fact가 32비트 이상으로 커졌다면 32비트 이하가 될 때까지 
		fact를 2로 나누고 나눈 횟수만큼 shift를 빼줌 */
	while(fact >> 32) {			
		fact >>= 1;
		shift--;
	}

	return (delta_exec*fact) >> shift;
}

struct task_struct *task_of(struct sched_entity *se)
{
	return container_of(se, struct task_struct, se);
}

struct cfs_rq *task_cfs_rq(struct task_struct *p)
{
	return p->se.cfs_rq;
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

/* entity_before(a, b)에서 a의 vruntime이 b보다 작은경우 true를 리턴 */
static inline int entity_before(struct sched_entity *a, struct sched_entity *b)
{
	return (s64)(a->vruntime - b->vruntime) < 0;
}

/* 현재 실행중인 task와 RB트리의 가장 왼쪽노드 task 중 vruntime이 더 작은 값으로 cfs_rq의 min_vruntime을 update */
void update_min_vruntime(struct cfs_rq *cfs_rq)
{
	struct sched_entity *curr = cfs_rq->curr;
	struct rb_node *leftmost = rb_first_cached(&cfs_rq->tasks_timeline);
	u64 vruntime = cfs_rq->min_vruntime;

	if(curr) {
		/* current entity인 경우 on_rq가 1이므로 이 경우에만 current의 vruntime을 가져옴 */
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

/* 프로세스의 정보를 rbtree에 넣고 tree를 조정하는 작업의 실제 처리를 담당 */
void __enqueue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	struct rb_node **link = &cfs_rq->tasks_timeline.rb_root.rb_node;
	struct rb_node *parent = NULL;
	struct sched_entity *entry;
	int leftmost = 1;

	/* rbtree에서 추가할 위치를 찾음 */ 
	while(*link) {
		parent = *link;
		entry = rb_entry(parent, struct sched_entity, run_node);

		/* 충돌발생을 고려하지 않음 같은 key값을 갖는 경우 같이 둠 */ 
		if(entity_before(se, entry)){
			link = &parent->rb_left;
		} else {
			link = &parent->rb_right;
			leftmost = 0;
		}
	}

	//if(leftmost) 
	//	cfs_rq->tasks_timeline.rb_leftmost = &se->run_node;

	rb_link_node(&se->run_node, parent, link);
	rb_insert_color_cached(&se->run_node, &cfs_rq->tasks_timeline, leftmost); 

} 

/* rbtree에서 프로세스를 제거하는 실제 처리를 담당 */
void __dequeue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	rb_erase_cached(&se->run_node, &cfs_rq->tasks_timeline);
}

/* rbtree의 leftmost node를 리턴 */ 
struct sched_entity *__pick_first_entity(struct cfs_rq *cfs_rq)
{
	struct rb_node *left = rb_first_cached(&cfs_rq->tasks_timeline);

	if(!left)
		return NULL;

	return rb_entry(left, struct sched_entity, run_node);
}

/* vruntime = execution time * (weight-0 / weight) 로 계산 
	( se가 weight-0일 경우 time slice값을 바로 리턴해줄 수 있음 ) */
static inline u64 calc_delta_fair(u64 delta, struct sched_entity *se)
{
	if(se->load.weight != NICE_0_LOAD)
		delta = __calc_delta(delta, NICE_0_LOAD, &se->load);

	return delta;
}

/* 실행중인 task의 개수를 기준으로 period 값을 구함 
	period = (nr_running <= sched_nr_latency)? 
		sched_latency : sched_mingranularity * nr_running  */
u64 __sched_period(unsigned long nr_running) 
{
	u64 period = sysctl_sched_latency;
	unsigned long nr_latency = sched_nr_latency;

	if(nr_running > nr_latency) {
		period = sysctl_sched_min_granularity;
		period *= nr_running;
	}

	return period;
}

/* se sched_entity의 time slice를 산출하여 return 
	( time slice = periods * (weight/tot_weight) 으로 계산하는데 
	이 때, x/tot_weight는 성능향상을 목적으로 x*(2^32/tot_weight)>>32 로 계산 )*/
u64 sched_slice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	u64 slice = __sched_period(cfs_rq->nr_running + !se->on_rq);
	
	//아직 task group을 사용하지 않을 것이므로 for_each_sched_entity를 통해 루프를 돌면서 실행하지 않음
	struct load_weight *load;
	struct load_weight lw;

	/* linux에서는 cfs_rq_of 함수를 이용해 task가 실행 중인 cpu의 rq->cfs_rq를 구해야하지만
		현재 rpi os version에서는 하나의 cpu만 이용하므로 cfs_rq도 하나만 존재하게됨 */
	//******cfs_rq구조체가 cpu마다 존재할 경우 cpu에 맞는 cfs_rq구조체를 가져와야함!!!

	load = &cfs_rq->load;

	/* se의 on_rq가 0인 경우 run queue에 존재하지 않고 current entity가 아닌경우이므로
		전체 시간 및 load 계산 시 이를 다시 추가해서 계산해 주어야 함 */
	if(!se->on_rq) {
		lw = cfs_rq->load;

		update_load_add(&lw, se->load.weight);
		load = &lw;
	}
	slice = __calc_delta(slice, se->load.weight, load);
	
	return slice;
}

/* sched_entity의 load에 해당하는 time slice 값을 산출하고 이를 통해 vruntime 값을 계산 */
u64 sched_vslice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	return calc_delta_fair(sched_slice(cfs_rq, se), se);
}

/* task의 실행시간을 기반으로 vruntime값을 update하고 cfs_rq의 min_vruntime을 update */
void update_curr(struct cfs_rq *cfs_rq)
{
	struct sched_entity *curr = cfs_rq->curr;
	u64 now = cfs_rq->clock_task;
	u64 delta_exec;

	if(!curr)
		return;

	delta_exec = now - curr->exec_start;
	if((s64)delta_exec <= 0)
		return;

	curr->exec_start = now;

	curr->sum_exec_runtime += delta_exec;
	
	curr->vruntime += calc_delta_fair(delta_exec, curr);
	update_min_vruntime(cfs_rq);

	//account_cfs_rq_runtime(); //bandwidth 사용시에만 필요 ????
}

/* 새로 실행하는 task의 exec_start시간 update */
static inline void update_stats_curr_start(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	se->exec_start = cfs_rq->clock_task;
}

/* cfs_rq에 enqueue될 entity의 vruntime을 결정 
	(기본적으로는 cfs_rq의 min_vruntime을 기준으로 결정) */
void place_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int initial)
{
	u64 vruntime = cfs_rq->min_vruntime;

	if(initial) 
		vruntime += sched_vslice(cfs_rq, se);
	else {
		unsigned long thresh = sysctl_sched_latency;

		thresh >>= 1;			//linux kernel에서는 GENTLE_FAIR_SLEEPERS를 지원하는 경우에만 실행

		vruntime -= thresh;
	}

	se->vruntime = max_vruntime(se->vruntime, vruntime);

}

/* rbtree에 프로세스를 추가 */
void enqueue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se) 
{
	update_curr(cfs_rq);
	account_entity_enqueue(cfs_rq, se);

	if(se != cfs_rq->curr)
		__enqueue_entity(cfs_rq, se);
	se->on_rq = 1;
}

/* rbtree에서 프로세스를 추가 */
void dequeue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se) 
{
	update_curr(cfs_rq);

	if(se != cfs_rq->curr)
		__dequeue_entity(cfs_rq, se);
	se->on_rq = 0;

	account_entity_dequeue(cfs_rq, se);

	update_min_vruntime(cfs_rq);
}

/* 현재 task가 선점이 필요한지 확인함 다음 2가지 경우에 선점이 일어남
	1. 현재 task의 time slice를 모두 소모한 경우 (이 경우 vruntime이 아닌 일반적인 시간을 사용)
	2. 더 작은 vruntime을 가지는 task가 있고 그 차이가 임계값보다 큰 경우 */
void check_preempt_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr) 
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

	se = __pick_first_entity(cfs_rq); 
	delta = curr->vruntime - se->vruntime;

	if(delta < 0)		//현재 task의 vruntime이 가장 작은 경우 
		return;

	if(delta > ideal_runtime)		//현재 task보다 더 작은 vruntime이 존재하고 그 차이가 임계값보다 큰 경우 
		resched_curr(curr);
}

/* 인자로 전달한 entity를 cfs_rq의 current entity로 설정
	current entity가 되었으므로 rbtree에서 dequeue시키고 exec_time정보도 갱신함 */
void set_next_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	/* entity가 rbtree에 enqueue되어 있는 entity인지 체크하고 enqueue되어있으면 dequeue함 
		__dequeue_entity()는 dequeue_entity()와 달리 on_rq필드를 초기화하지 않음 */
	if(se->on_rq) {
		//statistic update
		__dequeue_entity(cfs_rq, se); 
	}

	update_stats_curr_start(cfs_rq, se);
	cfs_rq->curr = se;

	se->prev_sum_exec_runtime = se->sum_exec_runtime;
}

/* rbtree의 leftmost node를 다음에 실행할 entity로 선택 */
struct sched_entity * pick_next_entity(struct cfs_rq *cfs_rq, struct sched_entity *curr)
{
	struct sched_entity *left = __pick_first_entity(cfs_rq);

	if(!left || (curr && entity_before(curr, left))) 
		left = curr;
	
	return left;
}

/* 기존에 실행하던 task를 다시 rbtree에 넣음 */
void put_prev_entity(struct cfs_rq *cfs_rq, struct sched_entity *prev)
{
	/* current entity가 cfs_rq에서 dequeue되지 않은 entity라면 rbtree에 enqueue함
		entity가 current entity가 될 때 on_rq필드는 1로 유지된 채 트리에서 빠지게 됨 
		다시 실행되기 위해서는 entity를 rbtree에 다시 enqueue해야 함 */
	if(prev->on_rq) {
		update_curr(cfs_rq);

		__enqueue_entity(cfs_rq, prev);
	}
	cfs_rq->curr = NULL;
}

void entity_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr)
{
	struct task_struct *p = task_of(curr);

	/* Update run-time statistics of the 'current' */
	update_curr(cfs_rq);

	/* 현재 고정 스케줄러 틱만 사용한다고 가정하여 hrtick을 사용하는 경우 pass */

	if(cfs_rq->nr_running > 1 && p->thread_info.preempt_count <= 0)
		check_preempt_tick(cfs_rq, curr);
}

/* CFS 스케줄러 틱 호출시마다 수행해야 할 일을 처리 */
void task_tick_fair(struct task_struct *curr) 
{
	struct cfs_rq *cfs_rq;
	struct sched_entity *se = &curr->se;

	cfs_rq = se->cfs_rq;
	entity_tick(cfs_rq, se);
}

/* 새로 생성된 task의 스케줄링 정보 초기화 및 vruntime 조정 */
void task_fork_fair(struct task_struct *p)
{
	struct cfs_rq *cfs_rq;
	struct sched_entity *se = &p->se, *curr;

	cfs_rq = task_cfs_rq(current);

	update_rq_clock(cfs_rq);

	curr = cfs_rq->curr;
	if(curr) {
		update_curr(cfs_rq);
		se->vruntime = curr->vruntime;
	}
	place_entity(cfs_rq, se, 1);

	/* 새로 생성한 task를 enqueue
		(linux kernel에서는 task_fork_fair함수에 이부분 존재하지 않음) */
	enqueue_entity(cfs_rq, se);

	if(sysctl_sched_child_runs_first && curr && entity_before(curr, se)) {
		swap(curr->vruntime, se->vruntime);
		resched_curr(curr);
	}

	//se->vruntime -= cfs_rq->min_vruntime;
}

/* 새로 추가하는 entity의 load값을 cfs_rq의 load에 추가 */
void account_entity_enqueue(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	update_load_add(&cfs_rq->load, se->load.weight);

	cfs_rq->nr_running++;
}

void account_entity_dequeue(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	update_load_sub(&cfs_rq->load, se->load.weight);

	cfs_rq->nr_running--;
}

struct task_struct * pick_next_task_fair(struct cfs_rq *cfs_rq, struct task_struct *prev)
{
	struct sched_entity *se;
	struct task_struct *p;

	if(!cfs_rq->nr_running)
		return NULL;

	put_prev_entity(cfs_rq, &prev->se);

	se = pick_next_entity(cfs_rq, NULL);
	set_next_entity(cfs_rq, se);

	p = task_of(se);

	return p;
}

