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

static void _T80SetPlanes(uint32_t cell) {  												// Planes of a cell from its attribute
	uint8_t attr = T80Attrs()[cell];
	uint8_t ink222 = T80Colour(attr & 15),paper222 = T80Colour(attr >> 4);
	uint32_t shift = (cell & 7) * 4;
	uint32_t *w = T80Planes() + (cell >> 3);
	for (int plane = 0;plane < 3;plane++) {  											// Blue, green, red
		uint32_t nib = (ink222 & 3) | ((paper222 & 3) << 2);
		*w = (*w & ~(0xFu << shift)) | (nib << shift);
		ink222 >>= 2;paper222 >>= 2;w += T80_PLANE_WORDS;
	}
}

void T80PutCell(int x,int y,uint8_t glyph,uint8_t attr) {
	if (x < 0 || y < 0 || x >= T80_COLS || y >= T80_ROWS) return;
	uint32_t cell = x + y * T80_COLS;
	T80Grid()[cell] = glyph;T80Attrs()[cell] = attr;
	_T80SetPlanes(cell);
}

void T80RecolourAll(void) {
	for (uint32_t cell = 0;cell < T80_CELLS;cell++) _T80SetPlanes(cell);
}

// ***************************************************************************************
//
//		5,13-5,15 (0.16.89, the interface asked by NeoBBS, D-01) : direct access to the grid, raw CP437
//		(every code is a glyph), attribute paper << 4 | ink. They do not go through the console : its
//		memory is not changed, so a console scroll or clear repaints what the console holds.
//
// ***************************************************************************************

uint8_t T80Write(int x,int y,uint8_t attr,const uint8_t *text,int length) {
	if (x < 0 || y < 0 || x >= T80_COLS || y >= T80_ROWS) return 1;
	for (int i = 0;i < length && x + i < T80_COLS;i++) T80PutCell(x + i,y,text[i],attr);
	return 0;
}

uint8_t T80Clear(int x1,int y1,int x2,int y2,uint8_t attr) {
	if (x1 < 0 || y1 < 0 || x2 >= T80_COLS || y2 >= T80_ROWS || x1 > x2 || y1 > y2) return 1;
	for (int y = y1;y <= y2;y++)
		for (int x = x1;x <= x2;x++) T80PutCell(x,y,' ',attr);
	return 0;
}

uint8_t T80Scroll(int top,int bottom,int count,uint8_t attr) {
	if (top < 0 || bottom >= T80_ROWS || top > bottom) return 1;
	int rows = bottom - top + 1,n = (count < 0) ? -count : count;
	if (n >= rows) return T80Clear(0,top,T80_COLS - 1,bottom,attr);
	if (n == 0) return 0;
	uint8_t *g = T80Grid(),*a = T80Attrs();
	int moved = (rows - n) * T80_COLS;
	if (count > 0) {  																	// Up : rows top+n.. move to top..
		memmove(g + top * T80_COLS,g + (top + n) * T80_COLS,moved);
		memmove(a + top * T80_COLS,a + (top + n) * T80_COLS,moved);
	} else {  																			// Down
		memmove(g + (top + n) * T80_COLS,g + top * T80_COLS,moved);
		memmove(a + (top + n) * T80_COLS,a + top * T80_COLS,moved);
	}
	for (uint32_t cell = top * T80_COLS;cell < (uint32_t)(bottom + 1) * T80_COLS;cell++)
		_T80SetPlanes(cell);  															// Planes follow the attributes (2 000 cells at most)
	return (count > 0) ? T80Clear(0,bottom - n + 1,T80_COLS - 1,bottom,attr) : T80Clear(0,top,T80_COLS - 1,top + n - 1,attr);
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
