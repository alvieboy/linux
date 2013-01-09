/*
 * Copyright (C) 2012 Alvaro Lopes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/init.h>
#include <linux/initrd.h>
#include <linux/sched.h>
#include <linux/console.h>
#include <linux/ioport.h>
#include <linux/bootmem.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/pfn.h>
#include <linux/root_dev.h>
#include <linux/cpu.h>
#include <linux/kernel.h>

#include <asm/sections.h>
#include <asm/processor.h>
#include <asm/pgtable.h>
#include <asm/setup.h>
#include <asm/sysreg.h>

#include <mach/board.h>
#include <mach/init.h>
#include <linux/personality.h>

extern int root_mountflags;


static void __init zpu_console_write(struct console *con, const char *s, unsigned n) {
	while (n--) {
		if (*s=='\n') {
			while ((UARTCTL&0x2)==2);
			UARTDATA='\r';
		}
		while ((UARTCTL&0x2)==2);
		UARTDATA=*s;
		s++;
	}
}

static struct console zpu_early_console = {
	.name =		"earlyconsole",
	.write =	zpu_console_write,
	.flags =	CON_PRINTBUFFER , /* | CON_BOOT */
	.index =	-1,
};

static char __initdata command_line[COMMAND_LINE_SIZE];

/*
 * Standard memory resources
 */
static struct resource __initdata kernel_data = {
	.name	= "Kernel data",
	.start	= 0,
	.end	= 0,
	.flags	= IORESOURCE_MEM,
};
static struct resource __initdata kernel_code = {
	.name	= "Kernel code",
	.start	= 0,
	.end	= 0,
	.flags	= IORESOURCE_MEM,
	.sibling = &kernel_data,
};

/*
 * Available system RAM and reserved regions as singly linked
 * lists. These lists are traversed using the sibling pointer in
 * struct resource and are kept sorted at all times.
 */
static struct resource *__initdata system_ram;
static struct resource *__initdata reserved = &kernel_code;

/*
 * We need to allocate these before the bootmem allocator is up and
 * running, so we need this "cache". 32 entries are probably enough
 * for all but the most insanely complex systems.
 */
static struct resource __initdata res_cache[32];
static unsigned int __initdata res_cache_next_free;

static void __init resource_init(void)
{
	struct resource *mem, *res;
	struct resource *new;

	kernel_code.start = __pa(init_mm.start_code);

	for (mem = system_ram; mem; mem = mem->sibling) {
		new = alloc_bootmem_low(sizeof(struct resource));
		memcpy(new, mem, sizeof(struct resource));

		new->sibling = NULL;
		
		if (request_resource(&iomem_resource, new))
			printk(KERN_WARNING "Bad RAM resource %08x-%08x\n",
			       mem->start, mem->end);
	}

	for (res = reserved; res; res = res->sibling) {
		new = alloc_bootmem_low(sizeof(struct resource));
		memcpy(new, res, sizeof(struct resource));

		new->sibling = NULL;
		
		if (insert_resource(&iomem_resource, new))
			printk(KERN_WARNING
			       "Bad reserved resource %s (%08x-%08x)\n",
			       res->name, res->start, res->end);
	}
}

static void __init
add_physical_memory(resource_size_t start, resource_size_t end)
{
	struct resource *new, *next, **pprev;

	for (pprev = &system_ram, next = system_ram; next;
	     pprev = &next->sibling, next = next->sibling) {
		if (end < next->start)
			break;
		if (start <= next->end) {
			printk(KERN_WARNING
			       "Warning: Physical memory map is broken\n");
			printk(KERN_WARNING
			       "Warning: %08x-%08x overlaps %08x-%08x\n",
			       start, end, next->start, next->end);
			return;
		}
	}

	if (res_cache_next_free >= ARRAY_SIZE(res_cache)) {
		printk(KERN_WARNING
		       "Warning: Failed to add physical memory %08x-%08x\n",
		       start, end);
		return;
	}

	new = &res_cache[res_cache_next_free++];
	new->start = start;
	new->end = end;
	new->name = "System RAM";
	new->flags = IORESOURCE_MEM;

	*pprev = new;
}

static int __init
add_reserved_region(resource_size_t start, resource_size_t end,
		    const char *name)
{
	struct resource *new, *next, **pprev;

	if (end < start)
		return -EINVAL;

	if (res_cache_next_free >= ARRAY_SIZE(res_cache))
		return -ENOMEM;

	for (pprev = &reserved, next = reserved; next;
	     pprev = &next->sibling, next = next->sibling) {
		if (end < next->start)
			break;
		if (start <= next->end)
			return -EBUSY;
	}

	new = &res_cache[res_cache_next_free++];
	new->start = start;
	new->end = end;
	new->name = name;
	new->sibling = next;
	new->flags = IORESOURCE_MEM;

	*pprev = new;

	return 0;
}

static unsigned long __init
find_free_region(const struct resource *mem, resource_size_t size,
		 resource_size_t align)
{
	struct resource *res;
	unsigned long target;

	target = ALIGN(mem->start, align);
	for (res = reserved; res; res = res->sibling) {
		if ((target + size) <= res->start)
			break;
		if (target <= res->end)
			target = ALIGN(res->end + 1, align);
	}

	if ((target + size) > (mem->end + 1))
		return mem->end + 1;

	return target;
}

static int __init
alloc_reserved_region(resource_size_t *start, resource_size_t size,
		      resource_size_t align, const char *name)
{
	struct resource *mem;
	resource_size_t target;
	int ret;

	for (mem = system_ram; mem; mem = mem->sibling) {
		target = find_free_region(mem, size, align);
		if (target <= mem->end) {
			ret = add_reserved_region(target, target + size - 1,
						  name);
			if (!ret)
				*start = target;
			return ret;
		}
	}

	return -ENOMEM;
}

/*
 * Pick out the memory size.  We look for mem=size@start,
 * where start and size are "size[KkMmGg]"
 */
static int __init early_mem(char *p)
{
	resource_size_t size, start;

	start = system_ram->start;
	size  = memparse(p, &p);
	if (*p == '@')
		start = memparse(p + 1, &p);

	system_ram->start = start;
	system_ram->end = system_ram->start + size - 1;
	return 0;
}
early_param("mem", early_mem);

/*
 * Find a free memory region large enough for storing the
 * bootmem bitmap.
 */
static unsigned long __init
find_bootmap_pfn(const struct resource *mem)
{
	unsigned long bootmap_pages, bootmap_len;
	unsigned long node_pages = PFN_UP(resource_size(mem));
	unsigned long bootmap_start;

	bootmap_pages = bootmem_bootmap_pages(node_pages);
	bootmap_len = bootmap_pages << PAGE_SHIFT;

	/*
	 * Find a large enough region without reserved pages for
	 * storing the bootmem bitmap. We can take advantage of the
	 * fact that all lists have been sorted.
	 *
	 * We have to check that we don't collide with any reserved
	 * regions, which includes the kernel image and any RAMDISK
	 * images.
	 */
	bootmap_start = find_free_region(mem, bootmap_len, PAGE_SIZE);

	return bootmap_start >> PAGE_SHIFT;
}

#define MAX_LOWMEM	HIGHMEM_START
#define MAX_LOWMEM_PFN	PFN_DOWN(MAX_LOWMEM)

static void __init setup_bootmem(void)
{
	unsigned bootmap_size;
	unsigned long first_pfn, bootmap_pfn, pages;
	unsigned long max_pfn, max_low_pfn;
	unsigned node = 0;
	struct resource *res;

	printk(KERN_INFO "Physical memory:\n");
	for (res = system_ram; res; res = res->sibling)
		printk("  %08x-%08x\n", res->start, res->end);
	printk(KERN_INFO "Reserved memory:\n");
	for (res = reserved; res; res = res->sibling)
		printk("  %08x-%08x: %s\n",
		       res->start, res->end, res->name);

	nodes_clear(node_online_map);

	for (res = system_ram; res; res = NULL) {
		first_pfn = PFN_UP(res->start);
		max_low_pfn = max_pfn = PFN_DOWN(res->end + 1);
		bootmap_pfn = find_bootmap_pfn(res);
		if (bootmap_pfn > max_pfn)
			panic("No space for bootmem bitmap!\n");

		if (max_low_pfn > MAX_LOWMEM_PFN) {
			max_low_pfn = MAX_LOWMEM_PFN;
#ifdef CONFIG_HIGHMEM
#error HIGHMEM is not supported by ZPUino yet
#endif
		}

		/* Initialize the boot-time allocator with low memory only. */
		bootmap_size = init_bootmem_node(NODE_DATA(node), bootmap_pfn,
						 first_pfn, max_low_pfn);

		/*
		 * Register fully available RAM pages with the bootmem
		 * allocator.
		 */
		pages = max_low_pfn - first_pfn;
		free_bootmem_node (NODE_DATA(node), PFN_PHYS(first_pfn),
				   PFN_PHYS(pages));

		/* Reserve space for the bootmem bitmap... */
		reserve_bootmem_node(NODE_DATA(node),
				     PFN_PHYS(bootmap_pfn),
				     bootmap_size,
				     BOOTMEM_DEFAULT);

		/* ...and any other reserved regions. */
		for (res = reserved; res; res = res->sibling) {
			if (res->start > PFN_PHYS(max_pfn))
				break;

			/*
			 * resource_init will complain about partial
			 * overlaps, so we'll just ignore such
			 * resources for now.
			 */
			if (res->start >= PFN_PHYS(first_pfn)
				&& res->end < PFN_PHYS(max_pfn))
				reserve_bootmem_node(NODE_DATA(node),
						     res->start,
						     resource_size(res),
						     BOOTMEM_DEFAULT);
		}
		node_set_online(node);
	}
}

static inline void EARLY_DEBUG(char c)
{
	while ((UARTCTL&0x2)==2);
	UARTDATA=c;
}

static void printnibble(unsigned int c)
{
	c&=0xf;
	if (c>9)
		EARLY_DEBUG(c+'a'-10);
	else
		EARLY_DEBUG(c+'0');
}

void printhexbyte(unsigned int c)
{
	printnibble(c>>4);
	printnibble(c);
}

void printhex(unsigned int c)
{
	printhexbyte(c>>24);
	printhexbyte(c>>16);
	printhexbyte(c>>8);
	printhexbyte(c);
}

EXPORT_SYMBOL(printhex)

void __zpu_debug_syscall(int num)
{
	EARLY_DEBUG('>');
	printhexbyte(num);
	EARLY_DEBUG('\r');
	EARLY_DEBUG('\n');
}
void __zpu_debug_syscall_leave(int retval)
{
	EARLY_DEBUG('<');
	printhex(retval);
	EARLY_DEBUG('\r');
	EARLY_DEBUG('\n');
}

void __init setup_arch (char **cmdline_p)
{
	struct clk *cpu_clk;
	init_mm.start_code = (unsigned long)_text;
	init_mm.end_code = (unsigned long)_etext;
	init_mm.end_data = (unsigned long)_edata;
	init_mm.brk = (unsigned long)_end;
	register_console(&zpu_early_console);
	/*
	 * Include .init section to make allocations easier. It will
	 * be removed before the resource is actually requested.
	 */
	kernel_code.start = __pa(__init_begin);
	kernel_code.end = __pa(init_mm.end_code - 1);
	kernel_data.start = __pa(init_mm.end_code);
	kernel_data.end = __pa(init_mm.brk - 1);

	//parse_tags(bootloader_tags);

	add_reserved_region(0x0, 0xfff, "Bootloader");
	add_physical_memory(0x0, 0x800000);
	setup_processor();
	setup_platform();
	setup_board();

	cpu_clk = clk_get(NULL, "cpu");
	if (IS_ERR(cpu_clk)) {
		printk(KERN_WARNING "Warning: Unable to get CPU clock\n");
	} else {
		unsigned long cpu_hz = clk_get_rate(cpu_clk);

		clk_enable(cpu_clk);

		printk("CPU: ZPUino Running at %lu.%03lu MHz.\n",
		       ((cpu_hz + 500) / 1000) / 1000,
		       ((cpu_hz + 500) / 1000) % 1000);
	}
	strlcpy(boot_command_line,"root=/dev/mmcblk0p2 rootwait console=tty0 console=ttySZ0", COMMAND_LINE_SIZE);
	strlcpy(command_line, boot_command_line, COMMAND_LINE_SIZE);
	*cmdline_p = command_line;
	parse_early_param();
	setup_bootmem();

#if defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif

	paging_init();
	resource_init();
	/* Fix thread info for init task */
	current->stack = current_thread_info();

	status_reg = PS_S;
}

