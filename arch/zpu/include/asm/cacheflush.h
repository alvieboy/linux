/*
 */
#ifndef __ASM_ZPU_CACHEFLUSH_H
#define __ASM_ZPU_CACHEFLUSH_H

/* Keep includes the same across arches.  */
#include <linux/mm.h>
#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 0

extern void cache_push (unsigned long vaddr, int len);
extern void dcache_push (unsigned long vaddr, int len);
extern void icache_push (unsigned long vaddr, int len);
extern void cache_push_all (void);
extern void cache_clear (unsigned long paddr, int len);

#define flush_cache_all()			__flush_cache_all()
#define flush_cache_mm(mm)			do { } while (0)
#define flush_cache_range(vma, start, end)	cache_push(start, end - start)
#define flush_cache_page(vma, vmaddr)		do { } while (0)
#define flush_dcache_range(start,end)		dcache_push(start, end - start)
#define flush_dcache_page(page)			do { } while (0)
#define flush_dcache_mmap_lock(mapping)		do { } while (0)
#define flush_dcache_mmap_unlock(mapping)	do { } while (0)
#define flush_icache_range(start,end)		cache_push(start, end - start)
#define flush_icache_page(vma,pg)		do { } while (0)
#define flush_icache_user_range(vma,pg,adr,len)	do { } while (0)

#define copy_to_user_page(vma, page, vaddr, dst, src, len) \
	memcpy(dst, src, len)
#define copy_from_user_page(vma, page, vaddr, dst, src, len) \
	memcpy(dst, src, len)


extern inline void __flush_cache_all(void)
{
	cache_push_all();
}

#endif /* __ASM_ZPU_CACHEFLUSH_H */
