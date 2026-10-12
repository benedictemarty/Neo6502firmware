// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      stacks.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      12th October 2026
//      Purpose :   T-132 : measured depth of the two core stacks (5,42 selectors 10 and 11)
//
//		Neither stack has a guard (study T-132). Core 0's stack goes down from 0x20042000 (top of
//		scratch_y) : the linker reserves 2 Ko, its only margin below is the other half of scratch_y,
//		then core 1's stack. Core 1's goes down from 0x20041000 (top of scratch_x), over the TMDS
//		encoders at the start of scratch_x. At boot, before core 1 starts, both regions are painted
//		with a pattern (core 0's up to just under its current stack pointer) ; the depth reached is
//		where the pattern was first overwritten. Read on demand (API context), never on core 1.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

#define STK_PATTERN 	(0x5AC3A53Cu)

extern "C" uint32_t __scratch_x_end__[],__StackOneTop[],__StackTop[];  			// SDK linker script symbols
#define CORE0_LOW 		((uint32_t *)0x20041000u)  									// Start of scratch_y

void STKPaint(void) {  																// main(), first thing : core 1 not started
	uint32_t sp;
	__asm volatile ("mov %0, sp" : "=r" (sp));
	for (uint32_t *p = CORE0_LOW;p < (uint32_t *)(sp - 64);p++) *p = STK_PATTERN;  	// Up to just under this frame
	for (uint32_t *p = __scratch_x_end__;p < __StackOneTop;p++) *p = STK_PATTERN;  	// Core 1 : after the encoders
}

static uint32_t _STKDepth(uint32_t *low,uint32_t *top) {  								// Bytes from the top to the first
	uint32_t *p = low;  																// word no longer holding the pattern
	while (p < top && *p == STK_PATTERN) p++;
	return (uint32_t)((uint8_t *)top - (uint8_t *)p);
}

uint32_t STKDepth(int core) {  														// 5,42 selector 10 (core 0), 11 (core 1)
	return (core == 0) ? _STKDepth(CORE0_LOW,__StackTop) : _STKDepth(__scratch_x_end__,__StackOneTop);
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//		12-10-26 	First version (T-132, study of the firmware's memory)
//
// ***************************************************************************************
