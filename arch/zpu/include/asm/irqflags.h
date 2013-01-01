/*
 */
#ifndef __ASM_ZPU_IRQFLAGS_H
#define __ASM_ZPU_IRQFLAGS_H


#include <linux/types.h>
#include <asm/sysreg.h>

extern void __zpu_set_interrupt_ctrl(unsigned val);

static inline unsigned long arch_local_save_flags(void)
{
	return (INTRCTL&1);
}

/*
 * This will restore ALL status register flags, not only the interrupt
 * mask flag.
 *
 * The empty asm statement informs the compiler of this fact while
 * also serving as a barrier.
 */
static inline void arch_local_irq_restore(unsigned long flags)
{
	__zpu_set_interrupt_ctrl(flags&1);
	asm volatile("nop\n nop\n" : : : "memory", "cc");
}

static inline void arch_local_irq_disable(void)
{
	INTRCTL &= ~1;
	asm volatile("nop\n nop\n");
}

static inline void arch_local_irq_enable(void)
{
	__zpu_set_interrupt_ctrl(1);
}

static bool __arch_irqs_disabled_flags(unsigned long flags)
{
	return (flags & 1) == 0;
}

static inline bool arch_irqs_disabled_flags(unsigned long flags)
{
	return __arch_irqs_disabled_flags(flags);
}

static inline bool arch_irqs_disabled(void)
{
	return arch_irqs_disabled_flags(arch_local_save_flags());
}

static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags = arch_local_save_flags();

	arch_local_irq_disable();

	return flags;
}

#endif /* __ASM_ZPU_IRQFLAGS_H */
