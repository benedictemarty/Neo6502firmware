// SPDX-License-Identifier: EUPL-1.2
// ***************************************************************************************
// ***************************************************************************************
//
//		Name :		agipic.cpp
//		Authors :	bmarty (bmarty@mailo.com)
//		Date :		2nd October 2026
//		Purpose :	Group 39, Sierra AGI pictures (T-107, see agipic.h) : API of the group.
//
//		PROVENANCE : this file (API wrapper) is written for Trinity. The decoder it calls,
//		agipic_decodeur.cpp with agipic_decodeur.h and agipic_tables.h, comes unchanged
//		(apart from include paths and the fill stack size) from Neo6502AGI
//		tools/agipic/, EUPL 1.2. That decoder was rewritten in a clean room from a
//		functional specification (Neo6502AGI docs/specs/picture-v2.md), its brush tables
//		obtained by black box observation of an earlier decoder, and checked pixel for
//		pixel against reference renderings and 20 000 random streams (Neo6502AGI
//		tools/agipic/salle-blanche/SALLE-BLANCHE.md, ADR 0003). Known limit : the
//		implementer may have seen other AGI interpreters (Sarien, ScummVM) beforehand ;
//		the clean room is a documented good faith effort, not an absolute guarantee.
//		It replaces a first decoder derived from Sarien (GPL v2), removed on 2026-10-02.
//
//		RAM : the fill stack (1 KB) and the decoder state are static ; no heap is used.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "interface/agipic_decodeur.h"

#ifdef PICO
#include "hardware/timer.h"
#endif

static_assert(AGI_PLANE_SIZE <= GFX_MEMORY_SIZE,"the AGI plane must fit in graphics memory");
static_assert(AGI_PLANE_SIZE == AGIPIC_TAILLE,"group 39 and the decoder must agree on the plane size");

static uint32_t agiLastDuration = 0;

static uint32_t _AGIMicros(void) {
	#ifdef PICO
	return time_us_32();
	#else
	return 0;																	// No common microsecond clock on the emulators
	#endif
}

// ***************************************************************************************
//
//		39,1 : decode the resource at address/length of 6502 RAM ; flags bit 0 = clear the
//		plane first (priority 4, visual 15).
//
// ***************************************************************************************

uint8_t AGIDrawPicture(uint16_t address,uint16_t length,uint8_t flags) {
	if (length == 0 || (uint32_t)address + length > MEMORY_SIZE) return AGI_ERR_PARAM;
	uint32_t start = _AGIMicros();
	int r = agipic_decoder(cpuMemory + address,length,gfxObjectMemory,flags & 1,NULL);
	agiLastDuration = _AGIMicros() - start;
	return (r == 0) ? AGI_ERR_OK : AGI_ERR_FILL;
}

// ***************************************************************************************
//
//		39,2 : copy the visual colours to the screen draw page from line y, each pixel
//		doubled in width (320 pixels). Modes with 8 or 4 bits per pixel, 320 wide.
//
// ***************************************************************************************

uint8_t AGIShowPicture(uint16_t y) {
	if (gMode.xGSize != 2 * AGI_WIDTH || (uint32_t)y + AGI_HEIGHT > gMode.yGSize) return AGI_ERR_PARAM;
	if (gMode.bitsPerPixel != 8 && gMode.bitsPerPixel != 4) return AGI_ERR_PARAM;
	for (int line = 0;line < AGI_HEIGHT;line++) {
		const uint8_t *src = gfxObjectMemory + line * AGI_WIDTH;
		uint8_t *dst = gMode.graphicsMemory + (uint32_t)(y + line) * gMode.stride;
		if (gMode.bitsPerPixel == 8) {
			for (int x = 0;x < AGI_WIDTH;x++) { uint8_t c = src[x] & 0x0F;*dst++ = c;*dst++ = c; }
		} else {
			for (int x = 0;x < AGI_WIDTH;x++) { uint8_t c = src[x] & 0x0F;*dst++ = (uint8_t)((c << 4) | c); }
		}
	}
	return AGI_ERR_OK;
}

// ***************************************************************************************
//
//		39,3 : duration of the last 39,1 in microseconds (board only, 0 on the emulators).
//		39,4 : visual colour and priority of a point of the plane.
//
// ***************************************************************************************

uint32_t AGILastDuration(void) { return agiLastDuration; }

uint8_t AGIReadPoint(uint8_t x,uint8_t y,uint8_t *visual,uint8_t *priority) {
	if (x >= AGI_WIDTH || y >= AGI_HEIGHT) return AGI_ERR_PARAM;
	uint8_t p = gfxObjectMemory[y * AGI_WIDTH + x];
	*visual = p & 0x0F;
	*priority = p >> 4;
	return AGI_ERR_OK;
}
