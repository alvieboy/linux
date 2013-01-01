/*
 */
#ifndef __ASM_ZPU_BARRIER_H
#define __ASM_ZPU_BARRIER_H

#define nop()			asm volatile("nop")

#define mb()			asm volatile("" : : : "memory")
#define rmb()			mb()
#define wmb()			asm volatile("" : : : "memory")
#define read_barrier_depends()  do { } while(0)
#define set_mb(var, value)      do { var = value; mb(); } while(0)

#ifdef CONFIG_SMP
# error "The ZPU port does not support SMP"
#else
# define smp_mb()		barrier()
# define smp_rmb()		barrier()
# define smp_wmb()		barrier()
# define smp_read_barrier_depends() do { } while(0)
#endif


#endif /* __ASM_ZPU_BARRIER_H */
