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
//		RAM (T-108) : the fill stack (2 x 2 KB) lives in graphics memory just after the plane
//		(offsets 26 880 to 30 975) ; no static array, no heap.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#define AGIPIC_PILE 0																	// No static fill stack in the decoder (T-108)
#include "interface/agipic_decodeur.h"
#include "interface/agicel.h"

#ifdef PICO
#include "hardware/timer.h"
#endif

static_assert(AGI_PLANE_SIZE <= GFX_MEMORY_SIZE,"the AGI plane must fit in graphics memory");
static_assert(AGI_PLANE_SIZE == AGIPIC_TAILLE,"group 39 and the decoder must agree on the plane size");

#define AGI_FILL_CAPACITY	2048															// Fill stack points (deepest seen : 62 on 20 000 random streams)
static_assert(AGI_PLANE_SIZE + 2 * AGI_FILL_CAPACITY <= GFX_MEMORY_SIZE,"the fill stack must fit after the AGI plane");

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
	uint8_t *stack = gfxObjectMemory + AGI_PLANE_SIZE;
	int r = agipic_decoder_pile(cpuMemory + address,length,gfxObjectMemory,flags & 1,NULL,
								stack,stack + AGI_FILL_CAPACITY,AGI_FILL_CAPACITY);
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

// ***************************************************************************************
//
//		39,5 / 39,6 (T-109) : cels of AGI views, drawn to the screen only ; the plane is
//		never written. agicel.cpp does the clipping and the priority test, _AGIPutScreen
//		writes one AGI pixel (two screen pixels).
//
// ***************************************************************************************

static_assert(AGICEL_LARGEUR == AGI_WIDTH && AGICEL_HAUTEUR == AGI_HEIGHT,"agicel and group 39 must agree on the plane");

static void _AGIPutScreen(void *ctx,int x,int y,uint8_t colour) {
	uint16_t top = *(const uint16_t *)ctx;
	uint8_t *dst = gMode.graphicsMemory + (uint32_t)(top + y) * gMode.stride;
	if (gMode.bitsPerPixel == 8) {
		dst[2 * x] = colour;dst[2 * x + 1] = colour;
	} else {
		dst[x] = (uint8_t)((colour << 4) | colour);
	}
}

static bool _AGIScreenOk(uint16_t top) {
	if (gMode.xGSize != 2 * AGI_WIDTH || (uint32_t)top + AGI_HEIGHT > gMode.yGSize) return false;
	return gMode.bitsPerPixel == 8 || gMode.bitsPerPixel == 4;
}

uint8_t AGIDrawCel(uint16_t address,uint8_t x,uint8_t yBottom,uint8_t priority,uint8_t flags,uint16_t top) {
	if (!_AGIScreenOk(top)) return AGI_ERR_PARAM;
	return (uint8_t)agicel_dessiner_vers(cpuMemory,MEMORY_SIZE,address,x,yBottom,priority,flags & 1,
	                                     gfxObjectMemory,_AGIPutScreen,&top);
}

uint8_t AGIRestoreRect(uint8_t x,uint8_t y,uint8_t width,uint8_t height,uint16_t top) {
	if (!_AGIScreenOk(top)) return AGI_ERR_PARAM;
	return (uint8_t)agicel_restaurer_vers(x,y,width,height,gfxObjectMemory,_AGIPutScreen,&top);
}

// ***************************************************************************************
//
//		39,7 / 39,8 (T-122) : add.to.pic. 39,7 writes a cel into the plane (colour and
//		priority) with the priority test of 39,5 ; rows are drawn top to bottom and the
//		search under a control line goes down, so a point already written never changes
//		the test of the next ones as long as the priority is 4 or more : 0-3 is refused.
//		39,8 sets the priority nibble of a rectangle, clipped to the plane.
//
// ***************************************************************************************

static void _AGIPutPlane(void *ctx,int x,int y,uint8_t colour) {
	gfxObjectMemory[y * AGI_WIDTH + x] = (uint8_t)((*(const uint8_t *)ctx << 4) | colour);
}

uint8_t AGIAddCelToPlane(uint16_t address,uint8_t x,uint8_t yBottom,uint8_t priority,uint8_t flags) {
	if (priority < 4 || priority > 15) return AGI_ERR_PARAM;
	return (uint8_t)agicel_dessiner_vers(cpuMemory,MEMORY_SIZE,address,x,yBottom,priority,flags & 1,
	                                     gfxObjectMemory,_AGIPutPlane,&priority);
}

uint8_t AGIFillPlanePriority(uint8_t x,uint8_t y,uint8_t width,uint8_t height,uint8_t priority) {
	if (width == 0 || height == 0 || priority > 15) return AGI_ERR_PARAM;
	for (int yy = y;yy < y + height && yy < AGI_HEIGHT;yy++) {
		uint8_t *p = gfxObjectMemory + yy * AGI_WIDTH;
		for (int xx = x;xx < x + width && xx < AGI_WIDTH;xx++)
			p[xx] = (uint8_t)((priority << 4) | (p[xx] & 0x0F));
	}
	return AGI_ERR_OK;
}
