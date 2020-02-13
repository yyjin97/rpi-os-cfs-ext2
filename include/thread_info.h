#ifndef _THREAD_INFO_H
#define _THREAD_INFO_H

#define TIF_NEED_RESCHED		1	

#define current_thread_info() ((struct thread_info *)current)

/* /arch/arm64/include/asm/proccessor.h */
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

struct thread_info {
    struct cpu_context  cpu_context;        //linux kernel에서는 thread_struct 구조체에 존재
    unsigned long       flags;
    int                 preempt_count;
};

static inline void set_ti_thread_flag(struct thread_info *ti, int flag)
{
    ti->flags = (unsigned long)flag;
}

static inline void clear_ti_thread_flag(struct thread_info *ti, int flag)
{
    ti->flags = 0;
}

static inline int test_ti_thread_flag(struct thread_info *ti, int flag)
{
    return ti->flags == (unsigned long)flag ? 1 : 0;
}

#endif