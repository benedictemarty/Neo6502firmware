// SPDX-License-Identifier: EUPL-1.2
// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      mixed.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      24th November 2026
//      Purpose :   Mode 4 (T-165) : the 320 x 240 x 256 graphics of mode 0 and an 80 x 25 grid of
//                  8 x 8 text cells (the mode 3 format), each cell showing either its text or the
//                  4 x 8 graphics pixels under it (8 x 8 physical pixels). Requested by the Ozmoo
//                  project (Infocom v6 games : image windows beside text windows).
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define GFX_MODE_MIXED   (4)
#define MIX_COLS         (80)
#define MIX_ROWS         (25)                                                   // 200 lines of the 240 : 25 rows fit with the encoder's 4 096 bytes in the 8 192 of the RAM bank
#define MIX_CELLS        (MIX_COLS * MIX_ROWS)                                  // 2 000
#define MIX_GRID_BYTES   (2 * MIX_CELLS)                                        // 4 000 : glyph codes, then attributes (paper << 4 | ink)
#define MIX_HOST_BYTES   (4096)                                                 // The encoder's table and font, right after the grid in the bank
#define MIX_GRID_PAGE    (0x91)                                                 // Page for the blitter (12,2 / 12,3) while in mode 4
#define MIX_LIMIT        (4)                                                    // Segments per row accepted by 5,44 (measured : 12 015 cycles of 16 000 at 4)

#define MIX_MAXRUNS      (MIX_LIMIT)                                            // Size of the run tables the board's encoder reads

//		The grid and the encoder's table and font live in the RAM bank $80 (banks.cpp, bytes 0-8095 while mode 4 is active : 1,18 / 1,22 / 1,26
//		see it too, so no heap is taken : the heap stays what 41,2 needs).
//		Core 1 of the board reads these tables directly : plain RAM, never behind a call (T-101).
extern uint8_t *mixGrid;                                                        // Glyph codes at 0, attributes at MIX_CELLS
extern uint8_t mixRunN[MIX_ROWS];                                               // Segments of each row (cells [A, B), T = 1 text)
extern uint8_t mixRunA[MIX_ROWS][MIX_MAXRUNS],mixRunB[MIX_ROWS][MIX_MAXRUNS],mixRunT[MIX_ROWS][MIX_MAXRUNS];
extern uint8_t *mixHost;                                                        // In the bank after the grid : the encoder's table and font (core 1 reads them)
extern uint8_t mixColour[16];                                                   // Palette entries 0-15 as RGB222 (read by core 1)

int MixInit(void);                                                              // Entering mode 4 : grid blank (spaces, attribute $0F), every cell graphic ; always 0
void MixRecolour(void);                                                         // 5,32 : mixColour from the palette
uint8_t MixSetCells(int x1,int y1,int x2,int y2,int text);                      // 5,44 : 0 ok, 1 bad rectangle, 2 more than MIX_LIMIT segments in a row (nothing changed)
int MixTextPixel(int x,int y,uint8_t *rgb222);                                  // Emulators : 1 and the RGB222 of the text pixel at logical (x, y) if its cell is text
