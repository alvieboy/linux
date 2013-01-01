#include <linux/clk.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/mmc/host.h>
#include <linux/spi/mmc_spi.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/syscore_ops.h>
#include <linux/export.h>
#include <linux/spi/spi.h>
#include <linux/spi/zpuino_spi.h>
#include <asm/thread_info.h>
#include <linux/sched.h>
#include <linux/zpuino_gpio.h>
#include <linux/module.h>
#include <asm/io.h>
#include <asm/cacheflush.h>

static void makejump(unsigned address, unsigned char *location)
{
	int i=4;
	do {
		location[i--] = ((unsigned)address & 0x7f) | 0x80;
		address>>=7;
	} while (i!=-1);

	location[5]=0x04;
}

extern asmlinkage void handle_interrupt(void);
extern asmlinkage void do_syscall(void);

extern void zpu_crash(void);
extern void printhex(unsigned);
extern void printhexbyte(unsigned);

static inline void EARLY_DEBUG(char c)
{
	while ((UARTCTL&0x2)==2);
	UARTDATA=c;
}

static inline void EARLY_DEBUG_STR(const char *c)
{
	while (*c) {
		EARLY_DEBUG(*c++);
	}
}

#define TRACEBUFFER ZPU_IO_SLOT(14)

void zpu_crash_impl(unsigned old_sp)
{
	int i;
	unsigned origsp= old_sp;
	unsigned dpos,cpos;
	ZPU_REGISTER(ZPU_IO_SLOT(14),0)=0;

	EARLY_DEBUG_STR("\r\n\r\nCRASH SP 0x");
	printhex(old_sp);
	old_sp -= 16;
	old_sp = 0x00425fac;

	EARLY_DEBUG_STR("\r\nSTACK TRACE:\r\n");
	for (i=0;i<64;i++) {
		EARLY_DEBUG_STR("SP 0x"); printhex(old_sp);
		EARLY_DEBUG_STR(": 0x"); printhex(*((unsigned*)old_sp));
		if (old_sp==origsp)
			EARLY_DEBUG('*');
		old_sp += 4;
		EARLY_DEBUG_STR("\r\n");
	}


	cpos=ZPU_REGISTER(TRACEBUFFER,7);

	dpos=cpos++;
	do {
		unsigned o = cpos*8;
		EARLY_DEBUG_STR("0x");
		printhex(ZPU_REGISTER(TRACEBUFFER,(o+3))); //pc
		EARLY_DEBUG_STR(" 0x");
		printhexbyte(ZPU_REGISTER(TRACEBUFFER,o)); // opcode
		EARLY_DEBUG_STR(" 0x");
		printhex(ZPU_REGISTER(TRACEBUFFER,(o+2))<<2); // sp
		EARLY_DEBUG_STR(" 0x");
		printhex(ZPU_REGISTER(TRACEBUFFER,(o+1))); // tos
		EARLY_DEBUG_STR("\r\n");
		cpos++;
		cpos &= (1<<12)-1;
	}
	while (dpos!=cpos);

	while (1);
}

void __init setup_platform(void)
{
	printk(KERN_INFO "ZPU: setting fast paths for interrupt and syscall\n");

	makejump((unsigned)&handle_interrupt, (unsigned char*)0x020);
	makejump((unsigned)&do_syscall,       (unsigned char*)0x380);
	makejump((unsigned)&zpu_crash,        (unsigned char*)0x0);

	// @TODO: flush icache
	flush_icache_range(0x000, 0x400);
#if 0
		/* DEBUGGING - force a break if we ever jump to 0x0 */
	{
		unsigned *p = (unsigned *)0;
		*p = 0;
	}
#endif
}

void __init setup_board(void)
{
}


static void zpuino_mask(struct irq_data *d)
{
	unsigned int irq_source = 1<<d->irq;
	INTRMASK &= ~irq_source;
}

static void zpuino_unmask(struct irq_data *d)
{
	unsigned int irq_source = 1<<d->irq;
	INTRMASK |= irq_source;
}

struct irq_chip zpuino_irq_chip = {
	.name		= "zpuino-intc",
	.irq_mask	= zpuino_mask,
	.irq_mask_ack	= zpuino_mask,
	.irq_unmask	= zpuino_unmask,
};


void __init init_IRQ(void)
{
	int index;
	for (index = 0; index < NR_IRQS; ++index)
		irq_set_chip_and_handler(index, &zpuino_irq_chip,
					 handle_level_irq);
}


static struct resource spi_resource[] = {
	[0] = {
		.start = ZPU_IO_SLOT(6),
		.end   = ZPU_IO_SLOT(6) + 128,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start = 0,
		.end   = 0,
		.flags = IORESOURCE_DMA,
	}
};

static struct resource gpio_resource[] = {
	{
		.start = ZPU_IO_SLOT(2),
		.end   = ZPU_IO_SLOT(2) + 128,
		.flags  = IORESOURCE_MEM,
	}
};

static struct resource uart_resource[] = {
	{
		.start = ZPU_IO_SLOT(1),
		.end   = ZPU_IO_SLOT(1) + 16,
		.flags  = IORESOURCE_MEM,
	}
};

static struct zpuino_spi_platform_data spi_platform_data = {
    .gpio_cs = 36
};

static struct platform_device spi_device = {
	.name		= "zpuino_spi",
	.id		= 0,
	.resource	= spi_resource,
	.num_resources	= ARRAY_SIZE(spi_resource),
	.dev = {
		.platform_data = &spi_platform_data
	}
};

static struct zpuinogpio_platform_data gpio_platform_data = {
	.nr_pins = 128,
};


static struct platform_device gpio_device = {
	.name		= "zpuino_gpio",
	.id		= 1,
	.resource	= gpio_resource,
	.num_resources	= ARRAY_SIZE(gpio_resource),
	.dev = {
		.platform_data = &gpio_platform_data
	},
};

static struct platform_device uart_device = {
	.name		= "zpuino_uart",
	.id		= 2,
	.resource	= uart_resource,
	.num_resources	= ARRAY_SIZE(uart_resource),
};


static int mmc_spi_get_ro(struct device *dev)
{
	return 0;
}

static int mmc_spi_get_cd(struct device *dev)
{
	return 1;
}

static void mmc_spi_setpower(struct device *dev, unsigned int maskval)
{
}


static struct mmc_spi_platform_data mmc_spi_info = {
	.get_ro = mmc_spi_get_ro,
	.get_cd = mmc_spi_get_cd,
	.caps = MMC_CAP_NEEDS_POLL,
	.ocr_mask = MMC_VDD_32_33 | MMC_VDD_33_34, /* 3.3V only */
	.setpower = mmc_spi_setpower,
};

static struct spi_board_info spi_board_info[] = {
	{
		.modalias = "mmc_spi",
		.max_speed_hz = 96000000,
		.bus_num = 0,
		.chip_select = 0,
		.platform_data = &mmc_spi_info
	},
};

static int __init system_device_init(void)
{
	platform_device_register(&uart_device);
	platform_device_register(&spi_device);
	platform_device_register(&gpio_device);

	spi_register_board_info(spi_board_info,
							ARRAY_SIZE(spi_board_info));

	return 0;
}

core_initcall(system_device_init);
