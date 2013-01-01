/*
 */
#ifndef __ASM_ZPU_BUG_H
#define __ASM_ZPU_BUG_H

#define HAVE_ARCH_BUG
#define BUG() __asm__ __volatile__ ("breakpoint\n")

#include <asm-generic/bug.h>

#endif /* __ASM_ZPU_BUG_H */
