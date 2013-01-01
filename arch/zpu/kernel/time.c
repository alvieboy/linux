/*
 * Copyright (C) 2004-2007 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/time.h>

static cycle_t read_cycle_count(struct clocksource *cs)
{
	return ZPU_TIMERTSC;
}

/*
 * The architectural cycle count registers are a fine clocksource unless
 * the system idle loop use sleep states like "idle":  the CPU cycles
 * measured by COUNT (and COMPARE) don't happen during sleep states.
 * Their duration also changes if cpufreq changes the CPU clock rate.
 * So we rate the clocksource using COUNT as very low quality.
 */
static struct clocksource counter = {
	.name		= "zpuino_counter",
	.rating		= 50,
	.read		= read_cycle_count,
	.mask		= CLOCKSOURCE_MASK(32),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

static inline void EARLY_DEBUG(char c)
{
	while ((UARTCTL&0x2)==2);
	UARTDATA=c;
}

static irqreturn_t timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evdev = dev_id;

/*	if (unlikely(!(intc_get_pending(0) & 1)))
		return IRQ_NONE;
		*/
	//EARLY_DEBUG('+');
	ZPU_TMR0CTL &= ~(1<<TCTLIF);
	evdev->event_handler(evdev);
	//EARLY_DEBUG('-');

	return IRQ_HANDLED;
}

static struct irqaction timer_irqaction = {
	.handler	= timer_interrupt,
	.flags		= IRQF_TIMER | IRQF_DISABLED,
	.name		= "timer",
};

static int comparator_next_event(unsigned long delta,
		struct clock_event_device *evdev)
{
	unsigned long	flags;

	raw_local_irq_save(flags);

	/* The time to read COUNT then update COMPARE must be less
	 * than the min_delta_ns value for this clockevent source.
	 */
//	sysreg_write(COMPARE, (sysreg_read(COUNT) + delta) ? : 1);

	raw_local_irq_restore(flags);

	return 0;
}

#define SYSTEM_CLOCK 96000000
#define PRESCALER 16

static void comparator_mode(enum clock_event_mode mode,
		struct clock_event_device *evdev)
{
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		ZPU_TMR0CTL = 0; /* Stop timer first */
		ZPU_TMR0CNT = 0;
		ZPU_TMR0CMP = ((SYSTEM_CLOCK/PRESCALER)/HZ)-1;
		ZPU_TMR0CTL = (1<<TCTLENA) | (1<<TCTLCCM) | (1<<TCTLDIR)
			| (1<<TCTLCP2) | (1<<TCTLIEN);
		printk(KERN_INFO "ZPU: setup timer OK\n");
		break;
	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_RESUME:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
		break;
	default:
		BUG();
	}
}

static struct clock_event_device comparator = {
	.name		= "zpu_clockevent",
	.features	= CLOCK_EVT_FEAT_PERIODIC,
	.set_next_event	= comparator_next_event,
    .shift = 16,
	.set_mode	= comparator_mode,
};

void read_persistent_clock(struct timespec *ts)
{
	ts->tv_sec = mktime(2007, 1, 1, 0, 0, 0);
	ts->tv_nsec = 0;
}

void __init time_init(void)
{
	unsigned long counter_hz;
	int ret;

	/* figure rate for counter */
	counter_hz = SYSTEM_CLOCK;//clk_get_rate( SYSTEM_CLOCK );
	ret = clocksource_register_hz(&counter, counter_hz);
	if (ret)
		printk(KERN_INFO "timer: could not register clocksource: %d\n", ret);

	/* setup COMPARE clockevent */
	comparator.mult = div_sc(counter_hz, NSEC_PER_SEC, comparator.shift);
	comparator.max_delta_ns = clockevent_delta2ns((u32)~0, &comparator);
	comparator.min_delta_ns = clockevent_delta2ns(50, &comparator) + 1;
	comparator.cpumask = cpumask_of(0);

	//sysreg_write(COMPARE, 0);
	timer_irqaction.dev_id = &comparator;

	ret = setup_irq(3, &timer_irqaction);
	if (ret)
		printk(KERN_INFO "timer: could not request IRQ 3: %d\n", ret);
	else {
		clockevents_register_device(&comparator);

		printk(KERN_INFO "%s: irq 3, %lu.%03lu MHz\n", comparator.name,
				((counter_hz + 500) / 1000) / 1000,
				((counter_hz + 500) / 1000) % 1000);
	}
}
