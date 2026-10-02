// SPDX-License-Identifier: EUPL-1.2
// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      agipic.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      2nd October 2026
//      Purpose :   Group 39, Sierra AGI pictures (T-107, ADR 0001 of Neo6502AGI) : decodes an
//                  AGI v2 PICTURE resource held in 6502 RAM into a 160x168 plane kept in
//                  graphics memory (page $90), one byte per pixel : priority in the high
//                  nibble, visual colour in the low nibble (26 880 bytes from offset 0).
//                  Generic : no knowledge of any particular game.
//                  Written for Trinity ; the decoder behind it (agipic_decodeur.h) comes
//                  from Neo6502AGI, clean room rewrite (see agipic.cpp for provenance).
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define AGI_WIDTH        160
#define AGI_HEIGHT       168
#define AGI_PLANE_SIZE   (AGI_WIDTH * AGI_HEIGHT)                               // 26 880 bytes of gfxObjectMemory

#define AGI_ERR_OK       0
#define AGI_ERR_PARAM    1                                                      // Range outside 6502 RAM, coordinates, mode
#define AGI_ERR_FILL     2                                                      // Fill stack overflow

uint8_t AGIDrawPicture(uint16_t address,uint16_t length,uint8_t flags);        // 39,1
uint8_t AGIShowPicture(uint16_t y);                                             // 39,2
uint32_t AGILastDuration(void);                                                 // 39,3
uint8_t AGIReadPoint(uint8_t x,uint8_t y,uint8_t *visual,uint8_t *priority);   // 39,4
uint8_t AGIDrawCel(uint16_t address,uint8_t x,uint8_t yBottom,uint8_t priority,uint8_t flags,uint16_t top);  // 39,5
uint8_t AGIRestoreRect(uint8_t x,uint8_t y,uint8_t width,uint8_t height,uint16_t top);                      // 39,6
