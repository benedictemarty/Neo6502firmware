// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      tick.cpp
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      16th January 2024
//      Reviewed :  No
//      Purpose :   Initialise 50Hz tick callback
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "pico/time.h"
#include "system/wdc65C02cpu.h"

// ***************************************************************************************
//
//		Interrupt tick to the 65C02 (F-60) : a repeating hardware timer on this core
//		(core 0, the bus loop) pulls IRQB low, an alarm releases it IRQ_PULSE_US later.
//		The 65C02 bus is stalled by the PIO while these handlers run (as for DSPHandler).
//		NOT YET RUN ON A BOARD.
//
// ***************************************************************************************

//		IRQB is level sensitive : it is released when the 65C02 reads $FFFF (vector fetch
//		of the interrupt sequence, seen by the bus loop : irqAsserted), so the handler
//		needs no acknowledge and cannot re-enter. With I=1 the line stays low until CLI
//		(ticks coalesce, none is lost).

static repeating_timer_t tickTimer;
static bool tickActive = false;
volatile bool irqAsserted = false;  											// Read by the bus loop (processor_pio.cpp)

extern "C" bool IRQTickCallback(repeating_timer_t *rt) {  						// Global : Phosphoneo co-sim calls it in place of the (unmodelled) timer
	irqAsserted = true;
	wdc65C02cpu_set_irq(true);
	return true;
}

volatile bool frameIrqOn = false;  												// F-10 : read by the DVI scanline callback (core 1)

void HWIRQSetFrame(uint8_t on) {  												// F-10
	frameIrqOn = on != 0;
	if (!on && !tickActive) { irqAsserted = false;wdc65C02cpu_set_irq(false); }
}

void HWIRQSetTick(uint16_t hz) {
	if (tickActive) { cancel_repeating_timer(&tickTimer);tickActive = false; }
	if (!frameIrqOn) { irqAsserted = false;wdc65C02cpu_set_irq(false); }
	if (hz == 0) return;
	tickActive = add_repeating_timer_us(-(int64_t)(1000000 / hz), IRQTickCallback, NULL, &tickTimer);
}

struct repeating_timer timer;

// ***************************************************************************************
//
// 							50Hz Callback function
//
// ***************************************************************************************

static bool Tick50Callback(struct repeating_timer *t) {
	TICKProcess();
    return true;
}

// ***************************************************************************************
//
//								Initialise callback
//
// ***************************************************************************************

void THWStart(void) {
    add_repeating_timer_ms(20, Tick50Callback, NULL, &timer);
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
