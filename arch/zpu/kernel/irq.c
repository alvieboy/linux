/*
 * Based on arch/i386/kernel/irq.c
 *   Copyright (C) 1992, 1998 Linus Torvalds, Ingo Molnar
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel_stat.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/device.h>
#include <asm/sysreg.h>
#include <linux/export.h>

/* May be overridden by platform code */
int __weak nmi_enable(void)
{
	return -ENOSYS;
}

void __weak nmi_disable(void)
{

}

void __zpu_set_interrupt_ctrl(unsigned val)
{
	INTRCTL = val;
}

void asmlinkage do_IRQ(struct pt_regs *regs)
{
	struct pt_regs *old_regs;
	unsigned served, line;

	old_regs = set_irq_regs(regs);
	irq_enter();
	/* Lookup interrupt line */
	served = INTRLINE;
	/* Check which bit is set */
	for (line=0; line<32; line++) {
		if (served&1)
			break;
		served>>=1;
	}
	if (line==32) {
		/* Bug */
		printk(KERN_ERR "HW problem, interrupt called with no interrupt!");
	} else {
		generic_handle_irq(line);
	}
	irq_exit();
	set_irq_regs(old_regs);
}

int show_interrupts(struct seq_file *p, void *v)
{
	return -EIO;
}
EXPORT_SYMBOL(show_interrupts);
