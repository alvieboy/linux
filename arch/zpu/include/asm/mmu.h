#ifndef __ASM_ZPU_MMU_H
#define __ASM_ZPU_MMU_H

typedef struct {
	struct vm_list_struct	*vmlist;
	unsigned long		end_brk;
} mm_context_t;

#endif /* __ASM_ZPU_MMU_H */
