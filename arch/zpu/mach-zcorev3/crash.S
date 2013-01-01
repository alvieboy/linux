#include <asm/asm.h>

.text  
.globl zpu_crash
.type zpu_crash, @function
zpu_crash:
        ARCH_DISABLE_INTERRUPTS  
        pushsp
        im zpu_crash_impl
        call
        im -1
        poppcrel        
.size zpu_crash, . - zpu_crash

