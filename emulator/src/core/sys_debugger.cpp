// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		sys_debugger.c
//		Purpose:	Debugger Code (System Dependent)
//		Created:	22nd November 2023
//		Author:		Paul Robson (paul@robsons->org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "sys_processor.h"
#include "debugger.h"
#include "hardware.h"

#include "common.h"

#include "6502/__6502mnemonics.h"

#define DBGC_ADDRESS 	(0x0F0)														// Colour scheme.
#define DBGC_DATA 		(0x0FF)														// (Background is in main.c)
#define DBGC_HIGHLIGHT 	(0xFF0)

static int renderCount = 0;
static BYTE8 *videoRAM = NULL;														// VRAM simple pattern.
static BYTE8 *isExtArray = NULL;
static uint16_t palette[256]; 														// Palette
static uint16_t displayScale = 3; 													// Display scale.

// *******************************************************************************************************************************
//
//											Handle palette changes
//
// *******************************************************************************************************************************

void RNDSetPalette(uint8_t colour,uint8_t r,uint8_t g,uint8_t b) {
	palette[colour] = ((r >> 4) << 8) | (g & 0xF0) | (b >> 4);
}

// *******************************************************************************************************************************
//
//											Handle mode start
//
// *******************************************************************************************************************************

void RNDStartMode0(struct GraphicsMode *gMode) {
	videoRAM = gMode->graphicsMemory;
	isExtArray = gMode->isExtLine;
}

//
//		The emulator can display every mode (rendering is done from the packed buffer
//		through GFXReadPixelRaw, so it is the same code path for all of them).
//
int RNDModeSupported(int mode) {
	return mode >= 0 && mode < GFX_MODE_COUNT;
}

void RNDSetDisplayPage(uint8_t *displayMemory) {
	videoRAM = displayMemory;  													// Immediate : the emulator renders from gMode.displayMemory anyway.
}

void RNDSuspend(void) {}  														// T-17 : nothing to pause here
void RNDResume(void) {}

// Write the display to a binary PPM (P6) file, for automated tests (shot:C:FILE ; Trinity T-19, from the fork).
int RNDWriteScreenshot(const char *fileName) {
	if (videoRAM == NULL) return -1;
	FILE *f = fopen(fileName,"wb");
	if (f == NULL) return -1;
	fprintf(f,"P6\n%d %d\n255\n",gMode.xGSize,gMode.yGSize);
	for (int y = 0;y < gMode.yGSize;y++) {
		for (int x = 0;x < gMode.xGSize;x++) {
			uint16_t p = palette[GFXReadDisplayPixelRaw(x,y)];
			fputc(((p >> 8) & 0x0F) * 17,f);fputc(((p >> 4) & 0x0F) * 17,f);fputc((p & 0x0F) * 17,f);
		}
	}
	fclose(f);
	return 0;
}

// *******************************************************************************************************************************
//
//											Get/Set emulator display scale
//
// *******************************************************************************************************************************

void DBGSetDisplayScale(uint16_t scale) {
	if (scale >= 1 && scale <= 4) {
		displayScale = scale;
	}
}

BYTE8 DBGGetDisplayScale(void) {
	return displayScale;
}

// *******************************************************************************************************************************
//
//													Debugger arguments
//
// *******************************************************************************************************************************

void DBGSaveArguments(int argc,char *argv[]) {
	for (int i = 0;i < argc;i++) {
		char *p = argv[i];
		printf("%s\n",p);
		if (strncmp(p,"scale=",6) == 0 && strlen(p) == 7) {
			DBGSetDisplayScale(p[6]-'0');
		}
		if (strcmp(p,"fullscreen") == 0 || strcmp(p,"fullscreen=1") == 0) {  		// Full screen from the start (Ctrl+F11 toggles).
			GFXSetFullScreen(1);
		}
	}
}

// *******************************************************************************************************************************
//
//								Get information about the active part of the display
//
// *******************************************************************************************************************************

//
//		Scale (scale=1..4, or the largest integer that fits in full screen) applies to every
//		video mode ; the window grows to fit modes wider than 320x240 (Hercules 720x350).
//
void DGBXGetActiveDisplayInfo(SDL_Rect *r,int *pxs,int *pys,int *pxc,int *pyc) {
		*pxc = gMode.xGSize;*pyc = gMode.yGSize;
		int sw,sh;
		if (GFXIsFullScreen()) {
			GFXGetDrawableSize(&sw,&sh);
			int s = sw / (*pxc);if (sh / (*pyc) < s) s = sh / (*pyc);
			if (s < 1) s = 1;
			*pxs = *pys = s;
		} else {
			*pxs = *pys = SCALE;
			sw = WIN_WIDTH;sh = WIN_HEIGHT;
			if ((*pxc) * SCALE + 16 > sw) sw = (*pxc) * SCALE + 16;  				// Grow the window for wide/tall modes.
			if ((*pyc) * SCALE + 16 > sh) sh = (*pyc) * SCALE + 16;
			GFXSetWindowSize(sw,sh);
		}
		r->w = (*pxs) * (*pxc);r->h = (*pys) * (*pyc);
		r->x = sw/2-r->w/2;r->y = sh/2-r->h/2;
}

// *******************************************************************************************************************************
//
//											This renders the debug screen
//
// *******************************************************************************************************************************

static const char *labels[] = { "A","X","Y","PC","SP","SR","CY","N","V","B","D","I","Z","C", NULL };

// Disassemble the instruction at addr, writing it into buffer.
// Returns address of next instruction.
int DBGXDasm65(int addr, char* buffer) {
	int p = addr;
	int opc = CPUReadMemory(p);p = (p + 1) & 0xFFFF;							// Read opcode.
	strcpy(buffer,_mnemonics[opc]);												// Work out the opcode.
	char *at = strchr(buffer,'@');												// Look for '@'
	if (at != NULL) {															// Operand ?
		char hex[6],temp[32];	
		if (at[1] == '1') {
			sprintf(hex,"%02x",CPUReadMemory(p));
			p = (p+1) & 0xFFFF;
		}
		if (at[1] == '2') {
			sprintf(hex,"%02x%02x",CPUReadMemory(p+1),CPUReadMemory(p));
			p = (p+2) & 0xFFFF;
		}
		if (at[1] == 'r') {
			int addr = CPUReadMemory(p);
			p = (p+1) & 0xFFFF;
			if ((addr & 0x80) != 0) addr = addr-256;
			sprintf(hex,"%04x",addr+p);
		}
		strcpy(temp,buffer);
		strcpy(temp+(at-buffer),hex);
		strcat(temp,at+2);
		strcpy(buffer,temp);
	}
	return p;
}


// *******************************************************************************************************************************
//
// 							Return the number of bytes (1, 2 or 3) occupied by the instruction at addr.
//
// *******************************************************************************************************************************

int DBGXInstructionSize65(int addr) {
	int opcode = CPUReadMemory(addr);
	const char *at = strchr(_mnemonics[opcode],'@');
	if (at != NULL) {
		switch(at[1]) {
			case '1': return 2;
			case 'r': return 2;
			case '2': return 3;
			default: break; // shouldn't happen...
		}
	}
	return 1;   // It's a bare opcode.
}


// *******************************************************************************************************************************
//
// 									Dump out nbytes of memory to a string buffer.
//
// *******************************************************************************************************************************

// Buffer must have room for at least 3 bytes per memory location,
// plus an extra byte for null-termination.
// e.g for nbytes=3: "XX XX XX \0"
void DBGXDumpMem(int addr, int nbytes, char* buffer) {
	char* p = buffer;
	for (int i = 0; i < nbytes; ++i) {
		int b = CPUReadMemory((addr + i) & 0xFFFF);
		p += sprintf(p, "%02x ", b);
	}
}


// *******************************************************************************************************************************
//
// 									Render debug information and/or display
//
// *******************************************************************************************************************************

#define REG(n) 	(CPUReadMemory((n)*2)+CPUReadMemory((n)*2+1) * 256)

void DBGXRender(int *address,int showDisplay) {
	int n = 0;
	char buffer[32];
	CPUSTATUS65 *s = CPUGetStatus65();

	if (showDisplay == 0) {
		GFXSetCharacterSize(36,24);
		DBGVerticalLabel(21,0,labels,DBGC_ADDRESS,-1);								// Draw the labels for the register

		#define DN(v,w) GFXNumber(GRID(24,n++),v,16,w,GRIDSIZE,DBGC_DATA,-1)		// Helper macro

		DN(s->a,2);DN(s->x,2);DN(s->y,2);DN(s->pc,4);DN(s->sp+0x100,4);DN(s->status,2);DN(s->cycles,4);
		DN(s->sign,1);DN(s->overflow,1);DN(s->brk,1);DN(s->decimal,1);DN(s->interruptDisable,1);DN(s->zero,1);DN(s->carry,1);

		n = 0;
		int a = address[1];																// Dump Memory.
		for (int row = 17;row < 24;row++) {
			GFXNumber(GRID(0,row),a,16,4,GRIDSIZE,DBGC_ADDRESS,-1);
			for (int col = 0;col < 8;col++) {
				int c = CPUReadMemory(a);
				GFXNumber(GRID(5+col*3,row),c,16,2,GRIDSIZE,DBGC_DATA,-1);
				c = (c & 0x7F);if (c < ' ') c = '.';
				GFXCharacter(GRID(30+col,row),c,GRIDSIZE,DBGC_DATA,-1);
				a = (a + 1) & 0xFFFF;
			}		
		}

		int p = address[0];																// Dump program code. 

		for (int row = 0;row < 16;row++) {
			int isPC = (p == ((s->pc) & 0xFFFF));										// Tests.
			int isBrk = (p == address[3]);
			GFXNumber(GRID(0,row),p,16,4,GRIDSIZE,isPC ? DBGC_HIGHLIGHT:DBGC_ADDRESS,	// Display address / highlight / breakpoint
																		isBrk ? 0xF00 : -1);
           	p = DBGXDasm65(p, buffer);
			GFXString(GRID(5,row),buffer,GRIDSIZE,isPC ? DBGC_HIGHLIGHT:DBGC_DATA,-1);	// Print the mnemonic
		}
		
	}
	renderCount++;
	if (showDisplay != 0) {
		SDL_Rect r;
		int xc,yc,xs,ys;
		DGBXGetActiveDisplayInfo(&r,&xs,&ys,&xc,&yc);
		SDL_Rect rc2;rc2 = r;
		rc2.w += 8;rc2.h += 8;rc2.x -=4;rc2.y -= 4;
		GFXRectangle(&rc2,0);
		//GFXRectangle(&r,0);
		rc2.w = xs;rc2.h = ys;
		BYTE8 *vPtr = videoRAM;
		if (vPtr != NULL) {
			for (int y = 0;y < yc;y++) {
				rc2.y = r.y + y*ys;rc2.x = r.x;
				for (int x = 0;x < xc;x++) {
					int col = palette[GFXReadDisplayPixelRaw(x,y)];
					if (col != 0) GFXRectangle(&rc2,col);
					rc2.x += xs;
				}
			}
			const uint8_t *cursorImage;
			uint16_t cursorX,cursorY;
			uint8_t xHit,yHit;
			if (MSEGetCursorDrawInformation(&cursorX,&cursorY)) {
				cursorImage = CURGetCurrent(&xHit,&yHit);				
				cursorX -= xHit;cursorY -= yHit;
				uint8_t w = 16,h = 16;
				if (cursorX + 16 >= xc) w = xc-cursorX;
				if (cursorY + 16 >= yc) h = yc-cursorY;
				rc2.w = xs;rc2.h = ys;
				for (int x = 0;x < w;x++) {
					for (int y = 0;y < h;y++) {
						rc2.x = r.x + (x+cursorX)*xs;
						rc2.y = r.y + (y+cursorY)*ys;
						uint8_t pixel = cursorImage[x+y*16];
						if (pixel != 0xFF) GFXRectangle(&rc2,palette[pixel]);
					}
				}
			}
			for (int y = 0; y < gMode.yCSize;y++) {
			 	rc2.x = r.x + r.w + 4;
			 	rc2.y = r.y + y * ys * gMode.fontHeight + 2;
			 	rc2.w = xs * 2;rc2.h = ys * gMode.fontHeight - 4;
			 	GFXRectangle(&rc2,isExtArray[y] ? 0x0F0 : 0xF00);
			}
		}	
	}
}
