#ifndef _SCHED_H
#define _SCHED_H

#include "types.h"
#include "rbtree.h"

#define THREAD_CPU_CONTEXT			0 		// offset of cpu_context in task_struct 

#ifndef __ASSEMBLER__

#define THREAD_SIZE				4096

#define NR_TASKS				64 

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS-1]

#define TASK_RUNNING				0
#define TASK_ZOMBIE				1

#define PF_KTHREAD				0x00000002	

#define TIF_NEED_RESCHED		1	

#define NICE_0_LOAD_SHIFT		10
#define NICE_0_LOAD				(1L << NICE_0_LOAD_SHIFT)


extern struct task_struct *current;
extern struct task_struct * task[NR_TASKS];
extern int nr_tasks;

struct cpu_context {
	unsigned long x19;
	unsigned long x20;
	unsigned long x21;
	unsigned long x22;
	unsigned long x23;
	unsigned long x24;
	unsigned long x25;
	unsigned long x26;
	unsigned long x27;
	unsigned long x28;
	unsigned long fp;
	unsigned long sp;
	unsigned long pc;
};

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
	struct cpu_context cpu_context;
	long state;	
	long counter;
	long priority;
	long preempt_count;
	unsigned long flags;
	unsigned long thread_flags;		//linux에서 thread_info 구조체의 멤버변수 
	struct sched_entity se;
	struct mm_struct mm;
};

struct sched_entity {
	struct load_weight		load;
	struct rb_node 			run_node;
	unsigned int 			on_rq;

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

extern void sched_init(void);
extern void schedule(void);
extern void timer_tick(void);
extern void preempt_disable(void);
extern void preempt_enable(void);
extern void switch_to(struct task_struct* next);
extern void cpu_switch_to(struct task_struct* prev, struct task_struct* next);
extern void exit_process(void);

static inline int test_tsk_need_resched(struct task_struct *tsk)
{
	return tsk->thread_flags & TIF_NEED_RESCHED;
}

static inline void set_tsk_need_resched(struct task_struct *tsk)
{
	tsk->thread_flags &= TIF_NEED_RESCHED;
}

#define INIT_TASK \
/*cpu_context*/ { { 0,0,0,0,0,0,0,0,0,0,0,0,0}, \
/* state etc */	 0,0,15, 0, PF_KTHREAD, 0, \
/* sched_entity */ { {0,0}, {0,0,0}, 0, 0, 0, 0, 0, 0 }, \
/* mm */ { 0, 0, {{0}}, 0, {0}} \
}
#endif
#endif
