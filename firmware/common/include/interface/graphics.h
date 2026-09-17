// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      graphics.h
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      21st November 2023
//      Reviewed :  No
//      Purpose :   Graphics mode manager include
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#define MAXCONSOLEWIDTH  	(80) 	 											// Max console size 
#define MAXCONSOLEHEIGHT  	(43)												// (80x43 in 9x8 on Hercules, 40x32 on 320x256)
#define MAXGRAPHICSMEMORY 	(2 * 160 * 256)  									// Graphics memory : 1 page of mode 0, 2 pages of modes 1 and 2 (F-55)
#define MAXSCREENWIDTH 		(720)  												// Widest mode (line buffers)
extern uint8_t gfxLineScratch[MAXSCREENWIDTH];  										// One shared scratch line (tilemap, blitter doubling) : never reentrant (R22)

//
//		Display modes (F5 / ADR-02). Mode 0 is the original 320x240x256 and is byte for byte
//		unchanged. Other modes pack pixels (1 bpp MSB first, 4 bpp high nibble first) in the
//		same graphicsMemory; only the generic code paths (console, pixel, line, rectangle,
//		read pixel) support them so far. RNDModeSupported() lets each host (RP2040, emulator,
//		Phosphoneo) say which modes it can display.
//
#define GFX_MODE_320x240x256	(0)
#define GFX_MODE_HERCULES		(1)												// 720x350, 1 bpp, text 80x25 in 9x14
#define GFX_MODE_320x256x16		(2)												// 320x256, 4 bpp, text 40x32 in 8x8
#define GFX_MODE_COUNT 			(5)

struct GraphicsModeDescriptor {
	uint16_t xGSize,yGSize;														// Pixels
	uint8_t  bitsPerPixel;														// 8, 4 or 1
	uint16_t stride;															// Bytes per pixel line
	uint8_t  xCSize,yCSize;														// Console size (chars)
	uint8_t  fontWidth,fontHeight;												// Character cell
	uint8_t  timing;															// 0 = 640x480p60 pixel doubled, 1 = 720x480p60 native
	uint16_t yOffset;															// Vertical centring (lines) on the DVI frame
	uint8_t  layout;  															// 0 = framebuffer ; else rendered from 6502 RAM (ADR-04, memvideo.cpp)
};
#define LAYOUT_FRAMEBUFFER 	(0)
#define LAYOUT_ORIC_TEXT 	(1)  													// Oric TEXT 40x28, $BB80, serial attributes, charset $B400/$B800
extern const struct GraphicsModeDescriptor gfxModes[GFX_MODE_COUNT];
#define MAXCONSOLEMEMORY 	(MAXCONSOLEWIDTH * (MAXCONSOLEHEIGHT+1))			// Max byte memory, console text.
																				// (extra line for scrolling.)
struct GraphicsMode {
	uint16_t xCSize,yCSize;														// Max size of console (chars)
	uint16_t xGSize,yGSize;														// Size of console (pixels) [0,0 = no graphics]
	uint8_t  fontWidth,fontHeight;  											// Font size in pixels.
	uint8_t  xCursor,yCursor;  													// Cursor position
	uint8_t  foreCol,backCol;  													// Current colours
	uint8_t *graphicsMemory;  													// graphics memory
	uint16_t *consoleMemory;  									  				// console memory.
	uint8_t  isExtLine[MAXCONSOLEHEIGHT]; 										// True if console is extended line.
	uint8_t  isCursorVisible;													// True if cursor visible.
	uint8_t  modeID;															// Current mode (GFX_MODE_*)
	uint8_t  bitsPerPixel;														// 8, 4 or 1 (see descriptor)
	uint8_t  layout;  															// LAYOUT_* (ADR-04)
	uint16_t stride;															// Bytes per pixel line
	uint32_t pageSize;															// Bytes per page (stride * yGSize)
	uint8_t  pageCount;															// Pages available in this mode (F-55)
	uint8_t  drawPage,displayPage;												// graphicsMemory points at the draw page
	uint8_t  *displayMemory;													// Base of the displayed page
};

extern struct GraphicsMode gMode;

void RNDSetPalette(uint8_t colour,uint8_t r,uint8_t g,uint8_t b); 				// Implementation specific.
int  RNDGetFrameCount(void);
void RNDStartMode0(struct GraphicsMode *gMode);
int  RNDModeSupported(int mode); 												// Implementation specific : can this host display it ?

int  GFXSetMode(int Mode);  													// General. Returns 0 if ok, 1 if unsupported.
int  GFXGetMode(void);
void GFXWritePixelRaw(int x,int y,uint8_t colour); 								// Any mode, no clipping, no sprite layer.
uint8_t GFXReadPixelRaw(int x,int y);
int  GFXIsPackedMode(void); 													// 1 if bitsPerPixel != 8 (generic slow paths only)
int  GFXSetDrawPage(int page);  												// F-55 : 0 if ok, 1 if no such page
int  GFXSetDisplayPage(int page);
uint8_t GFXReadDisplayPixelRaw(int x,int y);  									// Pixel of the displayed page (renderers)
void RNDSetDisplayPage(uint8_t *displayMemory); 								// Implementation specific : shown from next frame
void GFXDefaultPalette(void);
void GFXResetDefaults(void);
void GFXSetDefaults(uint8_t *cmd);
void GFXSetColour(uint8_t colour);
void GFXSetPalette(uint8_t colour,uint8_t r,uint8_t g,uint8_t b); 
void GFXGetPalette(uint8_t colour,uint8_t *r,uint8_t *g,uint8_t *b);
void GFXGraphicsCommand(uint8_t cmd,uint8_t *data);
void GFXFastLine(struct GraphicsMode *gMode,int x, int y, int x2, int y2);
void GFXPlotPixel(struct GraphicsMode *gMode,int x,int y);
void GFXPlotPixelChecked(struct GraphicsMode *gMode,int x,int y);
void GFXEllipse(struct GraphicsMode *gMode,int x1,int y1,int x2,int y2,int useSolidFill);
int GFXFindImage(int type,int id);
uint8_t GFXGetDrawSize(void);
void GFXSetDrawColour(uint8_t colour);
void GFXSetSolidFlag(uint8_t isSolid);
void GFXSetDrawSize(uint8_t size);
void GFXSetFlipBits(uint8_t flip);

#endif

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************

// ADR-04 : modes rendered from 6502 RAM (memvideo.cpp)
void MEMRenderLine(uint8_t *dest,int y);  										// One line of gMode.xGSize palette indexes
uint8_t MEMReadPixel(int x,int y);  												// Emulators : pixel of the rendered line (cached per line)
void MEMSetPalette(void);  														// Palette of the emulated machine (indexes 0-7)
