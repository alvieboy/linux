#ifndef __ZPU_MACH_ZPUINO_ASSEMBLER_H__
#define __ZPU_MACH_ZPUINO_ASSEMBLER_H__

#ifdef __ASSEMBLY__

.macro ARCH_ENABLE_INTERRUPTS
	im 1
	nop
	im 0x0A000000
	store
.endm

.macro ARCH_DISABLE_INTERRUPTS
	im 0
	nop
	im 0x0A000000
	store
.endm

.macro SAVE_REG x
	im _memreg + (\x*4)
	load
.endm

.macro RESTORE_REG x
	im _memreg + (\x*4)
	store
.endm

.macro SAVE_ALL_REGS
	SAVE_REG 3
	SAVE_REG 2
	SAVE_REG 1
	SAVE_REG 0
.endm

.macro RESTORE_ALL_REGS
	RESTORE_REG 0
	RESTORE_REG 1
	RESTORE_REG 2
	RESTORE_REG 3
.endm

#endif /* __ASSEMBLY__ */

#endif /* __ZPU_MACH_ZPUINO_ASSEMBLER_H__ */
