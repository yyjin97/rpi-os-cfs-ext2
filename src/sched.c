#include "sched.h"
#include "fair.h"
#include "irq.h"
#include "utils.h"
#include "mm.h"
#include "timer.h"
#include "printf.h"

static struct task_struct init_task = INIT_TASK;
struct task_struct *current = &(init_task);
struct task_struct * task[NR_TASKS] = {&(init_task), };
struct cfs_rq cfs_rq;
int nr_tasks = 1;
 
bool need_resched(void) 
{
	return test_ti_thread_flag(current_thread_info(), TIF_NEED_RESCHED);
}

void update_rq_clock(struct cfs_rq *cfs_rq)
{
	s64 delta;

	delta = timer_clock() - cfs_rq->clock_task;
	if(delta < 0)
		return;
	cfs_rq->clock_task += delta;
}

/* 현재 task에 resched 요청 플래그를 설정함 */
void resched_curr(struct sched_entity *se)
{
	struct task_struct *curr = task_of(se);

	if(test_tsk_need_resched(curr))
		return;

	/* curr의 런큐가 현재 사용중인 cpu인지 확인하고 polling signal?을 보내는 부분 제외
		(현재 RPI OS version에서는 0번 core만 사용하기 때문) */

	set_tsk_need_resched(curr);
	
	return;
}

void set_load_weight(struct task_struct *p)
{
	int prio = p->priority - MAX_RT_PRIO;
	struct load_weight *load = &p->se.load;

	load->weight = scale_load(sched_prio_to_weight[prio]);
	load->inv_weight = sched_prio_to_wmult[prio];
}

void sched_init(void)
{
	cfs_rq.nr_running = 0;
	cfs_rq.tasks_timeline = RB_ROOT_CACHED;
	cfs_rq.min_vruntime = 0;
	//cfs_rq.min_vruntime = (u64)(-(1LL << 20));
	cfs_rq.clock_task = timer_clock();

	cfs_rq.load.inv_weight = 0;
	cfs_rq.load.weight = 0;
	
	set_load_weight(&init_task);
	cfs_rq.curr = &current->se;

	init_task.se.cfs_rq = &cfs_rq;
	init_task.se.exec_start = cfs_rq.clock_task;
	init_task.se.vruntime = cfs_rq.min_vruntime;

	place_entity(&cfs_rq, &current->se, 1);

	enqueue_entity(&cfs_rq, &init_task.se);
}

void preempt_disable(void)
{
	current->thread_info.preempt_count++;
}

void preempt_enable(void)
{
	current->thread_info.preempt_count--;
}


void _schedule(void)
{
	preempt_disable();
	struct task_struct * next;
	struct task_struct * prev;

	prev = current;

	next = pick_next_task_fair(&cfs_rq, prev);
	clear_tsk_need_resched(prev);

	switch_to(next);
	preempt_enable();
}

void schedule(void)
{
	if(current)
		current->se.vruntime += cfs_rq.min_vruntime;
	_schedule();
}


void switch_to(struct task_struct *next) 
{
	if (current == next) 
		return;
	struct task_struct * prev = current;
	current = next;
	set_pgd(next->mm.pgd);
	cpu_switch_to(prev, next);
}

void schedule_tail(void) {
	preempt_enable();
}


void timer_tick(void)
{
	if(current->thread_info.preempt_count > 0)
		return;

	update_rq_clock(task_cfs_rq(current));

	task_tick_fair(current);
	enable_irq();
	if(need_resched()) 
		_schedule();
	
	disable_irq();
}

void exit_process(){				
	preempt_disable();
	struct task_struct *tsk = current;
	pid_t pid = current->pid;

	dequeue_entity(&cfs_rq, &tsk->se);

	/* current task에 대한 종료작업이 이미 수행중인 경우 수행하려던 종료작업을 중지함 */
	if(tsk->flags & PF_EXITING) {
		current->state = TASK_UNINTERRUPTIBLE;
		schedule();
	}

	/*
	for(int i =0;i<tsk->mm.kernel_pages_count;i++) {
		free_page(tsk->mm.kernel_pages[i]);
	} */

	for(int i =0;i<tsk->mm.user_pages_count;i++) {
		free_page(tsk->mm.user_pages[i].phys_addr);
	} 

	tsk->state = TASK_DEAD;
	tsk->flags = PF_EXITING;
	printf("\r\nexit process : %d\n\r", pid);

	preempt_enable();
	schedule();
}


/* nice 값에 따른 weight 값을 나타내는 배열 */
const int sched_prio_to_weight[40] = {
 /* -20 */     88761,     71755,     56483,     46273,     36291,
 /* -15 */     29154,     23254,     18705,     14949,     11916,
 /* -10 */      9548,      7620,      6100,      4904,      3906,
 /*  -5 */      3121,      2501,      1991,      1586,      1277,
 /*   0 */      1024,       820,       655,       526,       423,
 /*   5 */       335,       272,       215,       172,       137,
 /*  10 */       110,        87,        70,        56,        45,
 /*  15 */        36,        29,        23,        18,        15,
};

/* nice 값에 따른 (2^32)/weight 값을 나타내는 배열 */
const u32 sched_prio_to_wmult[40] = {
 /* -20 */     48388,     59856,     76040,     92818,    118348,
 /* -15 */    147320,    184698,    229616,    287308,    360437,
 /* -10 */    449829,    563644,    704093,    875809,   1099582,
 /*  -5 */   1376151,   1717300,   2157191,   2708050,   3363326,
 /*   0 */   4194304,   5237765,   6557202,   8165337,  10153587,
 /*   5 */  12820798,  15790321,  19976592,  24970740,  31350126,
 /*  10 */  39045157,  49367440,  61356676,  76695844,  95443717,
 /*  15 */ 119304647, 148102320, 186737708, 238609294, 286331153,
};
