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
#define MAXCONSOLEHEIGHT  	(30)												// 53x30 in mode 0, 80x25 in Hercules (Trinity : mode 2 removed)
#define MAXGRAPHICSMEMORY 	(320 * 240)  										// Graphics memory : 1 page of mode 0 (76 800), 2 pages of Hercules (2 x 31 500)
#define MAXSCREENWIDTH 		(720)  												// Widest mode (line buffers)

//
//		Display modes (F5 / ADR-02). Mode 0 is the original 320x240x256 and is byte for byte
//		unchanged. Other modes pack pixels (1 bpp MSB first, 4 bpp high nibble first) in the
//		same graphicsMemory; only the generic code paths (console, pixel, line, rectangle,
//		read pixel) support them so far. RNDModeSupported() lets each host (RP2040, emulator,
//		Phosphoneo) say which modes it can display.
//
#define GFX_MODE_320x240x256	(0)
#define GFX_MODE_HERCULES		(1)												// 720x350, 1 bpp, text 80x25 in 9x14
#define GFX_MODE_COUNT 			(2)												// Trinity : mode 2 (320x256x16) removed on 2026-09-19 (decision bmarty)

struct GraphicsModeDescriptor {
	uint16_t xGSize,yGSize;														// Pixels
	uint8_t  bitsPerPixel;														// 8, 4 or 1
	uint16_t stride;															// Bytes per pixel line
	uint8_t  xCSize,yCSize;														// Console size (chars)
	uint8_t  fontWidth,fontHeight;												// Character cell
	uint8_t  timing;															// 0 = 640x480p60 pixel doubled, 1 = 720x480p60 native
	uint16_t yOffset;															// Vertical centring (lines) on the DVI frame
};
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
void RNDSuspend(void);  														// T-17 : full teardown (unused)
void RNDResume(void);
void RNDCursorUpdate(void);  													// T-32c : core 0 prepares the cursor for the line callback
uint32_t RNDLateScanlines(void);  												// T-32c : late scanline counter (diagnostic)
uint32_t RNDPublishRejects(void);  											// T-71 : lines dropped instead of blocking the line callback
uint32_t RNDDmaWaitEscapes(void);
uint32_t RNDDmaRestarts(void);  										// T-71 : lane channels restarted from the IRQ  										// T-71 : bounded DVI channel wait gave up (must stay 0)
uint32_t RNDMonoBuffers(void);  											// T-71 : 1 bpp line buffers in use (2 or 4)
void RNDSetMonoBuffers(int count);  										// T-71 : pick 2 or 4 at run time, same binary
void HWBusProbe(void);  														// T-49 : sample the PIO stall flags (RAM, called by DSPSync)
void DBGInitialise(void);  														// T-73 : UART0 debug port (board only)
void DBGWrite(const char *s);  													// Appends to a RAM ring, never blocks
void DBGWriteNumber(const char *label,uint32_t value);
void DBGFlush(void);  															// Pushes into free FIFO space, from DSPSync
void DBGTelemetry(uint32_t late,uint32_t rejects,uint32_t sectors,uint32_t mode);  // T-73 : one line per second (T-71 : R=)
void DBGTelemetryTick(void);  													// T-73 : called from DSPSync
void DBGPoll(void);  															// T-74 : terminal — UART to the keyboard queue
void DBGClockChanged(void);  													// T-74 : re-derive the baud rate after a mode change
bool DBGOwnsGPIO(int gpio);  													// T-74 : true for the debug UART pins
bool DBGScanTick(void);  														// T-75 : which UEXT pin is the wire on ?
uint8_t STODebugDrive(int drive);  												// T-74 : FatFs drive -> USB address (RAM)
bool STODebugBusy(int slot);
uint32_t HWBusStalls(uint8_t which);  											// T-49 : 0 = data not ready, 1 = address FIFO late
void HWBusStallsReset(void);
uint32_t HWBusTiming(uint8_t which);  											// T-50 : µs away from the bus (sync / command)
void RNDFlashPause(void);  														// T-32b : park core 1 (RAM) around a flash write ...
void RNDFlashResume(void);  													// ... and let it go
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
