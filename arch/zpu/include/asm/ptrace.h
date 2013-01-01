/*
 */
#ifndef __ASM_ZPU_PTRACE_H
#define __ASM_ZPU_PTRACE_H


#define PTREGS_SIZE 28

#ifndef __ASSEMBLY__
struct pt_regs {
	unsigned long r0;
	unsigned long r1;
	unsigned long r2;
	unsigned long r3;
	unsigned long status_extension; /* Status extension. Used to fake user mode */
	unsigned long sp;
	unsigned long pc;
};


extern unsigned int current_save;
extern unsigned int status_reg;

#endif


#ifndef PS_S
#define PS_S  (0x00000001)
#endif
#ifndef PS_T
#define PS_T  (0x00000002)
#endif

#define user_mode(regs) (!((regs)->status_extension & PS_S))
#define valid_user_regs(regs) user_mode(regs)
#define instruction_pointer(regs) ((regs)->pc)

#endif /* __ASM_ZPU_PTRACE_H */
