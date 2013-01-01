/*
 * ZPU System Registers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ASM_ZPU_SYSREG_H
#define __ASM_ZPU_SYSREG_H

#define ZPU_IOBASE 0x08000000
#define ZPU_IO_SLOT_OFFSET_BIT 23

#define ZPU_IO_SLOT(x) (ZPU_IOBASE + (x<<ZPU_IO_SLOT_OFFSET_BIT))

#define ZPU_REGISTER(SLOT, y) *(volatile unsigned int*)(SLOT + (y<<2))


#define INTRBASE   ZPU_IO_SLOT(4)
#define TIMERSBASE ZPU_IO_SLOT(3)
#define INTRCTL    ZPU_REGISTER(INTRBASE,0)
#define INTRMASK   ZPU_REGISTER(INTRBASE,1)
#define INTRLINE   ZPU_REGISTER(INTRBASE,2)
#define INTRLEVEL  ZPU_REGISTER(INTRBASE,3)

#define ROFF_UARTDATA   0
#define ROFF_UARTCTL    1
#define ROFF_UARTSTATUS 1
#define ROFF_UARTINTCTL 2

#define UARTBASE ZPU_IO_SLOT(1)
#define UARTDATA    ZPU_REGISTER(UARTBASE,ROFF_UARTDATA)
#define UARTCTL     ZPU_REGISTER(UARTBASE,ROFF_UARTCTL)
#define UARTSTATUS  ZPU_REGISTER(UARTBASE,ROFF_UARTSTATUS)
#define UARTINTCTL  ZPU_REGISTER(UARTBASE,ROFF_UARTSTATUS)

#define ZPU_TMR0CTL  ZPU_REGISTER(TIMERSBASE,0)
#define ZPU_TMR0CNT  ZPU_REGISTER(TIMERSBASE,1)
#define ZPU_TMR0CMP  ZPU_REGISTER(TIMERSBASE,2)
#define ZPU_TIMERTSC ZPU_REGISTER(TIMERSBASE,3)

#define TCTLENA 0 /* Timer Enable */
#define TCTLCCM 1 /* Clear on Compare Match */
#define TCTLDIR 2 /* Direction */
#define TCTLIEN 3 /* Interrupt enable */
#define TCTLCP0 4 /* Clock prescaler bit 0 */
#define TCTLCP1 5 /* Clock prescaler bit 1 */
#define TCTLCP2 6 /* Clock prescaler bit 2 */
#define TCTLIF  7 /* Interrupt flag */
#define TCTUPDP0 9 /* Update policy */
#define TCTUPDP1 10 /* Update policy */


#endif /* __ASM_ZPU_SYSREG_H */
