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

uint16_t irqTickHz = 0;  														// Global : read by the Phosphoneo co-sim

uint8_t IRQSetTick(uint16_t hz) {
	if (hz > IRQ_TICK_MAX_HZ) return 1;
	irqTickHz = hz;
	HWIRQSetTick(hz);
	return 0;
}

uint16_t IRQGetTick(void) {
	return irqTickHz;
}

// ***************************************************************************************
//
//		F-10 : frame (vsync) interrupt : IRQB pulled low at the start of every frame of the
//		display, same delivery/release as the tick (read of $FFFF). Both may be on at once :
//		the handler cannot tell them apart (use 5,37 Frame Count / 1,1 Timer).
//
// ***************************************************************************************

static uint8_t irqFrameOn = 0;  												// Global : read by the Phosphoneo co-sim

uint8_t IRQSetFrame(uint8_t on) {
	irqFrameOn = on ? 1 : 0;
	HWIRQSetFrame(irqFrameOn);
	return 0;
}

uint8_t IRQGetFrame(void) {
	return irqFrameOn;
}
