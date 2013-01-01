#ifndef _ASM_ZPU_GPIO_H_
#define _ASM_ZPU_GPIO_H_

#define ARCH_NR_GPIOS	128


/* Arch-neutral GPIO API, supporting both "native" and external GPIOs. */
#include <asm-generic/gpio.h>

static inline int gpio_get_value(unsigned int gpio)
{
	return __gpio_get_value(gpio);
}

static inline void gpio_set_value(unsigned int gpio, int value)
{
	__gpio_set_value(gpio, value);
}

static inline int gpio_cansleep(unsigned int gpio)
{
	return __gpio_cansleep(gpio);
}


static inline int gpio_to_irq(unsigned int gpio)
{
	return -EINVAL;
}

static inline int irq_to_gpio(unsigned int irq)
{
	return irq;
}



#endif
