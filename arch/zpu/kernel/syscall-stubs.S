/*
 * Stubs for syscalls that require access to pt_regs or that take more
 * than five parameters.
 */
  	.macro SYSCALL1 address
        	pushsp /* This will be pt_regs* */
        	im 28
        	add
        	storesp 8
                impcrel \address
                poppcrel
        .endm
 
  	.macro SYSCALL2 address
        	pushsp /* This will be pt_regs* */
        	im 28
        	add
        	storesp 12
                impcrel \address
                poppcrel
        .endm

 	.macro SYSCALL3 address
        	pushsp /* This will be pt_regs* */
        	im 28
        	add
        	storesp 16
                impcrel \address
                poppcrel
        .endm

 	.macro SYSCALL4 address
        	pushsp /* This will be pt_regs* */
        	im 28
        	add
        	storesp 20
                impcrel \address
                poppcrel
        .endm

 	.macro SYSCALL5 address
        	pushsp /* This will be pt_regs* */
        	im 28
        	add
        	storesp 24
                impcrel \address
                poppcrel
        .endm

	.text
	.global __sys_rt_sigsuspend
	.type	__sys_rt_sigsuspend,@function
__sys_rt_sigsuspend:
	;mov	r10, sp
	;rjmp	sys_rt_sigsuspend

	im sys_rt_sigsuspend
    	breakpoint
        poppc

	.global	__sys_sigaltstack
	.type	__sys_sigaltstack,@function
__sys_sigaltstack:
	SYSCALL3(sys_sigaltstack)

	.global	__sys_rt_sigreturn
	.type	__sys_rt_sigreturn,@function
__sys_rt_sigreturn:

        SYSCALL1(sys_rt_sigreturn)


	.global	__sys_fork
	.type	__sys_fork,@function
__sys_fork:
        SYSCALL1(sys_fork)

	.global	__sys_clone
	.type	__sys_clone,@function

__sys_clone:
        SYSCALL5(sys_clone)
        poppc

	.global	__sys_vfork
	.type	__sys_vfork,@function
__sys_vfork:
	SYSCALL1(sys_vfork)

	.global	__sys_execve
	.type	__sys_execve,@function
__sys_execve:
	SYSCALL4(sys_execve)

