/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc.
 *
 * But use these as seldom as possible since they are slower than
 * regular operations.
 *
 */
#ifndef __ASM_ZPU_CMPXCHG_H
#define __ASM_ZPU_CMPXCHG_H

#include <asm-generic/cmpxchg.h>

#endif /* __ASM_ZPU_CMPXCHG_H */
