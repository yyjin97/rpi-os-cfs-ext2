#ifndef _SCHED_H
#define _SCHED_H

#include "types.h"
#include "rbtree.h"
#include "thread_info.h"

#define THREAD_CPU_CONTEXT			0 		// offset of cpu_context in task_struct 

#ifndef __ASSEMBLER__

#define THREAD_SIZE				4096

#define NR_TASKS				64 

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS-1]

#define TASK_RUNNING			0
#define TASK_ZOMBIE				1

#define PF_KTHREAD				0x00000002	

#define NICE_0_LOAD_SHIFT		10
#define NICE_0_LOAD				(1L << NICE_0_LOAD_SHIFT)
#define scale_load(w)			(w)
#define scale_load_down(w)		(w) 		//64bit 아키텍처에서 weight에 대한 해상도를 조절하는 부분 구현 x

#define MAX_RT_PRIO				100

extern struct task_struct *current;
extern struct task_struct * task[NR_TASKS];
extern int nr_tasks;

#define MAX_PROCESS_PAGES			16	

struct user_page {
	unsigned long phys_addr;
	unsigned long virt_addr;
};

struct mm_struct {
	unsigned long pgd;
	int user_pages_count;
	struct user_page user_pages[MAX_PROCESS_PAGES];
	int kernel_pages_count;
	unsigned long kernel_pages[MAX_PROCESS_PAGES];
};

struct task_struct {
	struct thread_info thread_info;
	long state;	
	long priority;
	unsigned long flags;
	struct sched_entity se;
	struct mm_struct mm;
	pid_t pid;
};

struct sched_entity {
	struct load_weight		load;
	struct rb_node 			run_node;
	unsigned int 			on_rq;			//entity의 enqueue된 여부를 나타내는 필드, rbtree에 enqueue되어있거나 current entity일 경우에는 1, dequeue되어있는 경우에는 0으로 set

	u64				exec_start;
	u64				sum_exec_runtime;
	u64				vruntime;
	u64				prev_sum_exec_runtime;

	struct cfs_rq	*cfs_rq;
};

struct load_weight {
	unsigned long			weight;
	u32				inv_weight;
};

struct cfs_rq {
	struct load_weight load;
	unsigned int nr_running;

	u64 min_vruntime;

	struct rb_root_cached tasks_timeline;		//cfs 런큐의 RB 트리

	struct sched_entity *curr;

	u64 clock_task;					//linux kernel의 경우 rq구조체에 존재 
};

extern void set_load_weight(struct task_struct *p);
extern void sched_init(void);
extern void schedule(void);
extern void timer_tick(void);
extern void preempt_disable(void);
extern void preempt_enable(void);
extern void switch_to(struct task_struct* next);
extern void cpu_switch_to(struct task_struct* prev, struct task_struct* next);
extern void exit_process(void);

static inline struct thread_info *task_thread_info(struct task_struct *task)
{
	return &task->thread_info;
}

static inline int test_tsk_need_resched(struct task_struct *tsk)
{
	return test_ti_thread_flag(task_thread_info(tsk), TIF_NEED_RESCHED);
}

static inline void set_tsk_need_resched(struct task_struct *tsk)
{
	set_ti_thread_flag(task_thread_info(tsk), TIF_NEED_RESCHED);
}

static inline void clear_tsk_need_resched(struct task_struct *tsk)
{
	clear_ti_thread_flag(task_thread_info(tsk), TIF_NEED_RESCHED);
}

static bool need_resched(void) 
{
	return test_ti_thread_flag(current_thread_info(), TIF_NEED_RESCHED);
}

#define INIT_TASK \
/*thread_info*/ { { { 0,0,0,0,0,0,0,0,0,0,0,0,0}, 0, 0 }, \
/* state etc */	 0, 120, PF_KTHREAD, \
/* sched_entity */ { {0,0}, {0,0,0}, 0, 0, 0, 0, 0, 0 }, \
/* mm */ { 0, 0, {{0}}, 0, {0}}, \
/* pid */ 0 \
}
#endif
#endif
