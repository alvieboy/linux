/*
 */
#ifndef __ASM_ZPU_SWITCH_TO_H
#define __ASM_ZPU_SWITCH_TO_H

struct task_struct;

extern struct task_struct *__switch_to(struct task_struct *,
								struct task_struct *);

#define switch_to(prev, next, last)					\
	do {								\
	last = __switch_to(prev, next); \
	} while (0)


#endif /* __ASM_ZPU_SWITCH_TO_H */
