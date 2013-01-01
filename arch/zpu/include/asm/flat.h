#ifndef _ASM_ZPU_FLAT_H__
#define _ASM_ZPU_FLAT_H__

#define flat_argvp_envp_on_stack()          1
#define flat_old_ram_flag(flags)			(flags)
#define flat_reloc_valid(reloc, size)			((reloc) <= (size))
#define flat_set_persistent(relval, p)			0
#define	flat_get_relocate_addr(rel)	(rel & 0x3fffffff)

#include <asm/unaligned.h>

static inline unsigned long
flat_get_addr_from_rp(unsigned long *rp, unsigned long relval,
			unsigned long flags, unsigned long *persistent)
{
	unsigned long addr;
	(void)flags;

	/* Is it a split 64/32 reference? */
	if (relval & 0x40000000) {
		addr = get_unaligned(rp);//flat_get_relocate_addr(get_unaligned(rp));
	} else {
		/* Get the address straight out */
		addr = get_unaligned(rp);
	}
#if 0
	printk(KERN_INFO "flat_get_addr_from_rp: rp %p, addr 0x%08x relval 0x%08x\n", rp,addr,relval);
#endif
	return addr;
}

static inline void
flat_put_addr_at_rp(unsigned long *rp, unsigned long addr, unsigned long relval)
{
#if 0
	printk(KERN_INFO "flat_put_addr_at_rp: rp %p, addr 0x%08x relval 0x%08x\n", rp,addr,relval);
#endif
	unsigned char *target;
	int i;
	/* IM ? */
	if (relval & 0x40000000) {
		//relval &= 0x3fffffff;
		target = (unsigned char *)rp;
		for (i=4;i>=0;i--) {
			if (addr) {
				target[i] = 0x80 | (addr&0x7f);
                addr>>=7;
			} else {
				target[i] = 0x0B; /* NOP */
			}
#if 0
			printk(KERN_INFO " > %p -> %u\n", &target[i], target[i]);
#endif
		}
	} else {
		/* Put it straight in, no messing around */
		put_unaligned(addr, rp);
	}
}


#endif
