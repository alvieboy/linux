/*
 */
#ifndef __ASM_ZPU_PROCESSOR_H
#define __ASM_ZPU_PROCESSOR_H

#include <asm/page.h>
#include <asm/cache.h>

#define KSTK_EIP(tsk)	((tsk)->thread.cpu_context.pc)
#define KSTK_ESP(tsk)	((tsk)->thread.cpu_context.ksp)

#undef ARCH_HAS_PREFETCH

#define cpu_relax()		barrier()

struct cpu_context {
	unsigned long pc;
	unsigned long ksp;	/* Kernel stack pointer */
};

/* This struct contains the CPU context as stored by switch_to() */
struct thread_struct {
	struct cpu_context cpu_context;
};

#define INIT_THREAD {						\
	.cpu_context = {					\
		.ksp = sizeof(init_stack) + (long)&init_stack,	\
	},							\
}

#define start_thread(regs, new_pc, new_sp)	 \
	do {					 \
		memset(regs, 0, sizeof(*regs));	 \
		regs->pc = new_pc;		 \
		regs->sp = new_sp - 4; /* keep some space for PC return */   \
	} while(0)

struct task_struct;

/* Free all resources held by a thread */
extern void release_thread(struct task_struct *);

/* Create a kernel thread without removing it from tasklists */
extern int kernel_thread(int (*fn)(void *), void *arg, unsigned long flags);

static inline void *current_text_addr(void)
{
	register void *pc asm("PC");
	return pc;
}

#define thread_saved_pc(tsk)    ((tsk)->thread.cpu_context.pc)

#define TASK_SIZE	0x80000000

#define TASK_UNMAPPED_BASE	(PAGE_ALIGN(TASK_SIZE / 3))

#define prepare_to_copy(tsk) do { } while(0)

struct pt_regs;
extern unsigned long get_wchan(struct task_struct *p);

#endif /* __ASM_ZPU_PROCESSOR_H */
