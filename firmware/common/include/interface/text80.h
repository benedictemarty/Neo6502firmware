// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      text80.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      12th October 2026
//      Purpose :   Mode 3, colour text 80 x 25 in 8 x 8 cells (T-131)
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _TEXT80_H
#define _TEXT80_H

#include <stdint.h>

#define GFX_MODE_TEXT80		(3)
#define T80_COLS 			(80)
#define T80_ROWS 			(25)
#define T80_CELLS 			(T80_COLS * T80_ROWS)
#define T80_PLANE_WORDS 	(T80_CELLS * 4 / 32)  										// 250 : 4 bits per cell and plane
#define T80_ROW_WORDS 		(T80_COLS * 4 / 32)  										// 10
#define T80_GRID_BYTES 		(T80_CELLS + 3 * T80_PLANE_WORDS * 4)  						// 5 000 : glyphs, then 3 colour planes
#define T80_HOST_BYTES 		(4096)  													// Board : encoder table and font after the grid

//		The grid lives at the start of graphicsMemory (word aligned, so at $80:0000 for the blitter) : T80_CELLS CP437 glyph codes, then
//		three colour planes (blue, green, red : TMDS lanes 0, 1, 2), each 4 bits per cell = 2 bits of ink
//		then 2 bits of paper, 8 cells per word, least significant first. The console (consoleMemory) stays
//		the reference : the grid is what it paints, in the layout the board's encoder reads.

//		Inline : core 1 (the board's encoder) computes these itself — a call would be a call into the FLASH,
//		the very fault of T-101 (make test-coeur1 caught it on the first build).
extern uint8_t graphicsMemory[];  														// Word aligned (memory.cpp)
static inline uint8_t *T80Grid(void) { return graphicsMemory; }  						// Glyph codes (row by row)
static inline uint32_t *T80Planes(void) { return (uint32_t *)(graphicsMemory + T80_CELLS); }	// 3 x T80_PLANE_WORDS
static inline uint8_t *T80HostArea(void) { return graphicsMemory + T80_GRID_BYTES; }	// T80_HOST_BYTES after the grid
void T80PutCell(int x,int y,uint8_t glyph,uint8_t ink222,uint8_t paper222);
void T80InvertCell(int x,int y);  														// Swap ink and paper (text cursor)
uint8_t T80Glyph(uint8_t ch);  															// Console code (Latin-1) -> CP437 glyph
uint8_t T80Colour(uint8_t index);  														// Palette entry -> RGB222 (rrggbb)
uint8_t T80PixelRGB222(int x,int y);  													// Pixel (640 x 200) as displayed
const uint8_t *T80Font(void);  															// 256 x 8 bytes, MSB = left

#endif
