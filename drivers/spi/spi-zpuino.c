/*
 * ZPUino SPI controller driver (master mode only)
 *
 * Copyright (C) Alvaro Lopes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/spi/zpuino_spi.h>

#define USPICTL_OFFSET 0
#define USPIDATA_OFFSET 4
#define USPIDATA16_OFFSET 12
#define USPIDATA32_OFFSET 20

/* SPI bits */
#define SPIREADY 0 /* SPI ready */
#define SPICP0   1 /* Clock prescaler bit 0 */
#define SPICP1   2 /* Clock prescaler bit 1 */
#define SPICP2   3 /* Clock prescaler bit 2 */
#define SPICPOL  4 /* Clock polarity */
#define SPISRE   5 /* Sample on Rising Edge */
#define SPIEN    6 
#define SPIBLOCK 7

#define _BV(x) (1<<(x))
#define ZSPI_MODE_MASK ( _BV(SPICPOL) | _BV(SPISRE) )

struct zpuino_spi {
	/* bitbang has to be first */
	struct spi_bitbang bitbang;
	struct completion done;
	struct resource mem; /* phys mem */
	void __iomem	*regs;	/* virt. address of the control registers */

	u32		irq;
	u32  select_pin;
    u8 bits_per_word;
	u8 *rx_ptr;		/* pointer in the Tx buffer */
	const u8 *tx_ptr;	/* pointer in the Rx buffer */
	int remaining_bytes;	/* the number of bytes left to transfer */

	unsigned int (*read_fn) (void __iomem *);
	void (*write_fn) (u32, void __iomem *);
	void (*tx_fn) (struct zpuino_spi *);
	void (*rx_fn) (struct zpuino_spi *);
};

static void zspi_write32(u32 val, void __iomem *addr)
{
	/*if (addr<0x08000000) {
        asm("breakpoint\n");
	} */
	iowrite32(val, addr);
	//printk(KERN_INFO "ZPI TX: %p 0x%08x\n", addr,val);
}

static unsigned int zspi_read32(void __iomem *addr)
{
	unsigned v = ioread32(addr);
	//printk(KERN_INFO "ZPI READ: %p 0x%08x\n", addr, v);
	return v;
}

/*
static void zspi_write32_le(u32 val, void __iomem *addr)
{
	iowrite32le(val, addr);
}

static unsigned int zspi_read32_le(void __iomem *addr)
{
	return ioread32le(addr);
}
*/
static void zspi_tx8(struct zpuino_spi *zspi)
{
	zspi->write_fn(*zspi->tx_ptr, zspi->regs + USPIDATA_OFFSET);
//	printk(KERN_INFO "TX %02x\n", *zspi->tx_ptr);
	zspi->tx_ptr++;
}

static void zspi_tx16(struct zpuino_spi *zspi)
{
	zspi->write_fn(*(u16 *)(zspi->tx_ptr), zspi->regs + USPIDATA16_OFFSET);
	zspi->tx_ptr += 2;
}

static void zspi_tx32(struct zpuino_spi *zspi)
{
	zspi->write_fn(*(u32 *)(zspi->tx_ptr), zspi->regs + USPIDATA32_OFFSET);
	zspi->tx_ptr += 4;
}

static void zspi_rx8(struct zpuino_spi *zspi)
{
	u32 data = zspi->read_fn(zspi->regs + USPIDATA_OFFSET);
	if (zspi->rx_ptr) {
		*zspi->rx_ptr = data & 0xff;
//		printk(KERN_INFO "RX %02x\n", *zspi->rx_ptr);
		zspi->rx_ptr++;
	}
}

static void zspi_rx16(struct zpuino_spi *zspi)
{
	u32 data = zspi->read_fn(zspi->regs + USPIDATA16_OFFSET);
	if (zspi->rx_ptr) {
		*(u16 *)(zspi->rx_ptr) = data & 0xffff;
		zspi->rx_ptr += 2;
	}
}

static void zspi_rx32(struct zpuino_spi *zspi)
{
	u32 data = zspi->read_fn(zspi->regs + USPIDATA32_OFFSET);
	if (zspi->rx_ptr) {
		*(u32 *)(zspi->rx_ptr) = data;
		zspi->rx_ptr += 4;
	}
}

static void zspi_init_hw(struct zpuino_spi *zspi)
{
	void __iomem *regs_base = zspi->regs;

	zspi->write_fn( (1<<SPIEN)|(1<<SPICP1)|(1<<SPIBLOCK)|(1<SPICPOL)|(1<SPISRE), regs_base + USPICTL_OFFSET);
}

static void zpuino_spi_chipselect(struct spi_device *spi, int is_on)
{
	struct zpuino_spi *zspi = spi_master_get_devdata(spi->master);
	//  prntk(KERN_INFO "SPI CS: %d, pin %d\n", is_on, zspi->select_pin);

	if (is_on == BITBANG_CS_INACTIVE) {
		/* Deselect the slave on the SPI bus */
		/* TODO */
        gpio_set_value( zspi->select_pin, 1);
        //asm("breakpoint \n .long 0xdead2000 \n");
	} else if (is_on == BITBANG_CS_ACTIVE) {
		/* Set the SPI clock phase and polarity */
		u16 cr = zspi->read_fn(zspi->regs + USPICTL_OFFSET) & ~ZSPI_MODE_MASK;

		if (spi->mode & SPI_CPHA)
			cr |= _BV(SPISRE);

		if (spi->mode & SPI_CPOL)
		cr |= _BV(SPICPOL);

		zspi->write_fn(cr, zspi->regs + USPICTL_OFFSET);

		/* We do not check spi->max_speed_hz here as the SPI clock
		 * frequency is not software programmable (the IP block design
		 * parameter)
		 */

		/* Activate the chip select */
		/* TODO */
		gpio_set_value( zspi->select_pin, 0);
		//asm("breakpoint \n .long 0xdead2001 \n");
	}
}

static int zpuino_spi_setup_transfer(struct spi_device *spi,
		struct spi_transfer *t)
{
	struct zpuino_spi *zspi = spi_master_get_devdata(spi->master);
	u8 bits_per_word;

	bits_per_word = (t && t->bits_per_word)
			 ? t->bits_per_word : spi->bits_per_word;
	if (bits_per_word != zspi->bits_per_word) {
		dev_err(&spi->dev, "%s, unsupported bits_per_word=%d\n",
			__func__, bits_per_word);
		return -EINVAL;
	}

	return 0;
}

static int zpuino_spi_setup(struct spi_device *spi)
{
	int i;
	struct zpuino_spi *zspi = spi_master_get_devdata(spi->master);

	gpio_set_value( zspi->select_pin, 1);

	for (i=0;i<5;i++) {
		zspi->write_fn(0xffffffff,zspi->regs + USPIDATA32_OFFSET);
	}

	return 0;
}

static void zpuino_spi_fill_tx_fifo(struct zpuino_spi *zspi)
{
	while (zspi->remaining_bytes > 0) {
		if (zspi->tx_ptr)
			zspi->tx_fn(zspi);
		else {
			// What to do ?
			//asm("breakpoint \n .long 0xdead2002 \n");
		}
		zspi->remaining_bytes -= zspi->bits_per_word / 8;
	}
}

static int zpuino_spi_txrx_bufs(struct spi_device *spi, struct spi_transfer *t)
{
	struct zpuino_spi *zspi = spi_master_get_devdata(spi->master);
	int bytes_per_transfer = zspi->bits_per_word / 8;
	u16 cr;

	/* We get here with transmitter inhibited */

	zspi->tx_ptr = t->tx_buf;
	zspi->rx_ptr = t->rx_buf;
	zspi->remaining_bytes = t->len;
	//printk(KERN_INFO "txrx: wr %p rd %p size %d\n",zspi->tx_ptr,zspi->rx_ptr,t->len);

	while (zspi->remaining_bytes >= bytes_per_transfer) {
		zspi->tx_fn(zspi);
		zspi->rx_fn(zspi);
		zspi->remaining_bytes -= bytes_per_transfer;
	}
    /*
	 if (t->rx_buf) {
	 int i;
	 printk(KERN_INFO "Rx: ");
		for (i=0;i<t->len;i++) {
		printk(KERN_INFO "%02x", ((unsigned char*)t->rx_buf)[i]);
		}
		}
		*/

	return t->len - zspi->remaining_bytes;
}

struct spi_master *zpuino_spi_init(struct device *dev, struct resource *mem,
	s16 bus_num, int num_cs, int little_endian, int bits_per_word, int cs_pin)
{
	struct spi_master *master;
	struct zpuino_spi *zspi;
	int ret;

	master = spi_alloc_master(dev, sizeof(struct zpuino_spi));
	if (!master)
		return NULL;

	/* the spi->mode bits understood by this driver: */
	master->mode_bits = SPI_CPOL | SPI_CPHA;

	zspi = spi_master_get_devdata(master);
	zspi->bitbang.master = spi_master_get(master);
	zspi->bitbang.chipselect = zpuino_spi_chipselect;
	zspi->bitbang.setup_transfer = zpuino_spi_setup_transfer;
	zspi->bitbang.txrx_bufs = zpuino_spi_txrx_bufs;
	zspi->bitbang.master->setup = zpuino_spi_setup;
	init_completion(&zspi->done);

	if (!request_mem_region(mem->start, resource_size(mem),
		"ZPUino-SPI"))
		goto put_master;

	zspi->regs = (void*)mem->start; /* No io remap */

	master->bus_num = bus_num;
	master->num_chipselect = num_cs;
//	master->dev.of_node = dev->of_node;

	zspi->mem = *mem;
	
	
	zspi->read_fn = zspi_read32;
	zspi->write_fn = zspi_write32;
	zspi->select_pin = cs_pin; /* TODO */
	zspi->bits_per_word = bits_per_word;

	if (zspi->bits_per_word == 8) {
		zspi->tx_fn = zspi_tx8;
		zspi->rx_fn = zspi_rx8;
	} else if (zspi->bits_per_word == 16) {
		zspi->tx_fn = zspi_tx16;
		zspi->rx_fn = zspi_rx16;
	} else if (zspi->bits_per_word == 32) {
		zspi->tx_fn = zspi_tx32;
		zspi->rx_fn = zspi_rx32;
	} else
		goto map_failed;
	
    gpio_direction_output(zspi->select_pin, 1);

	/* SPI controller initializations */
	zspi_init_hw(zspi);

	ret = spi_bitbang_start(&zspi->bitbang);
	if (ret) {
		dev_err(dev, "spi_bitbang_start FAILED\n");
		goto map_failed;
	}

	dev_info(dev, "at 0x%08llX\n",
		(unsigned long long)mem->start);
	return master;

map_failed:
	release_mem_region(mem->start, resource_size(mem));
put_master:
	spi_master_put(master);
	return NULL;
}
EXPORT_SYMBOL(zpuino_spi_init);

void zpuino_spi_deinit(struct spi_master *master)
{
	struct zpuino_spi *zspi;

	zspi = spi_master_get_devdata(master);

	spi_bitbang_stop(&zspi->bitbang);
	free_irq(zspi->irq, zspi);
	iounmap(zspi->regs);

	release_mem_region(zspi->mem.start, resource_size(&zspi->mem));
	spi_master_put(zspi->bitbang.master);
}
EXPORT_SYMBOL(zpuino_spi_deinit);

static int __devinit zpuino_spi_probe(struct platform_device *dev)
{
	struct zpuino_spi_platform_data *pdata;
	struct resource *r;
	struct spi_master *master;
	u8 i;
	int status;

	pdata = dev->dev.platform_data;

	status = gpio_request(pdata->gpio_cs, "gpio-spi-cs");

	if (status)
		return status;

	printk(KERN_INFO "ZPUino: probing for SPI controller\n");

	/*
	if (!num_cs) {
		dev_err(&dev->dev, "Missing slave select configuration data\n");
		return -EINVAL;
	}
       */

	r = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!r) {
        gpio_free(pdata->gpio_cs);
		return -ENODEV;
	}

	master = zpuino_spi_init(&dev->dev, r, dev->id, 1, 1, 8, pdata->gpio_cs);
	if (!master)
		return -ENODEV;

	/*
		if (pdata) {
		for (i = 0; i < pdata->num_devices; i++)
			spi_new_device(master, pdata->devices + i);
			}*/

	// spi_new_device(master, pdata->devices);

	platform_set_drvdata(dev, master);
	printk(KERN_INFO "ZPUino. SPI controller initialized %p\n",master);
	return 0;
}

static int __devexit zpuino_spi_remove(struct platform_device *dev)
{
	zpuino_spi_deinit(platform_get_drvdata(dev));
	platform_set_drvdata(dev, 0);

	return 0;
}

static struct platform_driver zpuino_spi_driver = {
	.probe = zpuino_spi_probe,
	.remove = __devexit_p(zpuino_spi_remove),
	.driver = {
		.name = "zpuino_spi",
		.owner = THIS_MODULE,
	},
};

static int __init init_zpuino_spi(void)
{
	printk(KERN_INFO "Registering ZPUino SPI driver\n");

	platform_driver_register(&zpuino_spi_driver);


    return 0;
}

static void __exit exit_zpuino_spi(void)
{
	platform_driver_unregister(&zpuino_spi_driver);
}

module_init(init_zpuino_spi);
module_exit(exit_zpuino_spi);

MODULE_AUTHOR("Alvaro Lopes <alvieboy@alvie.com>");
MODULE_DESCRIPTION("ZPUino SPI driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:zpuino_spi");
