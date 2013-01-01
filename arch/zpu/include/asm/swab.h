/*
 * ZPU byteswapping functions.
 */
#ifndef __ASM_ZPU_SWAB_H
#define __ASM_ZPU_SWAB_H

#include <linux/types.h>
#include <linux/compiler.h>


extern unsigned long __builtin_bswap_32(unsigned long x);
extern unsigned short __builtin_bswap_16(unsigned short x);

#if !(__GNUC__ == 4 && __GNUC_MINOR__ < 2)
static inline __attribute_const__ __u16 __arch_swab16(__u16 val)
{
	return ((val>>8)
		|((val&0x00ff)<<8));

}
#define __arch_swab16 __arch_swab16

static inline __attribute_const__ __u32 __arch_swab32(__u32 val)
{
	return ((val>>24)
		|((val&0x00ff0000)>>8)
		|((val&0x0000ff00)<<8)
		|((val&0x000000ff)<<24));
}
#define __arch_swab32 __arch_swab32
#endif

#endif /* __ASM_ZPU_SWAB_H */
