#ifndef __ASM_ZPU_CURRENT_H
#define __ASM_ZPU_CURRENT_H

#include <linux/thread_info.h>

struct task_struct;

inline static struct task_struct * get_current(void)
{
	return current_thread_info()->task;
}

#define current get_current()

#endif /* __ASM_ZPU_CURRENT_H */
