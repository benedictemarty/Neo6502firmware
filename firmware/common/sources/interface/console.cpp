// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      console.cpp
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      3rd March 2024
//      Reviewed :  No
//      Purpose :   Console system. Version 2.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

#include "interface/font_5x7.h"
#include "interface/font_8x14.h"  												// Hercules 9x14 text (F-52)
#include <stdarg.h>

struct GraphicsMode *graphMode;                                         

// ***************************************************************************************
//
//						User defined font memory (64 characters)
//
// ***************************************************************************************

uint8_t userDefinedFont[64*8];

// ***************************************************************************************
//
//									Update a user character
//
// ***************************************************************************************

uint8_t CONUpdateUserFont(uint8_t *data) {
	if (data[0] < 0xC0) return 1;
	for (int i = 0;i < 7;i++) {
		userDefinedFont[(data[0] & 0x3F)*8 + i] = data[i+1];
	}
	return 0;
}	

// ***************************************************************************************
//
//						Repaint character at console position (x,y)
//
// ***************************************************************************************

//
//		Packed modes (1/4 bpp) go through GFXWritePixelRaw. Colours beyond the mode depth
//		are masked by GFXWritePixelRaw (1 bpp keeps bit 0 : any non black colour is "on").
//		Cells of 14 lines (Hercules 9x14) use the 8x14 font for $20-$7F ; the 8 line
//		glyphs ($80-$BF symbols, $C0-$FF UDG) are centred vertically. The 9th column is
//		always background (no MDA style replication for $C0-$DF, the Neo charset differs).
//
//		Monochrome (Hercules) attributes : the colour nibbles form the IBM MDA attribute
//		byte (paper << 4 | ink), decoded as the MDA does (T-16, replaces the F-52 bit
//		scheme of 0.3.0 which turned NeoBASIC's inks 2-7 and 11 into underline, bold and
//		blink) : ink 0 = off (black), 1 = underline, 2-7 = normal, 8-15 = bold (bright) ;
//		paper 1-7 = inverse video, paper 8-15 = blink. So the mode 0 defaults (ink 7,
//		paper 0) and the NeoBASIC colour scheme render as plain text.
//
#define MDA_ATTR_UNDERLINE 	(0x02)
#define MDA_ATTR_BRIGHT 	(0x04)
#define MDA_ATTR_BLINK 		(0x08)
#define MDA_ATTR_INVERSE 	(0x10)
#define MDA_INK 			(7)  													// Default monochrome ink : normal text.
static void CONPaintCharacter(uint16_t x,uint16_t y);
#include "data/latin1font.h"  													// T-20 (F-17 of the fork) : Latin-1 symbols $A0-$BF, default letters $C0-$FF
static const uint8_t blankGlyph[8] = {0,0,0,0,0,0,0,0};
static const uint8_t *consoleFont = font_5x7;  									// 2,21 (F-95) : 8 line glyphs $20-$7F (built in or 6502 RAM)
static const uint8_t *consoleFont14 = font_8x14;  								// 14 line glyphs $20-$7F of the 9x14 cells (mode 1)

// 8 line glyph of any character : $20-$7F console font, $80-$9F blank (control codes), $A0-$BF Latin-1
// symbols (flash), $C0-$FF user defined (Latin-1 letters by default, 2,5 replaces them). QuickDraw uses it too.
const uint8_t *CONGlyph(uint8_t ch) {
	if (ch < 0x20) return blankGlyph;
	if (ch < 0x80) return consoleFont + (ch - 0x20) * 8;
	if (ch < 0xA0) return blankGlyph;
	if (ch < 0xC0) return font_latin1_symbols + (ch - 0xA0) * 8;
	return userDefinedFont + (ch - 0xC0) * 8;
}

void CONResetUserFont(void) {  													// Reset : Latin-1 letters in $C0-$FF
	memcpy(userDefinedFont,font_latin1_letters,sizeof(userDefinedFont));
}

// 2,21 : font for $20-$7F. addr = 0 restores the built in font ; otherwise 96 x 8 bytes in 6502 RAM (MSB = left),
// read in place (nothing copied to the RP2040 SRAM). addr14 : the 14 line glyphs of the 9x14 cells (96 x 14 bytes),
// 0 keeps the built in 8x14. The console is repainted.
uint8_t CONSetFont(uint16_t addr,uint16_t addr14) {
	if (addr != 0 && addr > 0x10000 - 96*8) return 1;  							// Would run past the end of RAM.
	if (addr14 != 0 && addr14 > 0x10000 - 96*14) return 1;
	consoleFont = (addr == 0) ? font_5x7 : cpuMemory + addr;
	consoleFont14 = (addr14 == 0) ? font_8x14 : cpuMemory + addr14;
	if (graphMode != NULL && graphMode->xGSize != 0) {  							// Repaint the whole console.
		for (int y = 0;y < graphMode->yCSize;y++) {
			for (int x = 0;x < graphMode->xCSize;x++) CONPaintCharacter(x,y);
		}
	}
	return 0;
}

static uint8_t consoleEcho = 0;  												// 2,20 (F-92) : mirror console text to the debug port

void CONSetDebugEcho(uint8_t on) {
	consoleEcho = on;
}
static uint8_t blinkHidden = 0;  												// Blink phase : 1 = blinking text hidden.
static void CONPaintCharacter(uint16_t x,uint16_t y);

//		Decode the MDA attribute byte : ink on (bit 0), attribute flags (bits 1-4) ; paper is on
//		only in inverse video (an "on" ink with an "on" paper would be invisible).
static uint8_t CONMDADecode(uint8_t ink,uint8_t paper,uint8_t *fcol,uint8_t *bcol) {
	uint8_t attr = 0;
	if ((ink & 7) == 1) attr |= MDA_ATTR_UNDERLINE;
	if (ink & 8) attr |= MDA_ATTR_BRIGHT;
	if (paper & 8) attr |= MDA_ATTR_BLINK;
	if (paper & 7) attr |= MDA_ATTR_INVERSE;
	*fcol = (ink & 7) ? 1 : 0;*bcol = 0;
	if (attr & MDA_ATTR_INVERSE) { *bcol = 1;*fcol = 0; }  						// Inverse : black on white.
	return attr;
}

static bool CONMDABlinks(uint16_t cell) {  										// Console memory cell of a blinking character.
	return (cell & 0x8000) != 0;  												// Paper bit 3
}

static void CONPaintCharacterPacked(uint16_t x,uint16_t y,uint16_t ch,uint8_t fcol,uint8_t bcol) {
	uint16_t cWidth = graphMode->fontWidth,cHeight = graphMode->fontHeight;
	int xOrg = x * cWidth + (graphMode->xGSize - graphMode->xCSize * cWidth) / 2;	// Horizontal centering.
	int yOrg = y * cHeight;
	int yPad = (cHeight > 8) ? (cHeight - 8) / 2 : 0;  							// Centring of 8 line glyphs in taller cells.
	uint8_t attr = 0;
	if (graphMode->bitsPerPixel == 1) {  											// Monochrome : decode the MDA attribute byte.
		uint8_t f,b;
		attr = CONMDADecode(fcol,bcol,&f,&b);
		fcol = f;bcol = b;
		if ((attr & MDA_ATTR_BLINK) && blinkHidden) fcol = bcol;  					// Hidden phase of blinking text.
	}
	for (uint16_t y1 = 0;y1 < cHeight;y1++) {
		uint16_t b = 0;
		if (cHeight == 14 && ch < 128) {
			b = consoleFont14[(ch-32)*14 + y1];  										// 2,21 : user 8x14 font or built in.
		} else if (y1 >= yPad && y1 < yPad + 8) {
			b = CONGlyph(ch)[y1 - yPad];  												// Console font, Latin-1 or UDG
		}
		if (attr & MDA_ATTR_BRIGHT) b |= (b >> 1);  									// Bold : double strike.
		if ((attr & MDA_ATTR_UNDERLINE) && y1 == cHeight - 2) b = 0xFF;  			// Underline row (MDA : row 12 of 14).
		for (uint16_t x1 = 0;x1 < cWidth;x1++) {
			GFXWritePixelRaw(xOrg + x1,yOrg + y1,(b & 0x80) ? fcol : bcol);
			b = b << 1;
		}
	}
}

//
//		Blink (monochrome only) : called from DSPSync / the host frame sync. Phase from
//		the 100 Hz timer (half a second on, half a second off) ; repaints the blinking
//		cells when the phase changes, except the cursor cell (its reversal is kept).
//
void CONBlinkSync(void) {
	if (graphMode == NULL || graphMode->bitsPerPixel != 1) return;
	uint8_t hidden = (TMRRead() / 50) & 1;
	if (hidden == blinkHidden) return;
	blinkHidden = hidden;
	for (int y = 0;y < graphMode->yCSize;y++) {
		for (int x = 0;x < graphMode->xCSize;x++) {
			if (x == graphMode->xCursor && y == graphMode->yCursor) continue;
			if (CONMDABlinks(graphMode->consoleMemory[x + y * MAXCONSOLEWIDTH])) CONPaintCharacter(x,y);
		}
	}
}

static void CONPaintCharacter(uint16_t x,uint16_t y) {
 	if (x < graphMode->xCSize && y < graphMode->yCSize) {  						// Coords in range.
 		uint16_t ch = graphMode->consoleMemory[x + y * MAXCONSOLEWIDTH];		// Character data
 		uint8_t fcol = (ch >> 8) & 0x0F,bcol = (ch >> 12) & 0x0F; 				// Extract colours
 		uint16_t cWidth = graphMode->fontWidth,cHeight = graphMode->fontHeight; 
 		ch = ch & 0xFF;  														// Character #

 		if (graphMode->xGSize != 0 && graphMode->bitsPerPixel != 8) {  			// Packed modes : generic path.
 			CONPaintCharacterPacked(x,y,ch,fcol,bcol);
 		} else if (graphMode->xGSize != 0) {  									// Only if graphics mode.
			for (uint16_t y1 = 0;y1 < cHeight;y1++) {  							// Each line of font data

				uint16_t b = CONGlyph(ch)[y1]; 									// Bit pattern for that line (font, $A0-$BF Latin-1, $C0-$FF UDG).												

				uint8_t *screen = graphMode->graphicsMemory+					// Where in memory it starts.
												x*cWidth+(y*cHeight+y1) * 320;	
				screen += ((320-graphMode->xCSize*cWidth)/2);  					// Horizontal centering.
				for (uint16_t x1 = 0;x1 < cWidth;x1++) { 						// Output colours MSB first.
					*screen++ = (b & 0x80) ? fcol : bcol;
					b = b << 1;
				}
			}
 		}
 	}
}

// ***************************************************************************************
//
//						Raw character output, tracking console.
//
// ***************************************************************************************

static void CONDrawCharacter(uint16_t x,uint16_t y,uint16_t ch,uint16_t fcol,uint16_t bcol) {
 	if (x < graphMode->xCSize && y < graphMode->yCSize) {  						// Coords in range.
 		graphMode->consoleMemory[x + y * MAXCONSOLEWIDTH] =  					// Character and colours are packed into the console Memory.
 												ch | (fcol << 8) | (bcol << 12);
		CONPaintCharacter(x,y);  												// And repaint it. 												
	}
}

// ***************************************************************************************
//
//									Clear the screen
//
// ***************************************************************************************

void CONClearScreen(void) {
	graphMode->xCursor = graphMode->yCursor = 0;  								// Home cursor
	if (graphMode->xGSize != 0) {  												// Graphics present ?
		if (SPRSpritesInUse() && !GFXIsPackedMode()) {  												// Sprites present, only delete that layer
			for (int i = 0;i < gMode.xGSize*gMode.yGSize;i++) {
				graphMode->graphicsMemory[i] &= 0xF0;
			}
		} else {																// Erase graphics screen to black
			uint8_t fill = graphMode->backCol;  								// Replicate the colour in packed modes.
			if (graphMode->bitsPerPixel == 4) fill = (fill & 0x0F) | (fill << 4);
			if (graphMode->bitsPerPixel == 1) fill = (fill & 1) ? 0xFF : 0x00;
			memset(graphMode->graphicsMemory,fill,graphMode->pageSize);  		// Draw page only (F-55)
			SPRScreenCleared();  												// Packed modes : sprites went with it.
		}
	}
	uint8_t blankInk = 7;  														// Normal text in every mode (T-16 : MDA decoding)
	for (int c = 0;c < MAXCONSOLEMEMORY;c++) {  								// Erase the console memory.
		graphMode->consoleMemory[c] = ' ' + (blankInk << 8) + (0 << 12);
	}
	for (int y = 0;y < graphMode->yCSize;y++) {  								// No extended lines.
		graphMode->isExtLine[y] = 0;
	}
	CONSetCursorVisible(1);														// Cursor now visible again.
	GFXResetDefaults(); 														// Reset graphics defaults.
}

// ***************************************************************************************
//
//							Accessors for cursor position
//
// ***************************************************************************************

uint8_t CONSetCursorPosition(uint8_t x,uint8_t y) {
	if (x >= graphMode->xCSize || y >= graphMode->yCSize) return 1;  			// Range error
	graphMode->xCursor = x;  													// Set new position.
	graphMode->yCursor = y;
	return 0;
}

void CONGetCursorPosition(uint8_t* x, uint8_t* y) {
	*x = graphMode->xCursor;
	*y = graphMode->yCursor;
}

// ***************************************************************************************
//
//								  Fetch the screen size
//
// ***************************************************************************************

void CONGetScreenSizeChars(uint8_t* width, uint8_t* height) {
	*width = graphMode->xCSize;
	*height = graphMode->yCSize;
}

// ***************************************************************************************
//
//								Initialise the console system
//
// ***************************************************************************************

void CONInitialise(struct GraphicsMode *gMode) {
	graphMode = gMode;	
	graphMode->foreCol = 7;graphMode->backCol = 0; 	 							// Reset colours
	consoleFont = font_5x7;  													// 2,21 : built in font again (mode change, reset).
	consoleFont14 = font_8x14;
	blinkHidden = 0;
	CONWrite(12);  																// Clear screen / home cursor.
}

// ***************************************************************************************
//
//						Copy a character from one position to another
//	
// ***************************************************************************************

static void CONCopy(uint16_t xFrom,uint16_t yFrom,uint16_t xTo,uint16_t yTo) {
	graphMode->consoleMemory[xTo + yTo * MAXCONSOLEWIDTH] =  					// Copy console mirror data
		 						graphMode->consoleMemory[xFrom + yFrom * MAXCONSOLEWIDTH];
	CONPaintCharacter(xTo,yTo);		 						
}

// ***************************************************************************************
//
//									Insert and delete lines
//
// ***************************************************************************************

void CONInsertLine(uint8_t y) {
	if (y >= graphMode->yCSize) return; 										// Bad line #
	uint8_t y1 = graphMode->yCSize-1;  											// Copy to line.
	while (y != y1) {  															// Copy the main block down.
		for (int x = 0;x < graphMode->xCSize;x++) {
			CONCopy(x,y-1,x,y);
			graphMode->isExtLine[y] = graphMode->isExtLine[y-1];
		}
		y1--;
	}
	for (int x = 0;x < graphMode->xCSize;x++) {  								// Blank bottom line and make it not extended.
		CONDrawCharacter(x,y,' ',graphMode->foreCol,graphMode->backCol);
	}
	graphMode->isExtLine[y] = false;  
}

// ***************************************************************************************
//
//										Delete lines
//
// ***************************************************************************************

void CONDeleteLine(uint8_t y) {
	if (y >= graphMode->yCSize) return; 										// Bad line #
	while (y != graphMode->yCSize-1) {  										// Copy the main block up.
		for (int x = 0;x < graphMode->xCSize;x++) {
			CONCopy(x,y+1,x,y);
			graphMode->isExtLine[y] = graphMode->isExtLine[y+1];
		}
		y++;
	}
	for (int x = 0;x < graphMode->xCSize;x++) {  								// Blank bottom line and make it not extended.
		CONDrawCharacter(x,y,' ',graphMode->foreCol,graphMode->backCol);
	}
	graphMode->isExtLine[y] = false;  
}

// ***************************************************************************************
//
//								Clears an area on the screen
//
// ***************************************************************************************

void CONClearArea(int x1, int y1, int x2, int y2) {
	x1 = std::min(graphMode->xCSize-1, x1);
	y1 = std::min(graphMode->yCSize-1, y1);
	x2 = std::min(graphMode->xCSize-1, x2);
	y2 = std::min(graphMode->yCSize-1, y2);
	x2 = std::max(x2, x1);
	y2 = std::max(y2, y1);

	for (int x=x1; x<=x2; x++)
		for (int y=y1; y<=y2; y++)
			CONDrawCharacter(x, y, ' ', graphMode->foreCol, graphMode->backCol);
}

// ***************************************************************************************
//
//								Set/Get the text colours
//
// ***************************************************************************************

void CONSetForeBackColour(int fg, int bg) {
	graphMode->foreCol = fg & 0x0f;
	graphMode->backCol = bg & 0x0f;
}

void CONGetForeBackColour(int *fg, int *bg) {
	*fg = graphMode->foreCol;
	*bg = graphMode->backCol;
}

// ***************************************************************************************
//
//								Set cursor visible flag
//
// ***************************************************************************************

void CONSetCursorVisible(uint8_t vFlag) {
	graphMode->isCursorVisible = vFlag;
}
// ***************************************************************************************
//
//								Reverse colours at cursor
//
// ***************************************************************************************

void CONReverseCursorBlock(void) {
	if (graphMode->isCursorVisible != 0 && graphMode->bitsPerPixel != 8) { 		// Packed modes : generic path.
		int xOrg = graphMode->xCursor * graphMode->fontWidth +
					(graphMode->xGSize - graphMode->xCSize * graphMode->fontWidth) / 2;
		int yOrg = graphMode->yCursor * graphMode->fontHeight;
		for (int y = 0;y < graphMode->fontHeight;y++) {
			for (int x = 0;x < graphMode->fontWidth;x++) {
				uint8_t mask = (graphMode->bitsPerPixel == 1) ? 1 : graphMode->foreCol;   // Monochrome : always reverse (T-16)
				GFXWritePixelRaw(xOrg+x,yOrg+y,GFXReadPixelRaw(xOrg+x,yOrg+y) ^ mask);
			}
		}
		return;
	}
	if (graphMode->isCursorVisible != 0) {
		for (int y = 0;y < graphMode->fontHeight;y++) {
			uint8_t *p = graphMode->graphicsMemory + 
						 (y + graphMode->yCursor * graphMode->fontHeight) * graphMode->xGSize +
						graphMode->xCursor * graphMode->fontWidth;
			for (int x = 0;x < graphMode->fontWidth;x++) {
				*p ^= graphMode->foreCol;
				p++;
			}
		}
	}
}

// ***************************************************************************************
//
//										Get end position
//	
// ***************************************************************************************

static uint16_t CONGetEndPosition(uint8_t y) {
	while (y < graphMode->yCSize-1) {  											// Until reached the last line.
		if (!graphMode->isExtLine[y]) return y; 								// This is a line end
		y++;  																	// Continuation, try next line.
	}
	return y;
}

// ***************************************************************************************
//
//								Delete character at cursor
//
// ***************************************************************************************

static void CONDeleteCharacter(void) {
	int yBase = CONGetEndPosition(graphMode->yCursor);  						// End of line
	int x = graphMode->xCursor,y = graphMode->yCursor;  						// Current position
	int xNext,yNext;
	while (x != graphMode->xCSize-1 || y != yBase) {  							// While not end of line
		xNext = x+1;yNext = y;if (xNext == graphMode->xCSize) { xNext = 0;yNext++; }
		CONCopy(xNext,yNext,x,y);
		x = xNext;y = yNext;
	}
	CONDrawCharacter(x,y,' ',graphMode->foreCol,graphMode->backCol);	
}

// ***************************************************************************************
//
//								Insert Space at cursor
//
// ***************************************************************************************

static void CONInsertCharacter(void) {
	int y = CONGetEndPosition(graphMode->yCursor);  						// End of line
	int x = graphMode->xCSize-1;

	while (x != graphMode->xCursor || y != graphMode->yCursor) {  			// While not end of line
		int xPrev = x-1;int yPrev = y;   									// Work out prior character
		if (xPrev < 0) { yPrev--;xPrev = graphMode->xCSize-1; }
		CONCopy(xPrev,yPrev,x,y);  											// Copy previous forward.
		x = xPrev;y = yPrev;
	}
	CONDrawCharacter(x,y,' ',graphMode->foreCol,graphMode->backCol);	
}

// ***************************************************************************************
//
//			Send command to console. Similar to BBC Micro, not all supported.
//
// ***************************************************************************************

void CONWrite(int c) {
	if (consoleEcho) {  														// 2,20 : text and newlines go to the debug UART / stderr
		if (c == CC_ENTER) { FDBWrite(13);FDBWrite(10); }
		else if ((c >= 32 && c < 127) || (c >= 0xA0 && consoleEcho > 1)) FDBWrite((uint8_t)c);
	}

	switch (c) {

		case CC_LEFT:															// A/1 left
			if (graphMode->xCursor-- == 0) 
				graphMode->xCursor = graphMode->xCSize-1;
			break;

		case CC_RIGHT:															// D/4 right
			if (++graphMode->xCursor == graphMode->xCSize) 
				graphMode->xCursor = 0;
			break;

		case CC_INSERT:  														// E/5 Insert
			CONInsertCharacter();break;

		case CC_BACKSPACE: 														// H/8 backspace
			if (graphMode->xCursor == 0) {  									// Start of line ?
				if (graphMode->yCursor == 0) return; 							// Top of screen.
				if (!graphMode->isExtLine[graphMode->yCursor-1]) return; 		// Start of a line ? 
				graphMode->xCursor = graphMode->xCSize-1;graphMode->yCursor--;	// Go to end of previous line.
			} else {
				graphMode->xCursor--;
			}
			CONDeleteCharacter();
			break;

		case CC_TAB:  															// I/9 Tab
			do {
				CONWrite(CC_RIGHT);
			} while ((graphMode->xCursor % 8) != 0);
			if (graphMode->xCursor == 0) CONWrite(CC_DOWN);
			break;
			
		case CC_LF:
			graphMode->yCursor++; 												// J/10 down with scrolling.
			if (graphMode->yCursor == graphMode->yCSize) {
				graphMode->yCursor--;
				CONDeleteLine(0);
			}
			break;

		case CC_CLS: 	 														// L/12 clears the screen
			CONClearScreen();break;

		case CC_ENTER: 	 														// M/13 carriage return.
			graphMode->isExtLine[graphMode->yCursor] = 0;
			graphMode->xCursor = 0;CONWrite(CC_LF);break;

		case CC_DOWN: 															// S/19 down
			graphMode->yCursor = (graphMode->yCursor+1) % graphMode->yCSize;
			break;

		case CC_HOME: 															// T/20 home cursor.		
			graphMode->xCursor = graphMode->yCursor = 0;
			break;

		case CC_VTAB:  															// V/22 Vertical Tab
			do {
				CONWrite(CC_DOWN);
			} while ((graphMode->yCursor % 8) != 0);
			break;

		case CC_UP:																// W/23 up cursor
			graphMode->yCursor = (graphMode->yCursor+graphMode->yCSize - 1) % graphMode->yCSize;
			break;

		case CC_REVERSE:  														// X/24 reverse character at cursor
			CONReverseCursorBlock();break;

		case CC_DELETE:  														// Z/26 delete
			CONDeleteCharacter();break;

		default:
			if ((c >= ' ' && c < 127) || c >= 0xA0) {  							// 32-126,192+ output a character.
				CONDrawCharacter(graphMode->xCursor,graphMode->yCursor,c,graphMode->foreCol,graphMode->backCol);
				graphMode->xCursor++;
				if (graphMode->xCursor == graphMode->xCSize) {  				// Char at EOL mark extended.
					graphMode->isExtLine[graphMode->yCursor] = 1;
					graphMode->xCursor = 0;CONWrite(CC_LF);
				}

			} else {
				if (c >= 0x80 && c < 0x90) graphMode->foreCol = c & 0x0F;
				if (c >= 0x90 && c < 0xA0) graphMode->backCol = c & 0x0F;
			}
	}
}

// ***************************************************************************************
//
//					Load screen line from current cursor position
//
// ***************************************************************************************

void CONGetScreenLine(uint16_t addr) {
	while (graphMode->isExtLine[graphMode->yCursor]) {  						// Find the bottom line.
		graphMode->yCursor++;
	}
	int start = graphMode->yCursor;  											// Figure out where to start.
	while (start > 0 && graphMode->isExtLine[start-1]) start--;

	int bufferSize = 0;
	for (int line = start;line <= graphMode->yCursor;line++) { 					// Input into buffer.
		for (int x = 0;x < graphMode->xCSize;x++) {
			if (bufferSize < 255) {
				int ch = graphMode->consoleMemory[x+line*MAXCONSOLEWIDTH] & 0xFF;
				ch = (ch < ' ') ? ' ' : ch;
				cpuMemory[addr + bufferSize + 1] = ch;
				bufferSize++;
			}
		}
	}
	while (bufferSize > 0 && cpuMemory[addr+bufferSize] == ' ') bufferSize--;  	// Strip trailing spaces.
	cpuMemory[addr] = bufferSize;
	CONWrite(CC_ENTER); 														// Start of next line
}

// ***************************************************************************************
//
//								Rubbish debugging tools.
//
// ***************************************************************************************

void CONWriteHex(uint16_t h) {
	CONWrite(' ');
	for (uint16_t i = 0;i < 4;i++) {
		CONWrite("0123456789ABCDEF"[(h >> 12) & 0x0F]);
		h = h << 4;
	}
}

void CONWriteString(const char *s, ...) {
	va_list ap;

	va_start(ap, s);
	int len = vsnprintf(NULL, 0, s, ap);
	va_end(ap);

	char buffer[len+1];
	va_start(ap, s);
	vsnprintf(buffer, len+1, s, ap);
	va_end(ap);
	
	s = buffer;
	while (*s != '\0') CONWrite(*s++);
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
