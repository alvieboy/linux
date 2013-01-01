/*
 * syscalls.h - Linux syscall interfaces (arch-specific)
 */

#ifndef _ASM_ZPU_SYSCALLS_H
#define _ASM_ZPU_SYSCALLS_H

#include <linux/compiler.h>
#include <linux/linkage.h>
#include <linux/types.h>
#include <linux/signal.h>

/* mm/cache.c */
asmlinkage int sys_cacheflush(int, void __user *, size_t);

#endif /* _ASM_ZPU_SYSCALLS_H */
