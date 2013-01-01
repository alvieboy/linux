/*
 * ZPUino Linux
 *
 * Copyright (C) 2012 Alvaro Lopes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/thread_info.h>
#include <linux/kbuild.h>

void foo(void)
{
	OFFSET(TI_task, thread_info, task);
	OFFSET(TI_exec_domain, thread_info, exec_domain);
	OFFSET(TI_flags, thread_info, flags);
	OFFSET(TI_cpu, thread_info, cpu);
	OFFSET(TI_preempt_count, thread_info, preempt_count);
	OFFSET(TI_restart_block, thread_info, restart_block);
	BLANK();
	OFFSET(TSK_active_mm, task_struct, active_mm);
	OFFSET(TSK_thread, task_struct, thread);

	OFFSET(TSK_TS_KSP, task_struct, thread.cpu_context.ksp);
	OFFSET(TSK_TS_PC, task_struct, thread.cpu_context.pc);
//	OFFSET(TSK_TS_R1, task_struct, thread.cpu_context.r1);
//	OFFSET(TSK_TS_R2, task_struct, thread.cpu_context.r2);
//	OFFSET(TSK_TS_R3, task_struct, thread.cpu_context.r3);
	BLANK();
	OFFSET(MM_pgd, mm_struct, pgd);
	BLANK();
}
