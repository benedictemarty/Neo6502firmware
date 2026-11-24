// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      mixed.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      24th November 2026
//      Purpose :   Mode 4, text cells over the graphics of mode 0 (T-165, see mixed.h).
//
//		A cell is text or graphics (mask bit). For each row the mask is turned into segments
//		(runs of cells of one kind) that core 1 reads : at most MIX_LIMIT per row, because each
//		segment costs the encoder a fixed overhead (measured on the board, essai-mixte : 11 049
//		cycles per line with 2 segments, 12 015 with 4, 16 301 with 8, the budget is 16 000).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "interface/font_cp437_8x8.h"

uint8_t *mixGrid = NULL;
uint8_t *mixHost = NULL;
uint8_t mixRunN[MIX_ROWS];
uint8_t mixRunA[MIX_ROWS][MIX_MAXRUNS],mixRunB[MIX_ROWS][MIX_MAXRUNS],mixRunT[MIX_ROWS][MIX_MAXRUNS];
uint8_t mixColour[16];
static uint32_t mixMask[MIX_ROWS][3];                                           // Bit x of the row : 1 text

static bool _MixBit(const uint32_t *m,int x) { return (m[x >> 5] >> (x & 31)) & 1; }

//		Segments of one row from its mask ; returns the count, or MIX_MAXRUNS + 1 if there are more than the tables hold.
static int _MixRuns(const uint32_t *m,uint8_t *a,uint8_t *b,uint8_t *t) {
	int n = 0,start = 0;
	for (int x = 1;x <= MIX_COLS;x++) {
		if (x == MIX_COLS || _MixBit(m,x) != _MixBit(m,start)) {
			if (n == MIX_MAXRUNS) return MIX_MAXRUNS + 1;
			a[n] = (uint8_t)start;b[n] = (uint8_t)x;t[n] = _MixBit(m,start) ? 1 : 0;n++;
			start = x;
		}
	}
	return n;
}

void MixRecolour(void) {
	for (int i = 0;i < 16;i++) mixColour[i] = T80Colour(i);
}

int MixInit(void) {
	mixGrid = BNKRamBank();
	mixHost = mixGrid + MIX_GRID_BYTES;                                         // Word aligned (4 000 = 4 x 1 000)
	memset(mixGrid,' ',MIX_CELLS);memset(mixGrid + MIX_CELLS,0x0F,MIX_CELLS);
	memset(mixMask,0,sizeof(mixMask));
	for (int r = 0;r < MIX_ROWS;r++) { mixRunA[r][0] = 0;mixRunB[r][0] = MIX_COLS;mixRunT[r][0] = 0;mixRunN[r] = 1; }
	MixRecolour();
	return 0;
}

uint8_t MixSetCells(int x1,int y1,int x2,int y2,int text) {
	if (!GFXIsMixedMode() || x1 < 0 || y1 < 0 || x2 >= MIX_COLS || y2 >= MIX_ROWS || x1 > x2 || y1 > y2) return 1;
	uint32_t newMask[MIX_ROWS][3];
	uint8_t a[MIX_ROWS][MIX_MAXRUNS],b[MIX_ROWS][MIX_MAXRUNS],t[MIX_ROWS][MIX_MAXRUNS],n[MIX_ROWS];
	for (int y = y1;y <= y2;y++) {
		memcpy(newMask[y],mixMask[y],sizeof(newMask[y]));
		for (int x = x1;x <= x2;x++) {
			if (text) newMask[y][x >> 5] |= 1u << (x & 31); else newMask[y][x >> 5] &= ~(1u << (x & 31));
		}
		int count = _MixRuns(newMask[y],a[y],b[y],t[y]);
		if (count > MIX_LIMIT) return 2;                                        // Nothing is changed
		n[y] = (uint8_t)count;
	}
	for (int y = y1;y <= y2;y++) {                                              // Core 1 may read a row while it is rewritten : one line shows the old or the new split
		memcpy(mixMask[y],newMask[y],sizeof(mixMask[y]));
		for (int i = 0;i < n[y];i++) { mixRunA[y][i] = a[y][i];mixRunB[y][i] = b[y][i];mixRunT[y][i] = t[y][i]; }
		mixRunN[y] = n[y];
	}
	return 0;
}

//		Emulators : the text pixel of logical pixel (x, y) (4 text columns of the cell are 2 logical pixels ; ink wins if either is lit)
int MixTextPixel(int x,int y,uint8_t *rgb222) {
	if (!GFXIsMixedMode() || x < 0 || y < 0 || x >= 320 || y >= 240) return 0;
	int cx = x >> 2,cy = y >> 3;
	if (!_MixBit(mixMask[cy],cx)) return 0;
	uint32_t cell = cx + cy * MIX_COLS;
	uint8_t glyph = mixGrid[cell],attr = mixGrid[MIX_CELLS + cell];
	uint8_t bits = fontCP437_8x8[glyph * 8 + (y & 7)];
	int px = (x & 3) * 2;                                                       // First of the two text columns under this pixel
	bool on = ((bits >> (7 - px)) & 1) || ((bits >> (6 - px)) & 1);
	*rgb222 = mixColour[on ? (attr & 15) : (attr >> 4)];
	return 1;
}
