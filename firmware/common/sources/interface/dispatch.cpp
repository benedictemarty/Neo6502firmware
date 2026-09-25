// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      dispatch.cpp
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      22nd November 2023
//      Reviewed :  No
//      Purpose :   Message dispatcher
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "data/prompt.h"
#include "interface/kbdcodes.h"
#include "interface/filesystem.h"

#define DCOMMAND    (cBlock+0)
#define DFUNCTION   (cBlock+1)
#define DERROR      (cBlock+2)
#define DSTATUS     (cBlock+3)
#define DPARAMS     (cBlock+4)

#define float_abs(n) (((n) < 0.0) ? -(n):(n))

#ifdef PICO
#define TIMECRITICAL(x) __time_critical_func(x)
#else
#define TIMECRITICAL(x) x
#endif

#include "data/neowho.h"

// bmarty R22 / T-12 : the toolbox groups (32.., ADR-01) are dispatched here, in flash. DSPHandler is copied to
// RAM (time critical) and grew by about 300 bytes of SRAM per group ; the toolbox calls are not time critical.
static void __attribute__((noinline)) DSPToolbox(uint8_t *cBlock, uint8_t *memory,uint8_t cmd) {
	(void)memory;
	#include "data/dispatch_toolbox.h"
}


// ***************************************************************************************
//
//							Handle commands sent by message
//
// ***************************************************************************************

void TIMECRITICAL(DSPHandler)(uint8_t *cBlock, uint8_t *memory) 
{
	float f1,f2;
	int i1,i2,r;
	uint32_t u1;
	uint8_t u2,u4,cmd;
	char *s;
	uint16_t u3;
	SOUND_UPDATE su;
	bool b1;
	*DERROR = 0;                                                                // Clear error state.
	cmd = *DCOMMAND; 															// Get the command.
	*DCOMMAND = 0;					     										// Clear the message indicating completion.
																				// Sweet 16 needs this to call routines. Doesn't matter because
																				// it isn't actually synchronous.
	#include "data/dispatch_code.h"  
}

// ***************************************************************************************
//
//     	  Polling code. Called at low repeat rate. About every 64k cycles is right.
//
// ***************************************************************************************

//		T-46 : nothing here may live in flash. KBDSync is __time_critical_func (RAM) and
//		CONBlinkSync returns at once except twice a second ; RNDCursorUpdate, added here by
//		0.10.3, was in flash and ran in full ~95 times a second — it broke every program on
//		the board (bissection bmarty 2026-09-23). The cursor state is published when it
//		changes instead (mouse.cpp, cursor.cpp).
void TIMECRITICAL(DSPSync)(void) 
{
	KBDSync();
	HWBusProbe();  																// T-49 : bus stalls (RAM only, see above)
	if (!DBGScanTick()) {  															// T-75 : the pin scan owns the port for 30 s
		DBGPoll();  																// T-74 : terminal input
		DBGTelemetryTick();  														// T-73 : one line per second, if the port is on
		DBGFlush();  																// T-73 : push the trace, FIFO space only
	}
	CONBlinkSync();  															// Hercules blink attribute (F-52)
}

// ***************************************************************************************
//
//     	  		Reset interfaces. Called at start, and also on command 0,0
//
// ***************************************************************************************

// ***************************************************************************************
//
//		Boot sequence (ADR-0001) : P0 hardware (no external I/O), P1 USB discovery up to the
//		settle barrier, P2 policy (settings, storage catalogue, menu), P3 hand over. The
//		6502 starts after this function returns (main.cpp).
//
// ***************************************************************************************

static void DSPResetP0(void) {  												// P0 : hardware and firmware state only
	DBGInitialise();  															// T-73 : debug port first, so it can report P0 itself
	CONSetDebugEcho(2);  														// T-74 : and the console mirrors to it (2,20), Latin-1 included
	MEMInitialiseMemory();                                                      // Set up memory, load kernel ROM
	MSEInitialise();  															// Mouse first, before starting graphics.
	CURInitialise();
	GFXSetMode(0);                                                              // Initialise graphics
	SPRReset();                                                                 // Reset sprites.
	QDInitGraf();                                                               // Toolbox (T-12) : QuickDraw port
	EVTReset();                                                                 // Event manager off
	WMReset();                                                                  // No windows
	MNReset();                                                                  // No menus
	CTReset();                                                                  // No controls
	DLReset();                                                                  // No dialogs
	RSReset();                                                                  // No resource file
	BNKReset();                                                                 // No bank mapped (T-17)
	CONResetUserFont();                                                         // Latin-1 letters in $C0-$FF (T-20)
	IRQSetTick(0);                                                              // No interrupt tick, no frame IRQ (T-14)
	IRQSetFrame(0);
	LOGDrawLogo();                                                              // Draw logo
	CONSetQuiet(true);                                                          // T-66 : and nothing over it until P2
	SNDInitialise();                                                            // Initialise sound hardware
	SNDManager();                                                               // Initialise sound manager
}

//		T-66 : the whole of P1 is silent. The logos stay on screen while the bus is discovered,
//		and the text only starts in P2 — which is what bmarty asked for : logos, a pause, then
//		everything else. The pause costs nothing : the barrier was already waiting here.
static void DSPResetP1(void) {  												// P1 : USB discovery, up to the settle barrier
	KBDInitialise();                                                            // Start the USB host stack
	KBDEvent(0,0xFF,0);                                                         // Reset the keyboard manager
	USBWaitSettled();                                                           // T-32 : quiet bus, or the ceiling
}

static void DSPResetP2(void) {  												// P2 : policy (no hardware init here)
	const char bootString[] = PROMPT;
	CONSetQuiet(false);                                                         // T-66 : the text starts here
	CONWrite(0x80+3);                                                           // Yellow text
	for (int i = 0;i < 19;i++) CONWrite(19);
	const char *c = bootString;
	while (*c != '\0') CONWrite(*c++);
	CONWrite(0x80+6);
	USBReport();                                                                // What the silent phase found
	STOSynchronise();                                                           // Report what was mounted
	TZLoadFromStorage();                                                        // Time zone from the settings sector (T-26)
	BOOTSelect();                                                               // Boot menu from boot/
}

void DSPReset(void) {
	DSPResetP0();
	DSPResetP1();
	DSPResetP2();
	CONWrite(0x80+2);
	IOInitialise();  															// P3 : UEXT, then the bus loop starts the 6502
}

// ***************************************************************************************
//
//                              Access helper functions
//
// ***************************************************************************************

static char szBuffer[81];  

char *DSPGetString(uint8_t *command,uint8_t paramOffset) {
	uint8_t *mem = cpuMemory+command[paramOffset]+(command[paramOffset+1]<<8);  // From here.
	uint8_t length = *mem < sizeof(szBuffer)-1 ? *mem : sizeof(szBuffer)-1;  	// Length to copy.
	memcpy(szBuffer,mem+1,length);                                              // Make ASCIIZ string
	szBuffer[length] = '\0';
	return szBuffer;
}

std::string DSPGetStdString(uint8_t *command,uint8_t paramOffset) {
	uint8_t *mem = cpuMemory+command[paramOffset]+(command[paramOffset+1]<<8);  // From here.
	return std::string((char*)mem+1, *mem);
}

void DSPSetStdString(uint8_t *command, uint8_t paramOffset, const std::string& value) {
	uint8_t *mem = cpuMemory+command[paramOffset]+(command[paramOffset+1]<<8);  // From here.
	*mem = std::min(*mem, (uint8_t)value.size());
	memcpy(mem+1, value.data(), *mem);
}

uint16_t DSPGetInt16(uint8_t *command,uint8_t paramOffset) {
	return command[paramOffset] + (command[paramOffset+1] << 8);
}

uint32_t DSPGetInt32(uint8_t *command,uint8_t paramOffset) {
    return DSPGetInt16(command, paramOffset) | (DSPGetInt16(command, paramOffset+2) << 16);
}

void DSPSetInt16(uint8_t *command,uint8_t paramOffset,uint16_t value) {
    command[paramOffset] = value;
    command[paramOffset+1] = value >> 8;
}

void DSPSetInt32(uint8_t *command,uint8_t paramOffset,uint32_t value) {
    DSPSetInt16(command, paramOffset, value);
    DSPSetInt16(command, paramOffset+2, value >> 16);
}

// ***************************************************************************************
//
//      Date        Revision
//      ====        ========
//		12-02-24 	Increased buffer size to screen size and truncated to avoid buffer overflow.
//
// ***************************************************************************************
