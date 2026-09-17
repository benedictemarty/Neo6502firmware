// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      memvideo.cpp
//      Authors :   bmarty <bmarty@mailo.com>
//      Date :      18th September 2026
//      Purpose :   ADR-04 : video modes rendered directly from the 6502 RAM, one line at a
//                  time, shared by the board (core 1, before the TMDS encoder), neo and
//                  Phosphoneo (GFXReadPixelRaw). Slice (a) : Oric TEXT.
//
//                  Oric TEXT (LAYOUT_ORIC_TEXT), rules taken from Phosphoric's video.c
//                  (oracle, itself aligned on Oricutron / the real ULA) :
//                  - 40 x 28 cells of 6 x 8, screen at $BB80 (40 bytes per row) ;
//                  - each scanline starts with ink 7 (white), paper 0 (black), text
//                    attributes 0 (standard charset, single height, no blink) ;
//                  - a byte with bits 6 and 5 clear is a SERIAL ATTRIBUTE : 0-7 ink, 8-15
//                    text attributes (bit 0 alternate charset, bit 1 double height, bit 2
//                    blink), 16-23 paper, 24-31 video mode (ignored here) ; the cell is
//                    drawn in the (new) paper colour, complemented if bit 7 is set ;
//                  - otherwise a character : code = byte & $7F, glyph row read in RAM at
//                    $B400 (standard) or $B800 (alternate) + code * 8, bits 5..0 = pixels
//                    left to right ; bit 7 = inverse (ink and paper each XOR 7, not a
//                    swap) ; blink inverts the cell in the "on" phase ;
//                  - double height : glyph row = (line >> 1) + 4 on odd char rows (the
//                    hardware quirk : must start on an even row).
//                  The 240 pixels are centred in the 320 of the host mode (40 black each
//                  side). Colours : Oric 0-7 = black, red, green, yellow, blue, magenta,
//                  cyan, white in palette entries 0-7 (MEMSetPalette).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

#define ORIC_SCREEN   (0xBB80)
#define ORIC_CHARSET  (0xB400)
#define ORIC_ALTSET   (0xB800)
#define ORIC_COLS     (40)
#define ORIC_ROWS     (28)
#define ORIC_MARGIN   ((320 - ORIC_COLS * 6) / 2)  									// 40 pixels each side.

static uint8_t memLine[320];  														// Line cache for the emulators (memory layouts are 320 wide at most in this slice).
static int memLineY = -1;
static uint32_t memLineFrame = 0;

// ***************************************************************************************
//
//								Oric TEXT : one scanline
//
// ***************************************************************************************

static void MEMRenderOricText(uint8_t *dest,int y) {
	int row = y >> 3,chline = y & 7;
	uint8_t ink = 7,paper = 0,attr = 0;  											// Reset at every line, as the ULA does.
	bool blinkOn = (TMRRead() / 27) & 1;  											// ~0.27 s phase (Phosphoric : frame_counter & 16).
	const uint8_t *cell = cpuMemory + ORIC_SCREEN + row * ORIC_COLS;
	for (int i = 0;i < ORIC_MARGIN;i++) *dest++ = 0;
	for (int col = 0;col < ORIC_COLS;col++) {
		uint8_t b = *cell++;
		if ((b & 0x60) == 0) {  													// Serial attribute.
			uint8_t v = b & 0x1F;
			switch (v & 0x18) {
				case 0x00: ink = v & 7;break;
				case 0x08: attr = v & 7;break;
				case 0x10: paper = v & 7;break;
				default: break;  													// 24-31 : video mode, not handled in this layout.
			}
			uint8_t bg = (b & 0x80) ? (paper ^ 7) : paper;
			for (int i = 0;i < 6;i++) *dest++ = bg;
		} else {
			bool inv = (b & 0x80) != 0;
			if ((attr & 4) && blinkOn) inv = !inv;
			uint8_t fg = inv ? (ink ^ 7) : ink,bg = inv ? (paper ^ 7) : paper;
			int erow = (attr & 2) ? ((chline >> 1) + ((row & 1) ? 4 : 0)) : chline;
			uint16_t base = (attr & 1) ? ORIC_ALTSET : ORIC_CHARSET;
			uint8_t bits = cpuMemory[base + (b & 0x7F) * 8 + erow];
			for (int bx = 5;bx >= 0;bx--) *dest++ = (bits & (1 << bx)) ? fg : bg;
		}
	}
	for (int i = 0;i < ORIC_MARGIN;i++) *dest++ = 0;
}

// ***************************************************************************************
//
//		Public : render line y of the current mode into dest (gMode.xGSize bytes)
//
// ***************************************************************************************

void MEMRenderLine(uint8_t *dest,int y) {
	switch (gMode.layout) {
		case LAYOUT_ORIC_TEXT: MEMRenderOricText(dest,y);break;
		default: memset(dest,0,gMode.xGSize);break;
	}
}

//
//		Emulators read pixel by pixel : the line is rendered once per (line, frame).
//
uint8_t MEMReadPixel(int x,int y) {
	uint32_t frame = RNDGetFrameCount();
	if (y != memLineY || frame != memLineFrame) {
		MEMRenderLine(memLine,y);
		memLineY = y;memLineFrame = frame;
	}
	return (x >= 0 && x < gMode.xGSize) ? memLine[x] : 0;
}

void MEMSetPalette(void) {
	static const uint8_t oric[8][3] = {
		{0,0,0},{255,0,0},{0,255,0},{255,255,0},{0,0,255},{255,0,255},{0,255,255},{255,255,255}
	};
	for (int i = 0;i < 8;i++) GFXSetPalette(i,oric[i][0],oric[i][1],oric[i][2]);
	memLineY = -1;
}
