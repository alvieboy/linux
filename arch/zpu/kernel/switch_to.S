/*
 ZPU switch_to function.
 */

#include <asm/sysreg.h>
#include <asm/thread_info.h>
#include <asm/ptrace.h>
#include <asm/asm-offsets.h>
#include <mach/assembler.h>

.text
.global	__switch_to
.type	__switch_to, @function

	/* Switch thread context from "prev" to "next", returning "last" */
__switch_to:
	// Get "last", which is our current thread
	pushsp
	im ~(THREAD_SIZE-1)
	and
	load /* This is task struct */
	im _memreg
	store

	ARCH_DISABLE_INTERRUPTS

	/* Store current SP in prev thread area cpu context */
	pushsp
	im TSK_TS_KSP
	addsp 12
	store
	/* And store return IP */
	im .leave_switch
	loadsp 8
	im TSK_TS_PC
	add
	store

	/* Save new current */
	loadsp 8
	im current_save
	store

	/* Switch to new stack - but first put IP there, so we can poppc */
	im TSK_TS_KSP
	addsp 12
	load        /* Load new SP */
	im -4
	add         /* Some space so we can place PC there */
	im TSK_TS_PC
	addsp 16
	load
	loadsp 4
	store   /* Stored in new stack */
	popsp   /* and move to new stack */

	ARCH_ENABLE_INTERRUPTS

.leave_switch:
	nop
	poppc
	.size	__switch_to, . - __switch_to

