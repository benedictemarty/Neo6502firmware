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
#define T80_PLANES_OFFSET 	(T80_CELLS)  												// 2 000
#define T80_ATTRS_OFFSET 	(T80_CELLS + 3 * T80_PLANE_WORDS * 4)  						// 5 000 : attribute bytes (0.16.89)
#define T80_GRID_BYTES 		(T80_ATTRS_OFFSET + T80_CELLS)  								// 7 000 : glyphs, 3 colour planes, attributes
#define T80_HOST_BYTES 		(4096)  													// Board : encoder table and font after the grid

//		The grid lives at the start of graphicsMemory (word aligned, so at $80:0000 for the blitter) : T80_CELLS CP437 glyph codes, then
//		three colour planes (blue, green, red : TMDS lanes 0, 1, 2), each 4 bits per cell = 2 bits of ink
//		then 2 bits of paper, 8 cells per word, least significant first ; then (0.16.89) one attribute byte
//		per cell, paper << 4 | ink, palette entries 0-15 : the reference the planes are computed from, so a
//		palette change (5,32) recolours both what the console painted and what 5,13-5,15 wrote directly.

//		Inline : core 1 (the board's encoder) computes these itself — a call would be a call into the FLASH,
//		the very fault of T-101 (make test-coeur1 caught it on the first build).
extern uint8_t graphicsMemory[];  														// Word aligned (memory.cpp)
static inline uint8_t *T80Grid(void) { return graphicsMemory; }  						// Glyph codes (row by row)
static inline uint32_t *T80Planes(void) { return (uint32_t *)(graphicsMemory + T80_CELLS); }	// 3 x T80_PLANE_WORDS
static inline uint8_t *T80HostArea(void) { return graphicsMemory + T80_GRID_BYTES; }	// T80_HOST_BYTES after the grid
static inline uint8_t *T80Attrs(void) { return graphicsMemory + T80_ATTRS_OFFSET; }  	// paper << 4 | ink
void T80PutCell(int x,int y,uint8_t glyph,uint8_t attr);  								// attr = paper << 4 | ink (0-15)
void T80RecolourAll(void);  															// Planes from the attributes (5,32)
uint8_t T80Write(int x,int y,uint8_t attr,const uint8_t *text,int length);  			// 5,13 : raw CP437, clipped at the row end
uint8_t T80Scroll(int top,int bottom,int count,uint8_t attr);  						// 5,14 : rows top-bottom, count > 0 up
uint8_t T80Clear(int x1,int y1,int x2,int y2,uint8_t attr);  							// 5,15 : rectangle of spaces
void T80InvertCell(int x,int y);  														// Swap ink and paper (text cursor)
uint8_t T80Glyph(uint8_t ch);  															// Console code (Latin-1) -> CP437 glyph
uint8_t T80Colour(uint8_t index);  														// Palette entry -> RGB222 (rrggbb)
uint8_t T80PixelRGB222(int x,int y);  													// Pixel (640 x 200) as displayed
const uint8_t *T80Font(void);  															// 256 x 8 bytes, MSB = left

#endif
