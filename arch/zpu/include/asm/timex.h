#ifndef __ASM_ZPU_TIMEX_H
#define __ASM_ZPU_TIMEX_H

#define CLOCK_TICK_RATE		500000	/* Underlying HZ */

typedef unsigned long cycles_t;

static inline cycles_t get_cycles (void)
{
	return 0;
}

#define ARCH_HAS_READ_CURRENT_TIMER

#endif /* __ASM_ZPU_TIMEX_H */
