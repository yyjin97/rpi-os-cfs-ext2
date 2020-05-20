#ifndef _FAIR_H
#define _FAIR_H

#include "sched.h"

void __update_inv_weight(struct load_weight *lw);

u64 __calc_delta(u64 delta_exec, unsigned long weight, struct load_weight *lw);

extern struct task_struct *task_of(struct sched_entity *se);
extern struct cfs_rq *task_cfs_rq(struct task_struct *p);

void update_min_vruntime(struct cfs_rq *cfs_rq);
void __enqueue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se);
void __dequeue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se);

struct sched_entity *__pick_first_entity(struct cfs_rq *cfs_rq);

u64 __sched_period(unsigned long nr_running); 
u64 sched_slice(struct cfs_rq *cfs_rq, struct sched_entity *se);
u64 sched_vslice(struct cfs_rq *cfs_rq, struct sched_entity *se);

extern void place_entity(struct cfs_rq *cfs_rq, struct sched_entity *se);

void update_curr(struct cfs_rq *cfs_rq);
void enqueue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se); 
void dequeue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se); 
void check_preempt_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr);
void set_next_entity(struct cfs_rq *cfs_rq, struct sched_entity *se);
struct sched_entity * pick_next_entity(struct cfs_rq *cfs_rq, struct sched_entity *curr);
void put_prev_entity(struct cfs_rq *cfs_rq, struct sched_entity *prev);
void entity_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr);
void task_tick_fair(struct task_struct *curr);
extern void task_fork_fair(struct task_struct *p);

void account_entity_enqueue(struct cfs_rq *cfs_rq, struct sched_entity *se);
void account_entity_dequeue(struct cfs_rq *cfs_rq, struct sched_entity *se);

extern struct task_struct * pick_next_task_fair(struct cfs_rq *cfs_rq, struct task_struct *prev);

#endif