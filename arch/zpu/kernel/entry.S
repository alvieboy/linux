/*
 * ZPUino Linux
 *
 * Copyright (C) 2012 Alvaro Lopes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <asm/thread_info.h>
#include <asm/ptrace.h>
#include <asm/asm-offsets.h>
#include <asm/sysreg.h>
#include <asm/unistd.h>
#include <asm/asm.h>

#undef DEBUG_SYSCALLS

.text

.type ret_from_fork, @function
.globl ret_from_fork
ret_from_fork:
    im _memreg
    load
    impcrel schedule_tail
    callpcrel
    storesp 0
    /* Child PID is always 0 */
    im 0
    storesp 4
ret_from_syscall:
    RESTORE_ALL_REGS

    ARCH_DISABLE_INTERRUPTS

    im status_reg
    store

    /* Current save */
    pushsp
    im ~(THREAD_SIZE-1)
    and
    /* Compute our exit SP, so we can save it */

    pushsp
    im 8
    add

    loadsp 4
    /* Save KSP first */
    load /* task_struct */
    im TSK_TS_KSP
    add
    store

    im current_save
    store

    /* If SP is same, we cannot write here because
     we will overwrite our own SP saved in pt_regs. */
    pushsp
    loadsp 4
    eq
    impcrel .Lno_need_to_store_pc
    neqbranch

    loadsp 4 /* PC */
    loadsp 4 /* New SP */
    store
    popsp

    ARCH_ENABLE_INTERRUPTS
    poppc
.Lno_need_to_store_pc:
    storesp 0
    ARCH_ENABLE_INTERRUPTS
    poppc

.globl kernel_thread_helper
kernel_thread_helper:
/*
    im _memreg     // r0
    load          // Arg
    im _memreg+8  // r2
    load          // Exit point
    im _memreg+4  // r1
    load          // Function
    */

    im _memreg+4     // r1
    load            // Arg
    im _memreg+12    // r3
    load          // Exit point
    im _memreg+8   // r2
    load          // Function

    poppc
       
.globl cache_push
.type cache_push, @function
cache_push:
    im 4 /* Flush dcache, enable SRAM */
    nop
    im ZPU_IO_SLOT(4) + 16
    store
    im 2 /* Flush icache, enable SRAM */
    nop
    im ZPU_IO_SLOT(4) + 16
    store
    nop
    nop
    nop
    poppc
.size cache_push, . - cache_push

.type handle_interrupt, @function
.globl handle_interrupt
handle_interrupt:
        
    im status_reg
    load
    // Coming from userspace ?
    im 1
    and
    impcrel .Lhandle_int_from_kernel
    neqbranch

    /* Yes, from userspace. Change to kernel stack */

    /* @NOTE: we should perhaps not do things like this here */

    im current_save
    load /* This is thread_info */
    load /* Load task_struct */
    im TSK_TS_KSP
    add
    load /* Load proper KSP */

    /* Save PC ? */
    loadsp 4
    loadsp 4
    store
    im -4      /* Decrease SP */
    add
    /* Save original SP */
    pushsp
    im 4
    add
    loadsp 4
    store
    popsp      /* Jump into kernel stack ! */

    impcrel .Lnosavestack
    poppcrel
        
.Lhandle_int_from_kernel:
    pushsp
.Lnosavestack:
    im status_reg
    load

    im 1
    nop
    im status_reg
    store           /* Privileged mode */
    SAVE_ALL_REGS
    pushsp
    impcrel do_IRQ
    callpcrel
    storesp 0 // Get rid of pt_regs

ret_from_interrupt:
    RESTORE_ALL_REGS
    /* Check if we are returning to userspace */

    ARCH_DISABLE_INTERRUPTS
    loadsp 0
    im 1
    and
    impcrel .Lret_from_kernel
    neqbranch
        
    /* Return to userspace. Save current_save, and update kernel SP */

    pushsp
    im ~(THREAD_SIZE-1)
    and
    im current_save
    store
    /* @TODO - save KSP */
.Lret_from_kernel:
    im status_reg
    store
    popsp
    ARCH_ENABLE_INTERRUPTS
    poppc
.size handle_interrupt, . - handle_interrupt

.globl  kernel_execve
.type    kernel_execve, @function
kernel_execve:
    loadsp 12
    loadsp 12
    loadsp 12
    im __NR_execve
    syscall
    im 20
    pushsp
    add
    popsp
    poppc
.size kernel_execve, . - kernel_execve

.globl do_syscall
.type do_syscall, @function

do_syscall:
    /* At this point we have:
     SP0 -> ret PC
     SP4 -> Syscall number
     SP8 -> arg0
     SP12 -> arg1
     SP16 -> arg2
     SP20 -> arg3
     SP24 -> arg4
     SP28 -> arg5
     */
    ARCH_DISABLE_INTERRUPTS

    im status_reg
    load
    // Coming from userspace ?
    im 1
    and
    impcrel .Lhandle_syscall_from_kernel
    neqbranch

    /* Ok, we were called from userspace. */

    im current_save
    load /* This is thread_info */
    load /* This is task_struct */
    im TSK_TS_KSP
    add
    load /* This is KSP */

    /* Save PC ? */
    loadsp 4
    loadsp 4
    store
    im -4      /* Decrease SP */
    add
    /* Save original SP */
    pushsp
    im 4
    add
    loadsp 4
    store
    popsp      /* Jump into kernel stack ! */
        
    impcrel .Lsyscallnosavestack
    poppcrel


.Lhandle_syscall_from_kernel:        
    /* Create a PTREGS structure */
    pushsp
.Lsyscallnosavestack:
    im status_reg
    load
    im 1
    nop
    im status_reg
    store
    ARCH_ENABLE_INTERRUPTS

    SAVE_ALL_REGS
        
    /* Reorder arguments. This uses more stack, but we
     have no other easy way */

    im 28 
    addsp 24
    load      // arg5
    im 24
    addsp 28
    load      // arg4
    im 20
    addsp 32
    load      // arg3

    im 16
    addsp 36
    load      // arg2

    im 12
    addsp 40
    load      // arg1

    im 8
    addsp 44
    load      // arg0
        
    /* pt_regs will be appropriately loaded from
     within syscall wrappers, whenever needed */
        
    /* Syscall number */
    im 4
    addsp 48
    load
    /* Debug syscall */
#ifdef DEBUG_SYSCALLS
    loadsp 0
    impcrel __zpu_debug_syscall
    callpcrel
    storesp 0
#endif
    /* Multiply by 4 */
    addsp 0
    addsp 0
    im sys_call_table
    add
    load
    call
#ifdef DEBUG_SYSCALLS
    im _memreg
    load
    impcrel __zpu_debug_syscall_leave
    callpcrel
    storesp 0
#endif
ret_sc:
    /* Get rid of temporary C frame */
    im 28
    pushsp
    add
    popsp
    /* Save syscall return value */
    SAVE_REG 0
    storesp 4

    impcrel .Lcheck_work
    callpcrel

    impcrel ret_from_syscall
    poppcrel
.Lcheck_work:
    /* Check if we have work to do */
    /* We're still in kernel stack here */

    /* @TODO: skip this if we're returning to kernel mode */
    pushsp
    im ~(THREAD_SIZE-1)
    and
    im TI_flags
    add
    load
    im _TIF_WORK_MASK
    and
    loadsp 0
    impcrel .Lwork_to_do
    neqbranch
    storesp 0
    poppc

.Lwork_to_do:
    loadsp 0
    im _TIF_NEED_RESCHED
    and
    impcrel .Lneed_resched
    neqbranch
.Lcheck_signal:
    /* Check if a signal is pending */
    im _TIF_SIGPENDING
    and
    impcrel .Lneed_signal
    neqbranch

    poppc
.Lneed_resched:

    /* Need to resched */
    impcrel schedule
    callpcrel
    impcrel .Lcheck_signal
    poppcrel
    
.Lneed_signal:

    /* We need to deliver signal */
    im 1  // syscall
    nop
    im 0  // oldset
    pushsp
    im 12
    add   // pt_regs
    impcrel do_signal
    callpcrel
    storesp 0
    storesp 0
    storesp 0
    poppc
.size    do_syscall, . - do_syscall

.data

.globl memory_end
memory_end:
    .long 0x0800000

.globl bootloader_tags
bootloader_tags:
    .word 0

