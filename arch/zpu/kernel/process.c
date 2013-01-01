/*
 * ZPUino Linux
 *
 * Copyright (C) 2012 Alvaro Lopes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/sched.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/fs.h>
#include <linux/pm.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <linux/tick.h>
#include <linux/uaccess.h>
#include <linux/unistd.h>

void (*pm_power_off)(void);
EXPORT_SYMBOL(pm_power_off);

unsigned int current_save;
unsigned int status_reg;

#undef ALVIE_DEBUG

void cpu_idle(void)
{
	/* endless idle loop with no priority at all */
	while (1) {
		tick_nohz_idle_enter();
		rcu_idle_enter();
		while (!need_resched())
			asm volatile ("nop");
		rcu_idle_exit();
		tick_nohz_idle_exit();
		schedule_preempt_disabled();
	}
}

void machine_halt(void)
{
    /* We do a simple endless loop here */
	while (1) {
		__asm__("nop\n");
	}
}

void machine_power_off(void)
{
	if (pm_power_off)
		pm_power_off();
}

void machine_restart(char *cmd)
{
	/* This will not work, it will cause a "crash" */
	asm volatile ("im 0\n"
				  "poppc\n");
}

asmlinkage extern void kernel_thread_helper(void);

int kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
{
	struct pt_regs regs;

	memset(&regs, 0, sizeof(regs));

	regs.r1 = (unsigned long)arg;
	regs.r2 = (unsigned long)fn;
	regs.r3 = (unsigned long)do_exit;
	regs.pc = (unsigned long)kernel_thread_helper;
	regs.status_extension = PS_S;

	return do_fork(flags | CLONE_VM | CLONE_UNTRACED,
		       0, &regs, 0, NULL, NULL);
}
EXPORT_SYMBOL(kernel_thread);

void exit_thread(void)
{
	/* nothing to do */
}

void flush_thread(void)
{
	/* nothing to do */
}

void release_thread(struct task_struct *dead_task)
{
	/* do nothing */
}

static void dump_mem(const char *str, const char *log_lvl,
		     unsigned long bottom, unsigned long top)
{
	unsigned long p;
	int i;

	printk("%s%s(0x%08lx to 0x%08lx)\n", log_lvl, str, bottom, top);

	for (p = bottom & ~31; p < top; ) {
		printk("%s%04lx: ", log_lvl, p & 0xffff);

		for (i = 0; i < 8; i++, p += 4) {
			unsigned int val;

			if (p < bottom || p >= top)
				printk("         ");
			else {
				if (__get_user(val, (unsigned int __user *)p)) {
					printk("\n");
					goto out;
				}
				printk("%08x ", val);
			}
		}
		printk("\n");
	}

out:
	return;
}

static inline int valid_stack_ptr(struct thread_info *tinfo, unsigned long p)
{
	return (p > (unsigned long)tinfo)
		&& (p < (unsigned long)tinfo + THREAD_SIZE - 3);
}

#ifdef CONFIG_FRAME_POINTER
static void show_trace_log_lvl(struct task_struct *tsk, unsigned long *sp,
			       struct pt_regs *regs, const char *log_lvl)
{
#if 0
	unsigned long lr, fp;
	struct thread_info *tinfo;

	if (regs)
		fp = regs->r7;
	else if (tsk == current)
		asm("mov %0, r7" : "=r"(fp));
	else
		fp = tsk->thread.cpu_context.r7;

	/*
	 * Walk the stack as long as the frame pointer (a) is within
	 * the kernel stack of the task, and (b) it doesn't move
	 * downwards.
	 */
	tinfo = task_thread_info(tsk);
	printk("%sCall trace:\n", log_lvl);
	while (valid_stack_ptr(tinfo, fp)) {
		unsigned long new_fp;

		lr = *(unsigned long *)fp;
#ifdef CONFIG_KALLSYMS
		printk("%s [<%08lx>] ", log_lvl, lr);
#else
		printk(" [<%08lx>] ", lr);
#endif
		print_symbol("%s\n", lr);

		new_fp = *(unsigned long *)(fp + 4);
		if (new_fp <= fp)
			break;
		fp = new_fp;
	}
	printk("\n");
#endif
}
#else
static void show_trace_log_lvl(struct task_struct *tsk, unsigned long *sp,
			       struct pt_regs *regs, const char *log_lvl)
{
	unsigned long addr;

	printk("%sCall trace:\n", log_lvl);

	while (!kstack_end(sp)) {
		addr = *sp++;
		if (kernel_text_address(addr)) {
#ifdef CONFIG_KALLSYMS
			printk("%s [<%08lx>] ", log_lvl, addr);
#else
			printk(" [<%08lx>] ", addr);
#endif
			print_symbol("%s\n", addr);
		}
	}
	printk("\n");
}
#endif

void show_stack_log_lvl(struct task_struct *tsk, unsigned long sp,
			struct pt_regs *regs, const char *log_lvl)
{
	struct thread_info *tinfo;

	if (sp == 0) {
		if (tsk)
			sp = tsk->thread.cpu_context.ksp;
		else
			sp = (unsigned long)&tinfo;
	}
	if (!tsk)
		tsk = current;

	tinfo = task_thread_info(tsk);

	if (valid_stack_ptr(tinfo, sp)) {
		dump_mem("Stack: ", log_lvl, sp,
			 THREAD_SIZE + (unsigned long)tinfo);
		show_trace_log_lvl(tsk, (unsigned long *)sp, regs, log_lvl);
	}
}

void show_stack(struct task_struct *tsk, unsigned long *stack)
{
	show_stack_log_lvl(tsk, (unsigned long)stack, NULL, "");
}

void dump_stack(void)
{
	unsigned long stack;

	show_trace_log_lvl(current, &stack, NULL, "");
}
EXPORT_SYMBOL(dump_stack);

static const char *cpu_modes[] = {
	"Application", "Supervisor"
};

void show_regs_log_lvl(struct pt_regs *regs, const char *log_lvl)
{
}

void show_regs(struct pt_regs *regs)
{
}

EXPORT_SYMBOL(show_regs);

asmlinkage void ret_from_fork(void);

int copy_thread(unsigned long clone_flags, unsigned long usp,
		unsigned long unused,
		struct task_struct *p, struct pt_regs *regs)
{
	struct pt_regs *childregs;

	childregs = ((struct pt_regs *)(THREAD_SIZE - 8 + (unsigned long)task_stack_page(p))) - 1;
	*childregs = *regs;

	if (user_mode(regs))
		childregs->sp = usp;
	else
		childregs->sp = (unsigned long)task_stack_page(p) + THREAD_SIZE - 8;

	p->thread.cpu_context.ksp = (unsigned long)childregs;
	p->thread.cpu_context.pc =  (unsigned long)ret_from_fork;

#ifdef ALVIE_DEBUG
	printk(KERN_INFO "Creating new thread with sp @ 0x%08x , ksp @ 0x%08x and PC 0x%08x, task struct 0x%08x\n",
	       (unsigned)childregs->sp,
	       (unsigned)p->thread.cpu_context.ksp,
	       (unsigned)regs->pc,
	       (unsigned)p);
#endif
	clear_tsk_thread_flag(p, TIF_DEBUG);
	return 0;
}

asmlinkage int sys_fork(struct pt_regs *regs)
{
	return do_fork(SIGCHLD, regs->sp, regs, 0, NULL, NULL);
}

asmlinkage int sys_clone(unsigned long clone_flags, unsigned long newsp,
		void __user *parent_tidptr, void __user *child_tidptr,
		struct pt_regs *regs)
{
	if (!newsp)
		newsp = regs->sp;
	return do_fork(clone_flags, newsp, regs, 0, parent_tidptr,
			child_tidptr);
}

asmlinkage int sys_vfork(struct pt_regs *regs)
{
	return do_fork(CLONE_VFORK | CLONE_VM | SIGCHLD, regs->sp, regs,
		       0, NULL, NULL);
}

asmlinkage int sys_execve(const char __user *ufilename,
			  const char __user *const __user *uargv,
			  const char __user *const __user *uenvp,
			  struct pt_regs *regs)
{
	int error;
	char *filename;

	filename = getname(ufilename);
	error = PTR_ERR(filename);
	if (IS_ERR(filename))
		goto out;

	error = do_execve(filename, uargv, uenvp, regs);
	putname(filename);

out:
	return error;
}


/*
 * This function is supposed to answer the question "who called
 * schedule()?"
 */
unsigned long get_wchan(struct task_struct *p)
{
	unsigned long pc;
	unsigned long stack_page;

	if (!p || p == current || p->state == TASK_RUNNING)
		return 0;

	stack_page = (unsigned long)task_stack_page(p);
	BUG_ON(!stack_page);

	/*
	 * The stored value of PC is either the address right after
	 * the call to __switch_to() or ret_from_fork.
	 */
	pc = thread_saved_pc(p);
	if (in_sched_functions(pc)) {
#ifdef CONFIG_FRAME_POINTER
		unsigned long fp = p->thread.cpu_context.r7;
		BUG_ON(fp < stack_page || fp > (THREAD_SIZE + stack_page));
		pc = *(unsigned long *)fp;
#else
		/*
		 * We depend on the frame size of schedule here, which
		 * is actually quite ugly. It might be possible to
		 * determine the frame size automatically at build
		 * time by doing this:
		 *   - compile sched.c
		 *   - disassemble the resulting sched.o
		 *   - look for 'sub sp,??' shortly after '<schedule>:'
		 */
		unsigned long sp = p->thread.cpu_context.ksp + 16;
		BUG_ON(sp < stack_page || sp > (THREAD_SIZE + stack_page));
		pc = *(unsigned long *)sp;
#endif
	}

	return pc;
}

