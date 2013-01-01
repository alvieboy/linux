/*
 * ZPUino Linux
 *
 * Copyright (C) 2012 Alvaro Lopes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/linkage.h>

#include <asm/page.h>
#include <asm/thread_info.h>
#include <asm/sysreg.h>

	.section .init.text,"ax"
	.global kernel_entry
	.global _start
.org 0x8
_start:
kernel_entry:
	/* Setup stack task */
	im init_thread_union
	nop
	im THREAD_SIZE - 8
	add
        
	popsp
	im start_kernel
	poppc
