/*
 */
#ifndef __ASM_ZPU_SIGCONTEXT_H
#define __ASM_ZPU_SIGCONTEXT_H

#include <asm/ptrace.h>

struct sigcontext {
	unsigned long	oldmask;
	struct pt_regs regs;
};

#endif /* __ASM_ZPU_SIGCONTEXT_H */
