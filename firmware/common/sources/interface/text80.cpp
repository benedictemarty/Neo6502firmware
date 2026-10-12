// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      text80.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      12th October 2026
//      Purpose :   Mode 3, colour text 80 x 25 in 8 x 8 cells (T-131)
//
//		640 x 200 shown with doubled lines in 640x480p60, as the CGA's 80 column text. Each cell
//		has an ink and a paper in RGB222 (levels 00/55/AA/FF : the CGA / ANSI colours are exact).
//		Measured on the board (essai c1086931, 2026-10-12) : the encoder takes 7 400 cycles of the
//		16 000 of a doubled line, no late line. The 8 x 16 version with native lines did not fit.
//		Shared by the board (dvi_320x240x256.cpp encodes the grid) and the emulators (T80PixelRGB222).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "interface/text80.h"
#include "interface/font_cp437_8x8.h"

//		T80Grid, T80Planes, T80HostArea : inline in text80.h (graphicsMemory is word aligned, memory.cpp : the
//		grid is at $80:0000 for 12,2 / 12,3).

const uint8_t *T80Font(void) { return fontCP437_8x8; }

// ***************************************************************************************
//
//									Write / invert a cell
//
// ***************************************************************************************

void T80PutCell(int x,int y,uint8_t glyph,uint8_t ink222,uint8_t paper222) {
	if (x < 0 || y < 0 || x >= T80_COLS || y >= T80_ROWS) return;
	uint32_t cell = x + y * T80_COLS,shift = (cell & 7) * 4;
	uint32_t *w = T80Planes() + (cell >> 3);
	T80Grid()[cell] = glyph;
	for (int plane = 0;plane < 3;plane++) {  											// Blue, green, red
		uint32_t nib = (ink222 & 3) | ((paper222 & 3) << 2);
		*w = (*w & ~(0xFu << shift)) | (nib << shift);
		ink222 >>= 2;paper222 >>= 2;w += T80_PLANE_WORDS;
	}
}

void T80InvertCell(int x,int y) {
	if (x < 0 || y < 0 || x >= T80_COLS || y >= T80_ROWS) return;
	uint32_t cell = x + y * T80_COLS,shift = (cell & 7) * 4;
	uint32_t *w = T80Planes() + (cell >> 3);
	for (int plane = 0;plane < 3;plane++) {
		uint32_t nib = (*w >> shift) & 0xF;
		nib = ((nib & 3) << 2) | (nib >> 2);  											// Ink <-> paper
		*w = (*w & ~(0xFu << shift)) | (nib << shift);
		w += T80_PLANE_WORDS;
	}
}

// ***************************************************************************************
//
//		Console code -> glyph. The console's $A0-$FF are Latin-1 (NeoDOS, NeoBASIC) : the same
//		character in CP437, else the letter without accent (cp437font8.py). Its control and colour
//		codes ($00-$1F, $80-$9F) are not stored as characters ; shown blank, as CONGlyph does.
//
// ***************************************************************************************

uint8_t T80Glyph(uint8_t ch) {
	if (ch < 0x20 || (ch >= 0x80 && ch < 0xA0)) return ' ';
	if (ch >= 0xA0) return latin1ToCP437[ch - 0xA0];
	return ch;
}

uint8_t T80Colour(uint8_t index) {  													// Nearest of the 4 levels per component
	uint8_t r,g,b;
	GFXGetPalette(index,&r,&g,&b);
	return ((r >> 6) << 4) | ((g >> 6) << 2) | (b >> 6);
}

// ***************************************************************************************
//
//		One pixel of the 640 x 200 picture, as the board's encoder shows it (emulators)
//
// ***************************************************************************************

uint8_t T80PixelRGB222(int x,int y) {
	if (x < 0 || y < 0 || x >= T80_COLS * 8 || y >= T80_ROWS * 8) return 0;
	uint32_t cell = (x >> 3) + (y >> 3) * T80_COLS,shift = (cell & 7) * 4;
	bool on = (fontCP437_8x8[T80Grid()[cell] * 8 + (y & 7)] >> (7 - (x & 7))) & 1;
	const uint32_t *w = T80Planes() + (cell >> 3);
	uint8_t rgb = 0;
	for (int plane = 0;plane < 3;plane++) {
		uint32_t nib = (w[plane * T80_PLANE_WORDS] >> shift) & 0xF;
		rgb |= (on ? (nib & 3) : (nib >> 2)) << (2 * plane);
	}
	return rgb;
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//		12-10-26 	First version (T-131, after the board measurement of the 8 x 8 essai)
//
// ***************************************************************************************
