// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      irq.h
//      Author :    bmarty <bmarty@mailo.com>
//      Purpose :   Periodic interrupt tick to the 65C02 (F-60). The host pulses IRQB
//                  (GPIO25 on the board) at the requested rate ; the 65C02 takes the
//                  interrupt through $FFFE/$FFFF (RAM : the program installs its own
//                  handler and CLIs). No acknowledge is needed (short pulse).
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _IRQ_H
#define _IRQ_H

#define IRQ_TICK_MAX_HZ 	(1000)

uint8_t IRQSetTick(uint16_t hz);  												// 0 = off ; returns 0 if ok, 1 if rate not supported
uint16_t IRQGetTick(void);
void HWIRQSetTick(uint16_t hz);  												// Implementation specific.

#endif
