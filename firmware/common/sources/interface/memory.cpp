// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      memory.cpp
//      Authors :   Paul Robson (paul@robsons.org.uk)
//                  Rien Matthijsse
//      Date :      20th November 2023
//      Reviewed :  No
//      Purpose :   Memory initialisation code.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "data/kernel_binary.h"                                            		// Contains kernel image.
#include "data/neodos_binary.h" 													// NeoDOS (Trinity 0.4.0 : resident environment)

#ifdef PICO
/* _Alignas(MEMORY_SIZE) */ uint8_t cpuMemory[MEMORY_SIZE] = {0};  				// Processor memory, aligned for Pico
uint8_t gfxObjectMemory[GFX_MEMORY_SIZE] = {0}; 								// Graphics objects memory.
uint8_t graphicsMemory[MAXGRAPHICSMEMORY] = {0};								// RAM used for graphics and console text.
uint16_t consoleMemory[MAXCONSOLEMEMORY] = {0};  								// Console RAM

#else
uint8_t cpuMemory[MEMORY_SIZE] = {0};
uint8_t gfxObjectMemory[GFX_MEMORY_SIZE] = {0};
uint8_t graphicsMemory[MAXGRAPHICSMEMORY] = {0};
uint16_t consoleMemory[MAXCONSOLEMEMORY] = {0};  					
#endif

uint16_t controlPort = DEFAULT_PORT;       										// Control point.

// ***************************************************************************************
//
//                          Load a binary image into ROM space
//
// ***************************************************************************************

static void loadROM(const uint8_t *vROM, uint16_t startAddress, uint16_t romSize) {
	for (int i = 0;i < romSize;i++) {
		cpuMemory[i+startAddress] = vROM[i];
		#ifdef PICO
		if ((i & 0xFF) == 0) sleep_ms(2);  										// Why ?
		#endif
	}
}

// ***************************************************************************************
//
//                     Initialise CPU memory (call once on boot up)
//
// ***************************************************************************************

void MEMInitialiseMemory(void) {
	loadROM(kernel_bin,KERNEL_LOAD,KERNEL_SIZE);    							// Load in the kernel
	loadROM(neodos_bin,NEODOS_LOAD,NEODOS_SIZE);  								// Load in NeoDOS to run by default
	cpuMemory[DEFAULT_PORT] = 0x00;               								// Clear the default command port
}

// ***************************************************************************************
//
//                     		Load the resident environment (1,3)
//
// ***************************************************************************************

// Trinity 0.4.0 (bmarty) : the firmware no longer embeds NeoBASIC. 1,3 "Load BASIC" loads the resident
// environment, NeoDOS (project Neo6502Msdos, image for $C000-$FBFF, neodos_binary.h), from the flash and points
// jmp (0) at it. The first 1,3 after reset applies the boot/ menu choice instead (bootmenu.cpp) ; NeoBASIC is
// started this way, as boot/neobasic.bin (image for $800).

void MEMLoadBasic(void) {
	if (BOOTLoadChoice()) return;  												// Trinity boot menu : chosen program (first 1,3 only)
	loadROM(neodos_bin,NEODOS_LOAD,NEODOS_SIZE);  								// The embedded NeoDOS image
	cpuMemory[0x0] = NEODOS_LOAD & 0xFF;  										// Start with jmp (0)
	cpuMemory[0x1] = NEODOS_LOAD >> 8;
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//		16-01-24 	Cleared memory on restart (not loading graphics library could crash)
//
// ***************************************************************************************
