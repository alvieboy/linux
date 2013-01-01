/*
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/user.h>
#include <linux/security.h>
#include <linux/unistd.h>
#include <linux/notifier.h>


void ptrace_disable(struct task_struct *child)
{
    asm("breakpoint");
}

long arch_ptrace(struct task_struct *child, long request,
				 unsigned long addr, unsigned long data)
{
	asm("breakpoint");
	return -1;
}

