/*
 */
#ifndef __ASM_ZPU_STRING_H
#define __ASM_ZPU_STRING_H

#define __HAVE_ARCH_MEMSET
extern void *memset(void *b, int c, size_t len);

#define __HAVE_ARCH_MEMCPY
extern void *memcpy(void *to, const void *from, size_t len);

#endif /* __ASM_ZPU_STRING_H */
