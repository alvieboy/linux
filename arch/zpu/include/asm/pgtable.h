/*
 */
#ifndef __ASM_ZPU_PGTABLE_H
#define __ASM_ZPU_PGTABLE_H

#include <asm/setup.h>
#include <asm/io.h>
#include <linux/sched.h>

typedef pte_t *pte_addr_t;

#define pgd_present(pgd)     	(1)       /* pages are always present on NO_MM */
#define pgd_none(pgd)		(0)
#define pgd_bad(pgd)		(0)
#define pgd_clear(pgdp)
#define kern_addr_valid(addr) 	(1)
#define	pmd_offset(a, b)	((void *)0)

#define PAGE_NONE		__pgprot(0)    /* these mean nothing to NO_MM */
#define PAGE_SHARED		__pgprot(0)    /* these mean nothing to NO_MM */
#define PAGE_COPY		__pgprot(0)    /* these mean nothing to NO_MM */
#define PAGE_READONLY		__pgprot(0)    /* these mean nothing to NO_MM */
#define PAGE_KERNEL		__pgprot(0)    /* these mean nothing to NO_MM */

extern void paging_init(void);
#define swapper_pg_dir ((pgd_t *) 0)

#define __swp_type(x)		(0)
#define __swp_offset(x)		(0)
#define __swp_entry(typ,off)	((swp_entry_t) { ((typ) | ((off) << 7)) })
#define __pte_to_swp_entry(pte)	((swp_entry_t) { pte_val(pte) })
#define __swp_entry_to_pte(x)	((pte_t) { (x).val })

static inline int pte_file(pte_t pte) { return 0; }

/*
 * ZERO_PAGE is a global shared page that is always zero: used
 * for zero-mapped memory areas etc..
 */
#define ZERO_PAGE(vaddr)	(virt_to_page(0))

extern unsigned int kobjsize(const void *objp);
extern int is_in_rom(unsigned long);

/*
 * No page table caches to initialise
 */
#define pgtable_cache_init()   do { } while (0)

#define io_remap_pfn_range(vma, vaddr, pfn, size, prot)		\
		remap_pfn_range(vma, vaddr, pfn, size, prot)

extern inline void flush_cache_mm(struct mm_struct *mm)
{
}

extern inline void flush_cache_range(struct mm_struct *mm,
				     unsigned long start,
				     unsigned long end)
{
}

/* Push the page at kernel virtual address and clear the icache */
extern inline void flush_page_to_ram (unsigned long address)
{
}

/* Push n pages at kernel virtual address and clear the icache */
extern inline void flush_pages_to_ram (unsigned long address, int n)
{
}

/*
 * All 32bit addresses are effectively valid for vmalloc...
 * Sort of meaningless for non-VM targets.
 */
#define	VMALLOC_START	0
#define	VMALLOC_END	0xffffffff

#define arch_start_context_switch(prev)     do {} while (0)

#define arch_enter_lazy_mmu_mode()	do {} while (0)
#define arch_leave_lazy_mmu_mode()	do {} while (0)
#define arch_flush_lazy_mmu_mode()	do {} while (0)
#define arch_enter_lazy_cpu_mode()	do {} while (0)
#define arch_leave_lazy_cpu_mode()	do {} while (0)
#define arch_flush_lazy_cpu_mode()	do {} while (0)

#endif /* __ASM_ZPU_PGTABLE_H */
