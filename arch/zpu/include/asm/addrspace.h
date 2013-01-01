/*
 (C) 2012 Alvaro Lopes
 */
#ifndef __ASM_ZPU_ADDRSPACE_H
#define __ASM_ZPU_ADDRSPACE_H

/* Returns the physical address of a PnSEG (n=1,2) address */
#define PHYSADDR(a)	(((unsigned long)(a)))
/*
 * Map an address to a certain privileged segment
 */
#define P1SEG 0
#define P2SEG 0
#define P3SEG 0
#define P4SEG 0

#define P1SEGADDR(a) ((__typeof__(a))(((unsigned long)(a) & 0x1fffffff) \
				      | P1SEG))
#define P2SEGADDR(a) ((__typeof__(a))(((unsigned long)(a) & 0x1fffffff) \
				      | P2SEG))
#define P3SEGADDR(a) ((__typeof__(a))(((unsigned long)(a) & 0x1fffffff) \
				      | P3SEG))
#define P4SEGADDR(a) ((__typeof__(a))(((unsigned long)(a) & 0x1fffffff) \
				      | P4SEG))


#endif /* __ASM_ZPU_ADDRSPACE_H */
