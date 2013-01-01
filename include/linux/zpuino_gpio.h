/*
 * zpuino_gpio.h ZPUino GPIO driver, platform data definition
 * Copyright (C) 2012 Alvaro Lopes
 *
 */

#ifndef _LINUX_ZPUINO_GPIO_H
#define _LINUX_ZPUINO_GPIO_H

struct zpuinogpio_platform_data {
	unsigned gpio_base;
	int nr_pins;
};

#endif
