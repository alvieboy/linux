/*
 * Copyright (C) 2004-2006 Atmel Corporation
 * Copyright (C) 2012 Alvaro Lopes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/init.h>
#include <linux/mmzone.h>
#include <linux/module.h>
#include <linux/bootmem.h>
#include <linux/pagemap.h>
#include <linux/nodemask.h>
#include <asm/sections.h>
#include <asm/page.h>

void __init paging_init()
{
    int nid;
	for_each_online_node(nid) {
		pg_data_t *pgdat = NODE_DATA(nid);
		unsigned long zones_size[MAX_NR_ZONES];
		unsigned long low, start_pfn;

		start_pfn = pgdat->bdata->node_min_pfn;
		low = pgdat->bdata->node_low_pfn;

		memset(zones_size, 0, sizeof(zones_size));
		zones_size[ZONE_NORMAL] = low - start_pfn;

		printk("Node %u: start_pfn = 0x%lx, low = 0x%lx\n",
		       nid, start_pfn, low);

		free_area_init_node(nid, zones_size, start_pfn, NULL);

		printk("Node %u: mem_map starts at %p\n",
		       pgdat->node_id, pgdat->node_mem_map);
	}

	mem_map = NODE_DATA(0)->node_mem_map;
}

void __init mem_init(void)
{
	int codesize, reservedpages, datasize, initsize;
	int nid, i;

	reservedpages = 0;
	high_memory = NULL;

	/* this will put all low memory onto the freelists */
	for_each_online_node(nid) {
		pg_data_t *pgdat = NODE_DATA(nid);
		unsigned long node_pages = 0;
		void *node_high_memory;

		num_physpages += pgdat->node_present_pages;

		if (pgdat->node_spanned_pages != 0)
			node_pages = free_all_bootmem_node(pgdat);

		totalram_pages += node_pages;

		for (i = 0; i < node_pages; i++)
			if (PageReserved(pgdat->node_mem_map + i))
				reservedpages++;

		node_high_memory = (void *)((pgdat->node_start_pfn
					     + pgdat->node_spanned_pages)
					    << PAGE_SHIFT);
		if (node_high_memory > high_memory)
			high_memory = node_high_memory;
	}

	codesize = (unsigned long)_etext - (unsigned long)_text;
	datasize = (unsigned long)_edata - (unsigned long)_data;
	initsize = (unsigned long)__init_end - (unsigned long)__init_begin;

	printk ("Memory: %luk/%luk available (%dk kernel code, "
		"%dk reserved, %dk data, %dk init)\n",
		nr_free_pages() << (PAGE_SHIFT - 10),
		totalram_pages << (PAGE_SHIFT - 10),
		codesize >> 10,
		reservedpages << (PAGE_SHIFT - 10),
		datasize >> 10,
		initsize >> 10);
}

static inline void free_area(unsigned long addr, unsigned long end, char *s)
{
	unsigned int size = (end - addr) >> 10;

	for (; addr < end; addr += PAGE_SIZE) {
		struct page *page = virt_to_page(addr);
		ClearPageReserved(page);
		init_page_count(page);
		free_page(addr);
		totalram_pages++;
	}

	if (size && s)
		printk(KERN_INFO "Freeing %s memory: %dK (%lx - %lx)\n",
		       s, size, end - (size << 10), end);
}

void free_initmem(void)
{
	free_area((unsigned long)__init_begin, (unsigned long)__init_end,
		  "init");
}

#ifdef CONFIG_BLK_DEV_INITRD

void free_initrd_mem(unsigned long start, unsigned long end)
{
	free_area(start, end, "initrd");
}

#endif

