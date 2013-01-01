/*
 * Copyright (C) 2012 Alvaro Lopes
 */
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>

void __iomem *__ioremap(unsigned long phys_addr, size_t size,
			unsigned long flags)
{
	return (void __iomem*)phys_addr;
}
EXPORT_SYMBOL(__ioremap);

void __iounmap(void __iomem *addr)
{
}
EXPORT_SYMBOL(__iounmap);
