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
#include "data/basic_binary.h" 													// NeoBasic

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
	loadROM(basic_bin,BASIC_LOAD,BASIC_SIZE);  									// Load in BASIC to run by default
	cpuMemory[DEFAULT_PORT] = 0x00;               								// Clear the default command port
}

// ***************************************************************************************
//
//                     			Load BASIC from Flash
//
// ***************************************************************************************

// Trinity (bmarty, Neo6502Basic) : the BASIC is decoupled from the firmware. If NEOBASIC.BIN exists at the
// root of the storage, it is loaded at $800 instead of the embedded copy (same layout as bin/basic.bin) ;
// otherwise the embedded BASIC is used. Returns true when the storage copy was loaded.

#define BASIC_STORAGE_FILE "neobasic.bin"

bool MEMLoadBasicFromStorage(void) {
	uint8_t exists = 0;
	if (FIOExistsFile(BASIC_STORAGE_FILE,&exists) != 0 || !exists) return false;
	if (FIOReadFileBasic(BASIC_STORAGE_FILE,BASIC_LOAD) != 0) {  					// Unreadable : back to the embedded one.
		loadROM(basic_bin,BASIC_LOAD,BASIC_SIZE);
		return false;
	}
	return true;
}

void MEMLoadBasic(void) {
	if (!MEMLoadBasicFromStorage()) loadROM(basic_bin,BASIC_LOAD,BASIC_SIZE);  	// Storage copy, else the embedded ROM image
	cpuMemory[0x0] = BASIC_LOAD & 0xFF;  										// Start with jmp (0)
	cpuMemory[0x1] = BASIC_LOAD >> 8;
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//		16-01-24 	Cleared memory on restart (not loading graphics library could crash)
//
// ***************************************************************************************
