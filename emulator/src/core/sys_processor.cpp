// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		sys_processor.c
//		Purpose:	Processor Emulation.
//		Created:	22nd November 2023
//		Author:		Paul Robson (paul@robsons.org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <cstdint>
#include "sys_processor.h"
#include "sys_debug_system.h"
#include "hardware.h"
#include "common.h"
#include "interface/kbdcodes.h"

// *******************************************************************************************************************************
//
//														CPU / Memory
//
// *******************************************************************************************************************************

LONG32 cycles;																		// Cycle Count.
static int argumentCount;
static char **argumentList;
static bool useDebuggerKeys = false;  												// Use the debugger keys.
static bool traceMode = false;														// Dump each CPU instruction to stdout.

// Test automation (headless runs) : cycles:N shot:C:FILE text:C:FILE keys:C:TEXT  (see CPURunTestHooks)
static LONG32 totalCycles = 0;  													// Cycles since reset.
static LONG32 irqTickCycles = 0,irqTickNext = 0;  									// F-60 interrupt tick (cycles between ticks).
static bool irqPending = false;

void HWIRQSetTick(uint16_t hz) {
	irqTickCycles = (hz == 0) ? 0 : CYCLE_RATE / hz;
	irqTickNext = totalCycles + irqTickCycles;
	irqPending = false;
}
static LONG32 exitAtCycles = 0;  													// cycles:N  exit after N cycles.
struct TestHook { LONG32 at; char kind; char arg[512]; bool done; };
static TestHook testHooks[16];
static int testHookCount = 0;
static const char *typeText = NULL;  												// keys:C:TEXT autotype state.
static int typePos = -1;
static LONG32 typeNext = 0;
static bool typeDown = false;
static int typeCode = 0,typeMods = 0;

WORD16 CPUGetPC(void) {
	return CPUGetPC65();
}

// *******************************************************************************************************************************
//
//											   	Read and Write Functions
//
// *******************************************************************************************************************************

BYTE8 *CPUAccessMemory(void) {
	return cpuMemory;
}

BYTE8 Read(WORD16 address) {
	return cpuMemory[address];
}

void _Write(WORD16 address,BYTE8 data) { 
	 cpuMemory[address] = data;			
	 if (address == CONTROLPORT) {
	 	DSPHandler(cpuMemory+address,cpuMemory);
	 }
}

// *******************************************************************************************************************************
//
//													Remember Arguments
//
// *******************************************************************************************************************************

void CPUSaveArguments(int argc,char *argv[]) {
	argumentCount = argc;
	argumentList = argv;
}

// *******************************************************************************************************************************
//
//														Read Neo File
//
// *******************************************************************************************************************************

static FILE *fInputFile = NULL;

static uint8_t CPUReadByte(uint8_t *p) {
	return fgetc(fInputFile);
}

void CPUReadNeoFile(char *fileName) {
	uint8_t header[8];
	printf("Trying to read header for %s\n",fileName);	
	fInputFile = fopen(fileName,"rb"); 												// Open file.
	if (fInputFile == NULL) return;

	for (int i = 0;i < 8;i++) header[i] = CPUReadByte(NULL);  
	bool isExec = header[0] == 0x03 && header[1] == 0x4E &&  						// Look for the magic number
								header[2] == 0x45 && header[3] == 0x4F;
	printf("Header check %d\n",(int)isExec);
	if (!isExec) { fclose(fInputFile);return; }
	printf("Header found.\n");
	uint16_t execAddress = header[6] + (header[7] << 8); 							// Get the execute address
	printf("Execute from $%x\n",execAddress);
	bool processing = true; 
	int error = 0;
	while (processing && error == 0) {
		error = FIOReadBlock(CPUReadByte,NULL,&processing); 						// Read one block.
	}
	fclose(fInputFile);
	if (execAddress != 0xFFFF) {  													// Not the default $FFFF e.g. don't execute
		cpuMemory[0xFFFC] = execAddress & 0xFF;
		cpuMemory[0xFFFD] = execAddress >> 8;
	}
}

// *******************************************************************************************************************************
//
//														Reset the CPU
//
// *******************************************************************************************************************************

//#include "binary.h"

void CPUReset(void) {
	char command[128];

	for (int i = 1;i < argumentCount;i++) { 										// Look for loads.
		strcpy(command,argumentList[i]);  											// Copy command
		printf("[%s]\n",command);
		char *pos = strchr(command,'@'); 											// Look for splitting @
		if (pos != NULL) {
			int ch,address;
			unsigned char *p;
			*pos++ = '\0'; 															// Split it
			if (strcmp(pos,"page") == 0) { 											// Load to page.
				address = cpuMemory[0x820] + (cpuMemory[0x821] << 8);
			} else {
				if (sscanf(pos,"%x",&address) != 1)  								// Hex -> Decimal
							exit(fprintf(stderr,"Bad format %s",pos));
			}
			if (strcmp(command,"run") == 0) {  										// Arbitrary run address run@x
				printf("Run machine code from $%x\n",address);
				cpuMemory[0xFFFC] = address & 0xFF;  								// 6502 run.
				cpuMemory[0xFFFD] = address >> 8;
			} else {
				p = cpuMemory+address;				 								// Load here.	
				if (address == 0xFFFF) p = gfxObjectMemory;  						// Load to graphics memory
				printf("Load %s to %x\n",command,address);
				FILE *f = fopen(command,"rb");  									// Read file in and copy to RAM.
				if (f == NULL) exit(fprintf(stderr,"Bad file %s",command));
				while (ch = fgetc(f),ch >= 0) {
					*p++ = ch;
					address = (address+1) & 0xFFFF;
				}
				fclose(f);
			}
		} else {			
			if (strcmp(command,"cold") == 0) { 										// Cold boots from $800
				printf("Cold boot $800\n");
				cpuMemory[0xFFFC] = 0;cpuMemory[0xFFFD] = 8;
			}
			if (strcmp(command,"warm") == 0) { 										// Warm boots from $803
				printf("Warm boot $803\n");
				cpuMemory[0xFFFC] = 3;cpuMemory[0xFFFD] = 8;
			}
			if (strcmp(command,"exec") == 0) { 										// Warm boots from $806
				printf("Warm boot $806\n");
				cpuMemory[0xFFFC] = 6;cpuMemory[0xFFFD] = 8;
			}
			if (strcmp(command,"keys") == 0) { 										// Keys work properly.
				useDebuggerKeys = true;
			}
			if (strncmp(command,"path:",5) == 0) {  								// Set storage path
				HWSetDefaultPath(command+5);
			}
			if (strncmp(command,"cycles:",7) == 0) {  								// Exit after N cycles (headless tests)
				exitAtCycles = atol(command+7);
			}
			if ((strncmp(command,"shot:",5) == 0 || strncmp(command,"text:",5) == 0 || 	// Timed screenshot / console text / autotype
						strncmp(command,"keys:",5) == 0) && testHookCount < 16) {
				char *sep = strchr(command+5,':');
				if (sep != NULL) {
					*sep = '\0';
					testHooks[testHookCount].at = atol(command+5);
					testHooks[testHookCount].kind = command[0];
					strncpy(testHooks[testHookCount].arg,sep+1,sizeof(testHooks[0].arg)-1);
					testHooks[testHookCount].done = false;
					testHookCount++;
				}
			}
			if (strcmp(command,"trace") == 0) { 									// Dump every CPU instruction to stdout.
				traceMode = true;
			}
			if (strlen(command) > 4 && 												// Load .NEO file (case-insensitive
						strcasecmp(command+strlen(command)-4,".neo") == 0) {	// so FTD.NEO / Foo.Neo also match).
				CPUReadNeoFile(command);
			}
		}
	}
	HWReset();																		// Reset Hardware
	CPUReset6502();
}

// *******************************************************************************************************************************
//
//							When non-zero disables the debugger keys, requiring control
//
// *******************************************************************************************************************************

int CPUUseDebugKeys(void) {
	return useDebuggerKeys ? 1 : 0;
}

// *******************************************************************************************************************************
//
//												Execute a single instruction
//
// *******************************************************************************************************************************

// *******************************************************************************************************************************
//
//								Test automation hooks : timed screenshot, console text, autotype, exit
//
// *******************************************************************************************************************************

static int CPUAsciiToHID(char ch,int *mods) {
	static const char *shifted = ")!@#$%^&*(";
	static const char *plain  = "-=[]\\;'`,./";
	static const char *shift2 = "_+{}|:\"~<>?";
	*mods = 0;
	if (ch >= 'a' && ch <= 'z') return 0x04 + ch - 'a';
	if (ch >= 'A' && ch <= 'Z') { *mods = KEY_SHIFT;return 0x04 + ch - 'A'; }
	if (ch >= '1' && ch <= '9') return 0x1E + ch - '1';
	if (ch == '0') return 0x27;
	for (int i = 0;i < 10;i++) if (shifted[i] == ch) { *mods = KEY_SHIFT;return (i == 0) ? 0x27 : 0x1E + i - 1; }
	for (int i = 0;plain[i];i++) if (plain[i] == ch) return 0x2D + i;
	for (int i = 0;shift2[i];i++) if (shift2[i] == ch) { *mods = KEY_SHIFT;return 0x2D + i; }
	if (ch == '\n') return 0x28;
	if (ch == 27) return 0x29;
	if (ch == ' ') return 0x2C;
	if (ch == '\t') return 0x2B;
	if (ch == 8) return 0x2A;
	return 0;
}

static void CPUConsoleText(const char *fileName) {
	FILE *f = fopen(fileName,"w");
	if (f == NULL) return;
	for (int y = 0;y < gMode.yCSize;y++) {
		for (int x = 0;x < gMode.xCSize;x++) {
			int ch = gMode.consoleMemory[x + y * MAXCONSOLEWIDTH] & 0xFF;
			fputc((ch >= 32 && ch < 127) ? ch : (ch == 0 ? ' ' : '.'),f);
		}
		fputc('\n',f);
	}
	fclose(f);
}

static void CPURunTestHooks(void) {
	for (int i = 0;i < testHookCount;i++) {
		if (!testHooks[i].done && totalCycles >= testHooks[i].at) {
			testHooks[i].done = true;
			if (testHooks[i].kind == 's') RNDWriteScreenshot(testHooks[i].arg);
			if (testHooks[i].kind == 't') CPUConsoleText(testHooks[i].arg);
			if (testHooks[i].kind == 'k') { typeText = testHooks[i].arg;typePos = 0;typeNext = totalCycles; }
		}
	}
	if (typePos >= 0 && typeText[typePos] != '\0' && totalCycles >= typeNext) {  		// Autotype : press, 3 frames, release, 3 frames.
		if (!typeDown) {
			char ch = typeText[typePos];
			if (ch == '\\' && typeText[typePos+1] != '\0') { typePos++;ch = (typeText[typePos] == 'n') ? '\n' : typeText[typePos]; }
			typeCode = CPUAsciiToHID(ch,&typeMods);
			if (typeCode == 0) { typePos++;return; }
			KBDEvent(1,typeCode,typeMods);
			typeDown = true;
		} else {
			KBDEvent(0,typeCode,0);
			typeDown = false;typePos++;
		}
		typeNext = totalCycles + 3 * CYCLES_PER_FRAME;
	}
	if (exitAtCycles != 0 && totalCycles >= exitAtCycles) {
		for (int i = 0;i < testHookCount;i++) { 										// Flush hooks that were due at exit.
			if (!testHooks[i].done && testHooks[i].at >= exitAtCycles) {
				testHooks[i].done = true;
				if (testHooks[i].kind == 's') RNDWriteScreenshot(testHooks[i].arg);
				if (testHooks[i].kind == 't') CPUConsoleText(testHooks[i].arg);
			}
		}
		printf("cycles:%ld reached - exiting emulator\n",(long)exitAtCycles);
		CPUExit();
	}
}

BYTE8 CPUExecuteInstruction(void) {
	BYTE8 forceSync = 0;

	if (CPUGetPC() == 0xFFFF) {
		printf("Hit CPU $FFFF - exiting emulator\n");
		CPUExit();
		return FRAME_RATE;
	}

    if (traceMode) {
		char mem[10]; // "XX XX XX \0"
		char dasm[32];
		DBGXDumpMem(CPUGetPC(), DBGXInstructionSize65(CPUGetPC()), mem);
		DBGXDasm65(CPUGetPC(), dasm);
		printf("%04x  %-10s %s\n", CPUGetPC(), mem, dasm);
	}

	LONG32 before = cycles;
	forceSync = CPUExecute6502();
	totalCycles += cycles - before;
	CPURunTestHooks();
	if (irqTickCycles != 0 && totalCycles >= irqTickNext) {  						// F-60 : periodic IRQ (level : pending until taken).
		irqTickNext += irqTickCycles;
		irqPending = true;
	}
	if (irqPending && CPUTriggerIRQ()) irqPending = false;

	int cycleMax = CYCLES_PER_FRAME; 	
	if (cycles < cycleMax && forceSync == 0) return 0;								// Not completed a frame.
	cycles = 0;																		// Reset cycle counter.
	HWSync();																		// Update any hardware
	return FRAME_RATE;																// Return frame rate.
}

// *******************************************************************************************************************************
//
//												Read/Write Memory
//
// *******************************************************************************************************************************

BYTE8 CPUReadMemory(WORD16 address) {
	return Read(address);
}

void CPUWriteMemory(WORD16 address,BYTE8 data) {
	Write(address,data);
}

#include "gfx.h"

// *******************************************************************************************************************************
//
//		Execute chunk of code, to either of two break points or frame-out, return non-zero frame rate on frame, breakpoint 0
//
// *******************************************************************************************************************************

BYTE8 CPUExecute(WORD16 breakPoint1,WORD16 breakPoint2) { 
	BYTE8 next;
	BYTE8 brk = 0x03;
	do {
		BYTE8 r = CPUExecuteInstruction();											// Execute an instruction
		if (r != 0) return r; 														// Frame out.
		next = CPUReadMemory(CPUGetPC());
	} while (CPUGetPC() != breakPoint1 && CPUGetPC() != breakPoint2 && next!=brk);	// Stop on breakpoint or UNOP, which is now break debugger
	return 0; 
}

// *******************************************************************************************************************************
//
//									Return address of breakpoint for step-over, or 0 if N/A
//
// *******************************************************************************************************************************

WORD16 CPUGetStepOverBreakpoint(void) {
	BYTE8 opcode = CPUReadMemory(CPUGetPC());										// Current opcode.
	int offset = CPUGetStep65(opcode); 									 			// Get offset
	if (offset != 0) offset = (CPUGetPC()+offset) & 0xFFFF;							// Step over Subroutines
	return offset;																	// Do a normal single step
}

// *******************************************************************************************************************************make //
//												Called at exit, dumps CPU memory
//
// *******************************************************************************************************************************

void CPUEndRun(void) {
	FILE *f = fopen("memory.dump","wb");
	fwrite(cpuMemory,1,MEMSIZE,f);
	fclose(f);	
}

void CPUExit(void) {	
	printf("Exiting.\n");
	GFXExit();
}



