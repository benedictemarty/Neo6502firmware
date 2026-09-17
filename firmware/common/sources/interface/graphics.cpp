// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      graphics.cpp
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      21st November 2023
//      Reviewed :  No
//      Purpose :   Graphics mode manager
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

uint8_t gfxLineScratch[MAXSCREENWIDTH];  											// Shared scratch line (R22 : one buffer instead of one per user)

#include "interface/palette.h"  												// Default palette.

struct GraphicsMode gMode;														// Info about current mode.

struct _PaletteState {  														// Current RGB Palette values.
	uint8_t r,g,b;
} currentPalette[256];

// ***************************************************************************************
//
//							Initialise the graphics mode 0
//
// ***************************************************************************************

const struct GraphicsModeDescriptor gfxModes[GFX_MODE_COUNT] = {
	{ 320,240, 8, 320, 53,30, 6,8,  0, 0 },  									// 0 : original 320x240x256
	{ 720,350, 1,  90, 80,25, 9,14, 1, 65 },  									// 1 : Hercules text 80x25 (9x14) / graphics 720x348, timing 720x480
	{ 320,256, 4, 160, 40,32, 8,8,  0, 0 },  									// 2 : 320x256 x 16 colours, text 40x32 (8x8), vertical repeat 1
	{ 720,350, 1,  90, 80,43, 9,8,  1, 65 },  									// 3 : Hercules text 80x43 (9x8, 8 line font) on the mode 1 bitmap (F-57)
};

static void GFXInitialiseMode(int mode) {
	const struct GraphicsModeDescriptor *d = &gfxModes[mode];
	gMode.xCSize = d->xCSize;gMode.yCSize = d->yCSize;							// Console text size
	gMode.xGSize = d->xGSize;gMode.yGSize = d->yGSize;  						// Graphics pixel size.
	gMode.fontWidth = d->fontWidth;gMode.fontHeight = d->fontHeight;  			// Font size on display
	gMode.graphicsMemory = graphicsMemory;  									// Set up memory pointers.
	gMode.consoleMemory = consoleMemory;
	gMode.modeID = mode;
	gMode.bitsPerPixel = d->bitsPerPixel;
	gMode.stride = d->stride;
	gMode.pageSize = (uint32_t)d->stride * d->yGSize;  							// Pages (F-55) : as many as fit, at most 2.
	gMode.pageCount = MAXGRAPHICSMEMORY / gMode.pageSize;
	if (gMode.pageCount > 2) gMode.pageCount = 2;
	gMode.drawPage = gMode.displayPage = 0;
	gMode.displayMemory = graphicsMemory;
}

int GFXSetDrawPage(int page) {
	if (page < 0 || page >= gMode.pageCount) return 1;
	gMode.drawPage = page;
	gMode.graphicsMemory = graphicsMemory + page * gMode.pageSize;
	return 0;
}

int GFXSetDisplayPage(int page) {
	if (page < 0 || page >= gMode.pageCount) return 1;
	gMode.displayPage = page;
	gMode.displayMemory = graphicsMemory + page * gMode.pageSize;
	RNDSetDisplayPage(gMode.displayMemory);
	return 0;
}

uint8_t GFXReadDisplayPixelRaw(int x,int y) {
	uint8_t *save = gMode.graphicsMemory;  										// Same unpacking as the draw page.
	gMode.graphicsMemory = gMode.displayMemory;
	uint8_t p = GFXReadPixelRaw(x,y);
	gMode.graphicsMemory = save;
	return p;
}

int GFXGetMode(void) {
	return gMode.modeID;
}

int GFXIsPackedMode(void) {
	return gMode.bitsPerPixel != 8;
}

// ***************************************************************************************
//
//		Generic pixel access, valid in every mode (slow path ; no clipping, no sprite layer)
//
// ***************************************************************************************

void GFXWritePixelRaw(int x,int y,uint8_t colour) {
	uint8_t *line = gMode.graphicsMemory + y * gMode.stride;
	switch (gMode.bitsPerPixel) {
		case 8: line[x] = colour;break;
		case 4: if (x & 1) line[x >> 1] = (line[x >> 1] & 0xF0) | (colour & 0x0F);
				else line[x >> 1] = (line[x >> 1] & 0x0F) | (colour << 4);
				break;
		case 1: if (colour & 1) line[x >> 3] |= (0x80 >> (x & 7));
				else line[x >> 3] &= ~(0x80 >> (x & 7));
				break;
	}
}

uint8_t GFXReadPixelRaw(int x,int y) {
	uint8_t *line = gMode.graphicsMemory + y * gMode.stride;
	switch (gMode.bitsPerPixel) {
		case 8: return line[x];
		case 4: return (x & 1) ? (line[x >> 1] & 0x0F) : (line[x >> 1] >> 4);
		case 1: return (line[x >> 3] >> (7 - (x & 7))) & 1;
	}
	return 0;
}

// ***************************************************************************************
//
//								Change display mode
//
// ***************************************************************************************

int GFXSetMode(int Mode) {
	if (Mode < 0 || Mode >= GFX_MODE_COUNT || !RNDModeSupported(Mode)) return 1; 	// Unknown or not displayable here.
	GFXInitialiseMode(Mode); 													// Initialise the mode
	RNDStartMode0(&gMode); 					 									// Start it (renderer reads the descriptor fields)
	RNDSetDisplayPage(gMode.displayMemory);
	GFXDefaultPalette();   														// Standard palette
	if (gMode.bitsPerPixel == 1) GFXSetPalette(1,255,255,255); 					// Monochrome : "on" is white (changeable with 5,32)
	CONInitialise(&gMode);  													// Initialise the console.
	return 0;
}

// ***************************************************************************************
//
//		Default palette. For $0x, this is default colour x. For $yx it is default 
//		colour y. x doesn't matter
//
//		This is for the split layer sprites.
//
// ***************************************************************************************

void GFXDefaultPalette(void) {
	for (int i = 0;i < 256;i++) {  												// Default (wrong) palette.
		uint8_t c = i;
		int p = ((i < 16) ? i : (i >> 4)) *3; 									// What colour ?
		GFXSetPalette(c,default_palette[p],default_palette[p+1],default_palette[p+2]); 
   }
}

// ***************************************************************************************
//
//									Set Palette
//
// ***************************************************************************************

void GFXSetPalette(uint8_t colour,uint8_t r,uint8_t g,uint8_t b) {
	RNDSetPalette(colour,r,g,b);
	currentPalette[colour].r = r;
	currentPalette[colour].g = g;
	currentPalette[colour].b = b;
}

// ***************************************************************************************
//
//								   Fetch Palette
//
// ***************************************************************************************

void GFXGetPalette(uint8_t colour,uint8_t *r,uint8_t *g,uint8_t *b) {
	*r = currentPalette[colour].r;
	*g = currentPalette[colour].g;
	*b = currentPalette[colour].b;
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//		19-01-24 	Spun out default palette to seperate function.
//
// ***************************************************************************************
