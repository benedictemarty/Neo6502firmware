// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      irq.cpp
//      Author :    bmarty <bmarty@mailo.com>
//      Purpose :   Periodic interrupt tick to the 65C02 (F-60), common part.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

static uint16_t tickHz = 0;

uint8_t IRQSetTick(uint16_t hz) {
	if (hz > IRQ_TICK_MAX_HZ) return 1;
	tickHz = hz;
	HWIRQSetTick(hz);
	return 0;
}

uint16_t IRQGetTick(void) {
	return tickHz;
}
