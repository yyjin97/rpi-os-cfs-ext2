#include "sched.h"

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

/* vruntime = time slice * (weight-0 / weight) 로 계산 
	se가 weight-0일 경우 time slice값을 바로 리턴해줄 수 있음 */
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


