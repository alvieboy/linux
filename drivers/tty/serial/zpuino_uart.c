/*
 *  Driver for ZPUINO UART
 *
 *  Copyright (C) 2012 Alvaro Lopes
 */

#include <linux/module.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/kthread.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/serial_core.h>
#include <asm/irq.h>
#include <asm/sysreg.h>

#define SERIAL_ZPUINOUART_MAJOR	TTY_MAJOR
#define SERIAL_ZPUINOUART_MINOR	64

/*#define UART_GET_STATUS(port) UARTSTATUS
#define UART_GET_CTRL(port) UARTCTL
#define UART_GET_DATA(port) UARTDATA
#define UART_SET_DATA(port) UARTDATA
*/
#define UART_STATUS_RXDATA 1
#define UART_STATUS_TXFULL 2
#define UART_STATUS_IN_TX 4
#define UART_STATUS_INTERRUPT_ENABLED 8

#define UART_NR 2

static int zpuino_zpuinouart_port_nr=0;


static inline unsigned int zpuino_uart_get_status(struct uart_port *port)
{
	//return ioread32((void* __iomem)(port->membase + 4));
	return ioread32((void* __iomem)(ZPU_IO_SLOT(1) + 4));
}

static inline unsigned int zpuino_uart_get_data(struct uart_port *port)
{
	//return ioread32((void* __iomem)port->membase);
	return ioread32((void* __iomem)ZPU_IO_SLOT(1));
}

static inline void zpuino_uart_put_data(struct uart_port *port,unsigned val)
{
	//iowrite32(val,(void* __iomem)port->membase);
	iowrite32(val,(void* __iomem)ZPU_IO_SLOT(1));
}

static inline void zpuino_uart_put_control(struct uart_port *port, unsigned val)
{
	//	iowrite32(val,(void* __iomem)(port->membase + 4));
	iowrite32(val,(void* __iomem)(ZPU_IO_SLOT(1) + 4));
}



static void zpuinouart_tx_chars(struct uart_port *port);

static void zpuinouart_start_tx(struct uart_port *port)
{
	if (!(zpuino_uart_get_status(port) & UART_STATUS_TXFULL))
		zpuinouart_tx_chars(port);
}

static void zpuinouart_enable_ms(struct uart_port *port)
{
}

static void zpuinouart_rx_chars(struct uart_port *port)
{
	struct tty_struct *tty = port->state->port.tty;
	unsigned int status, ch, rsr, flag;
	unsigned int max_chars = port->fifosize;

	status = zpuino_uart_get_status(port);

	while (status & UART_STATUS_RXDATA && (max_chars--)) {

		ch = zpuino_uart_get_data(port) & 0xff;
		flag = TTY_NORMAL;

		port->icount.rx++;
        /*
		if (uart_handle_sysrq_char(port, ch))
			goto ignore_char;
        */
		uart_insert_char(port, 0, 0, ch, flag);

		status = zpuino_uart_get_status(port);
	}

	tty_flip_buffer_push(tty);
}

static unsigned int zpuinouart_tx_empty(struct uart_port *port)
{
	return !(zpuino_uart_get_status(port) & UART_STATUS_TXFULL);
}

static void zpuinouart_wait_tx_free(struct uart_port *port)
{
	while (!zpuinouart_tx_empty(port))
		barrier();
}

#define UART_PUT_CHAR(port,char) do { \
	zpuinouart_wait_tx_free(port); \
	zpuino_uart_put_data(port, char); \
	} while(0)


static void zpuinouart_tx_chars(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;
	unsigned cnt=0;

	if (port->x_char) {
		UART_PUT_CHAR(port, port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		return;
	}

	if (uart_circ_empty(xmit)) {
		return;
	}
	/* @TODO: remove this */

	do {
		UART_PUT_CHAR(port, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		cnt++;
	} while (!uart_circ_empty(xmit));

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);
}

static unsigned int zpuinouart_get_mctrl(struct uart_port *port)
{
	return TIOCM_CAR | TIOCM_DSR | TIOCM_CTS;
}

static void zpuinouart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
}

static void zpuinouart_break_ctl(struct uart_port *port, int break_state)
{
}

static void zpuino_uart_set_interrupt(unsigned val)
{
    iowrite32(val,(void* __iomem)(ZPU_IO_SLOT(1) + 8));
}

static irqreturn_t zpuinouart_isr(int irq, void *data)
{
	struct uart_port *port = (struct uart_port *)data;

	zpuinouart_rx_chars(port);

	zpuino_uart_set_interrupt(1);

	return IRQ_HANDLED;
}

static int zpuinouart_startup(struct uart_port *port)
{
	int retval;
	retval = request_irq(port->irq, zpuinouart_isr, 0, "zpuino_uart",
						 (void *)port);
	if (retval)
		return retval;

	return 0;
}

static void zpuinouart_shutdown(struct uart_port *port)
{
	if (port->irq)
		free_irq(port->irq,port);
}

static void zpuinouart_set_termios(struct uart_port *port,
				struct ktermios *termios, struct ktermios *old)
{
	unsigned long flags;
	unsigned int baud, quot;

	return;
	/* Ask the core to calculate the divisor for us. */
	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk / 16);
	if (baud == 0)
		panic("invalid baudrate %i\n", port->uartclk / 16);

	quot = (uart_get_divisor(port, baud));

	spin_lock_irqsave(&port->lock, flags);

	/* Update the per-port timeout. */
	uart_update_timeout(port, termios->c_cflag, baud);

	/* Set baud rate */
	quot -= 1;
	zpuino_uart_put_control(port, (quot & 0xffff)| (1<<16));

	spin_unlock_irqrestore(&port->lock, flags);
}

static const char *zpuinouart_type(struct uart_port *port)
{
	return "ZPUino UART";
}

static void zpuinouart_release_port(struct uart_port *port)
{
	release_mem_region(port->membase, 0x16);
}

static int zpuinouart_request_port(struct uart_port *port)
{
	printk(KERN_INFO "Request port\n");

	return request_mem_region(port->membase, 0x16, "zpuinouart")
	    != NULL ? 0 : -EBUSY;
	return 0;
}

/* Configure/autoconfigure the port */
static void zpuinouart_config_port(struct uart_port *port, int flags)
{
	zpuinouart_request_port(port);
}

/* Verify the new serial_struct (for TIOCSSERIAL) */
static int zpuinouart_verify_port(struct uart_port *port,
			       struct serial_struct *ser)
{
    return 0;
}

static void zpuinouart_stop_tx(struct uart_port *port)
{
}

static void zpuinouart_stop_rx(struct uart_port *port)
{
}

static struct uart_ops zpuino_zpuinouart_ops = {
	.tx_empty = zpuinouart_tx_empty,
	.set_mctrl = zpuinouart_set_mctrl,
	.get_mctrl = zpuinouart_get_mctrl,
	.stop_tx = zpuinouart_stop_tx,
	.start_tx = zpuinouart_start_tx,
	.stop_rx = zpuinouart_stop_rx,
	.enable_ms = zpuinouart_enable_ms,
	.break_ctl = zpuinouart_break_ctl,
	.startup = zpuinouart_startup,
	.shutdown = zpuinouart_shutdown,
	.set_termios = zpuinouart_set_termios,
	.type = zpuinouart_type,
	.release_port = zpuinouart_release_port,
	.request_port = zpuinouart_request_port,
	.config_port = zpuinouart_config_port,
	.verify_port = zpuinouart_verify_port,
};

static struct uart_port zpuino_zpuinouart_ports[UART_NR];
static struct device_node *zpuino_zpuinouart_nodes[UART_NR];

static int zpuinouart_scan_fifo_size(struct uart_port *port, int portnumber)
{
    return 1;
}

static void zpuinouart_flush_fifo(struct uart_port *port)
{
}


/* ======================================================================== */
/* Console driver, if enabled                                               */
/* ======================================================================== */

#ifdef CONFIG_SERIAL_ZPUINO_ZPUINOUART_CONSOLE

static void zpuinouart_console_putchar(struct uart_port *port, int ch)
{
	UART_PUT_CHAR(port, ch);
}

static void
zpuinouart_console_write(struct console *co, const char *s, unsigned int count)
{
	struct uart_port *port = &zpuino_zpuinouart_ports[co->index];
	unsigned int status;
	uart_console_write(port, s, count, zpuinouart_console_putchar);
}

static void __init
zpuinouart_console_get_options(struct uart_port *port, int *baud,
			    int *parity, int *bits)
{
	*parity = 'n';
	*bits = 8;
	*baud = 9600;
}

static void __init zpuinouart_init_one_port(struct uart_port *port)
{
	port->mapbase = ZPU_IO_SLOT(1);
    port->membase = ZPU_IO_SLOT(1);

}

static int __init zpuinouart_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	if (co->index >= zpuino_zpuinouart_port_nr)
		co->index = 0;

	port = &zpuino_zpuinouart_ports[co->index];
	//printk(KERN_INFO "Port at %p\n", port);

	if (!port->membase) {
		//printk(KERN_INFO "No membase?????\n");
		return -ENXIO;//NODEV;
		//port->membase = ZPU_IO_SLOT(1);
	}

	spin_lock_init(&port->lock);

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	else
		zpuinouart_console_get_options(port, &baud, &parity, &bits);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver zpuino_zpuinouart_driver;

static struct console zpuino_zpuinouart_console = {
	.name = "ttySZ",
	.write = zpuinouart_console_write,
	.device = uart_console_device,
	.setup = zpuinouart_console_setup,
	.flags = CON_PRINTBUFFER,
	.index = -1,
	.data = &zpuino_zpuinouart_driver,
};


static int zpuino_zpuinouart_configure(void);

static int __init zpuinouart_console_init(void)
{

	//if (zpuino_zpuinouart_configure())
	//	return -ENODEV;

	//register_console(&zpuino_zpuinouart_console);
	//register_console(&zpuino_zpuinouart_console);

	return 0;
}

//console_initcall(zpuinouart_console_init);

#define ZPUINO_CONSOLE	(&zpuino_zpuinouart_console)
#else

#define ZPUINO_CONSOLE	NULL

#endif

static struct uart_driver zpuino_zpuinouart_driver = {
	.owner = THIS_MODULE,
	.driver_name = "zpuino_uart",
	.dev_name = "ttySZ",
	.major = SERIAL_ZPUINOUART_MAJOR,
	.minor = SERIAL_ZPUINOUART_MINOR,
	.nr = UART_NR,
	.cons = ZPUINO_CONSOLE,
};


static struct uart_port *zpuino_uart_get_port(void)
{
	struct uart_port *port;
	int id;

	/* Find the next unused port */
	for (id = 0; id < UART_NR; id++)
		if (zpuino_zpuinouart_ports[id].mapbase == 0)
			break;

	if (id >= UART_NR)
		return NULL;

	port = &zpuino_zpuinouart_ports[id];

    return port;
}


/* ======================================================================== */
/* OF Platform Driver                                                       */
/* ======================================================================== */

static int __devinit zpuinouart_probe(struct platform_device *dev)
{
	struct uart_port *port;
	struct resource *r;

	r = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!r)
		return -ENODEV;

    
	if (!request_mem_region(r->start, resource_size(r),
							"ZPUino-UART"))
		return -ENODEV;

	/* @TODO: request irq  line */

	port = zpuino_uart_get_port();

	if (NULL==port) {
        return -ENODEV;
	}

	port->membase = r->start;
	port->mapbase = r->start;
	port->line=0;
	port->dev = &dev->dev;
	port->uartclk = 960000000;
	port->fifosize = 2048;
	port->type = PORT_ZPUINO;
	port->iotype = SERIAL_IO_MEM;
	port->ops = &zpuino_zpuinouart_ops;



	port->irq = 1;

	uart_add_one_port(&zpuino_zpuinouart_driver, (struct uart_port *) port);

	zpuinouart_flush_fifo((struct uart_port *) port);

	printk(KERN_INFO "ZPUINO: UART at 0x%lx, irq %d\n",
	       (unsigned long) port->membase, port->irq);

	zpuino_uart_set_interrupt(1);

	return 0;
}

static struct platform_driver zpuino_uart_driver = {
	.probe = zpuinouart_probe,
	.driver = {
		.name = "zpuino_uart",
		.owner = THIS_MODULE,
	},
};


static int __init zpuino_zpuinouart_init(void)
{
	int ret;

	printk(KERN_INFO "Serial: ZPUINO UART driver\n");

	ret = uart_register_driver(&zpuino_zpuinouart_driver);
    
	if (ret) {
		printk(KERN_ERR "%s: uart_register_driver failed (%i)\n",
		       __FILE__, ret);
		return ret;
	}

	ret = platform_driver_register(&zpuino_uart_driver);

	if (ret != 0)
		uart_unregister_driver(&zpuino_zpuinouart_driver);

	return ret;
}

static void __exit zpuino_zpuinouart_exit(void)
{
	int i;

	for (i = 0; i < zpuino_zpuinouart_port_nr; i++)
		uart_remove_one_port(&zpuino_zpuinouart_driver,
				     &zpuino_zpuinouart_ports[i]);

	uart_unregister_driver(&zpuino_zpuinouart_driver);
}

module_init(zpuino_zpuinouart_init);
module_exit(zpuino_zpuinouart_exit);

MODULE_AUTHOR("Alvaro Lopes");
MODULE_DESCRIPTION("ZPUINO UART serial driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:zpuino_uart");
MODULE_LICENSE("GPL");
