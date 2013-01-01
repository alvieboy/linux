/*
 * ZPUino GPIO driver
 * Copyright (C) 2012 Alvaro Lopes
 *
 */

#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/zpuino_gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

#define DRIVER_NAME "zpuino_gpio"

#define REGISTER(SLOT, y) (SLOT + (y<<2))

#define GPIODATA(base,x)     REGISTER(base,x)
#define GPIOTRIS(base,x)     REGISTER(base,4+x)
#define GPIOPPSMODE(base,x)  REGISTER(base,8+x)
#define GPIOPPSOUT(base,x)   REGISTER(base,(128 + x))
#define GPIOPPSIN(base,x)    REGISTER(base,(256 + x))

struct zpuinogpio {
	void __iomem		*regs;
	spinlock_t		lock; /* mutual exclusion */
	struct gpio_chip	gpio;
};

static int zpuinogpio_update_bit(struct gpio_chip *gpio, unsigned index,
	unsigned offset, bool enabled)
{
	struct zpuinogpio *zgpio = container_of(gpio, struct zpuinogpio, gpio);
	u32 reg;

//	printk(KERN_INFO "Update bit index %u offset %u value %d\n",index,offset,enabled);

	spin_lock(&zgpio->lock);
	reg = ioread32(zgpio->regs + (offset<<2));

	if (enabled)
		reg |= (1 << index);
	else
		reg &= ~(1 << index);

	iowrite32(reg, zgpio->regs + (offset<<2));
	spin_unlock(&zgpio->lock);

	return 0;
}

static int zpuinogpio_gpio_direction_input(struct gpio_chip *gpio, unsigned nr)
{
	return zpuinogpio_update_bit(gpio, nr % 32 , 4 + (nr / 32), true);
}

static int zpuinogpio_gpio_get(struct gpio_chip *gpio, unsigned nr)
{
	struct zpuinogpio *zgpio = container_of(gpio, struct zpuinogpio, gpio);
	u32 value;

	value = ioread32(zgpio->regs + ((nr/32)<<2) );
	return (value & (1 << (nr%32) )) ? 1 : 0;
}

static int zpuinogpio_gpio_direction_output(struct gpio_chip *gpio,
						unsigned nr, int val)
{
	return zpuinogpio_update_bit(gpio, nr%32 , 4 + (nr/32) , false);
}

static void zpuinogpio_gpio_set(struct gpio_chip *gpio,
				unsigned nr, int val)
{
	zpuinogpio_update_bit(gpio, nr % 32, nr/32, val != 0);
}

static struct irq_chip zpuinogpio_irqchip = {
	.name		= "GPIO"
};

static int __devinit zpuinogpio_probe(struct platform_device *pdev)
{
	int err, i;
	struct gpio_chip *gc;
	struct zpuinogpio *zgpio;
	struct resource *iomem;
	struct zpuinogpio_platform_data *pdata = pdev->dev.platform_data;

	if (!pdata) {
		err = -EINVAL;
		goto err_mem;
	}

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iomem) {
		err = -EINVAL;
		goto err_mem;
	}

	zgpio = kzalloc(sizeof(*zgpio), GFP_KERNEL);
	if (!zgpio) {
		err = -EINVAL;
		goto err_mem;
	}

	spin_lock_init(&zgpio->lock);

	if (!request_mem_region(iomem->start, resource_size(iomem),
		DRIVER_NAME)) {
		err = -EBUSY;
		goto err_request;
	}

	zgpio->regs = (void*)iomem->start;

	gc = &zgpio->gpio;

	gc->label = dev_name(&pdev->dev);
	gc->owner = THIS_MODULE;
	gc->dev = &pdev->dev;
	gc->direction_input = zpuinogpio_gpio_direction_input;
	gc->get = zpuinogpio_gpio_get;
	gc->direction_output = zpuinogpio_gpio_direction_output;
	gc->set = zpuinogpio_gpio_set;
	gc->to_irq = NULL;
	gc->dbg_show = NULL;
	gc->ngpio = pdata->nr_pins;
	gc->can_sleep = 0;

	err = gpiochip_add(gc);
	if (err)
		goto err_chipadd;

	platform_set_drvdata(pdev, zgpio);

	printk(KERN_INFO "ZPUino GPIO driver registered, %d pins\n",gc->ngpio);

	return 0;

err_chipadd:
	release_mem_region(iomem->start, resource_size(iomem));
err_request:
	kfree(zgpio);
err_mem:
	printk(KERN_ERR DRIVER_NAME": Failed to register GPIOs: %d\n", err);

	return err;
}

static int __devexit zpuinogpio_remove(struct platform_device *pdev)
{
	int err;
	struct zpuinogpio_platform_data *pdata = pdev->dev.platform_data;
	struct zpuinogpio *zgpio = platform_get_drvdata(pdev);
	struct resource *iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	err = gpiochip_remove(&zgpio->gpio);
	if (err)
		printk(KERN_ERR DRIVER_NAME": failed to remove gpio_chip\n");

	release_mem_region(iomem->start, resource_size(iomem));
	kfree(zgpio);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver zpuinogpio_platform_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= zpuinogpio_probe,
	.remove		= zpuinogpio_remove,
};

/*--------------------------------------------------------------------------*/

module_platform_driver(zpuinogpio_platform_driver);

MODULE_DESCRIPTION("ZPUino GPIO driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alvaro Lopes");
MODULE_ALIAS("platform:"DRIVER_NAME);

