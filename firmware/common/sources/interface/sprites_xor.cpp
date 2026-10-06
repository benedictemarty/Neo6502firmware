// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      sprites_xor.cpp
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      4th January 2024
//      Reviewed :  No
//      Purpose :   XOR Sprite drawing code.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

static const int16_t clipTop = 0;
static const int16_t clipBottom = 239;
static const int16_t clipLeft = 0;
static const int16_t clipRight = 319;

// ***************************************************************************************
//
//								XOR in image L->R
//
// ***************************************************************************************

static void _SPXORDrawForwardLine(SPRITE_ACTION *sa) {
	uint8_t bytes = sa->xBytes;
	uint8_t *image = sa->image;
	uint8_t *display = sa->display;
	uint8_t b;
	while (bytes--) {
		if ((b = *image++)) {
			if (b & 0xF0) {
				*display ^= (b & 0xF0);
			}
			display++;
			if (b & 0x0F) {
				*display ^= b << 4;
			}		
			display++;
		} else {
			display += 2;
		}
	}
}

// ***************************************************************************************
//
//								XOR in image R->L
//
// ***************************************************************************************

static void _SPXORDrawBackwardLine(SPRITE_ACTION *sa) {
	uint8_t bytes = sa->xBytes;
	uint8_t *image = sa->image+sa->xBytes-1;
	uint8_t *display = sa->display;
	uint8_t b;
	while (bytes--) {
		if ((b = *image--)) {
			if (b & 0x0F) {
				*display ^= b << 4;
			}					
			display++;
			if (b & 0xF0) {
				*display ^= (b & 0xF0);
			}
			display++;
		} else {
			display += 2;
		}
	}
}

// ***************************************************************************************
//
//					  Erase sprite : Same as draw as we are XORing
//
// ***************************************************************************************

void SPRPHYErase(SPRITE_ACTION *s) {
	SPRPHYDraw(s); 
}

// ***************************************************************************************
//
//										Draw a sprite
//
// ***************************************************************************************

//
//		Packed modes (1/4 bpp, F-53) : the sprite is XORed pixel by pixel into the frame
//		buffer (no separate layer). Erase = draw again. Over a black background the
//		colours are exact ; over a coloured background they mix (XOR) ; in monochrome
//		any non zero sprite pixel inverts the background. Generic, unoptimised path.
//
static void _SPXORDrawPacked(SPRITE_ACTION *s) {
	int xSize = s->xSize,ySize = s->ySize;
	for (int yPos = 0;yPos < ySize;yPos++) {
		int y = s->y + yPos;
		if (y < 0 || y >= gMode.yGSize) continue;
		int yImg = (s->flip & 2) ? ySize-1-yPos : yPos;
		const uint8_t *line = s->image + yImg * xSize / 2;
		for (int xPos = 0;xPos < xSize;xPos++) {
			int x = s->x + xPos;
			if (x < 0 || x >= gMode.xGSize) continue;
			int xImg = (s->flip & 1) ? xSize-1-xPos : xPos;
			uint8_t p = line[xImg >> 1];
			p = (xImg & 1) ? (p & 0x0F) : (p >> 4);
			if (p != 0) GFXWritePixelRaw(x,y,GFXReadPixelRaw(x,y) ^ p);
		}
	}
}

void SPRPHYDraw(SPRITE_ACTION *s) {
	if (GFXIsPackedMode()) { _SPXORDrawPacked(s);return; }
	if (s->x < clipLeft - s->xSize || s->x > clipRight) return; 				// Clip completely.

	s->xBytes = s->xSize/2; 							 						// Bytes to copy

	if (s->x + s->xSize > clipRight) {  										// Clip on the right.
		s->xBytes -= (s->x+s->xSize-clipRight)/2;  								// By putting out less data.
		if ((s->flip & 1) != 0) s->image += s->xSize/2-s->xBytes;
	}

	int yAdjust = s->xSize/2; 	 												// Handle vertical flipping.
	if (s->flip & 2) {
		s->image += (s->ySize-1) * s->xSize/2;  								// Shift image data to last line
	 	yAdjust = -yAdjust;  													// And work backwards.
	}

	if (s->x < clipLeft) {  													// Are we off to the left.
		int adjust = (clipLeft-s->x+1)/2; 	 									// Bytes to clip
		s->display += adjust * 2;
		if ((s->flip & 1) == 0) s->image += adjust;
		s->xBytes -= adjust;
	}

	for (int yPos = 0;yPos < s->ySize;yPos++) {   								// Work top to bottom
		if (s->y+yPos >= clipTop && s->y+yPos <= clipBottom) {  				// In clip area.
			if (s->flip & 1) {  												// Draw according to x flip
				_SPXORDrawBackwardLine(s); 					
			} else {
				_SPXORDrawForwardLine(s); 					
			}
		}
		s->display += gMode.xGSize;
		s->image += yAdjust;
	}
	//printf("%d %d\n",s->isVisible,s->drawAddress);
}

// ***************************************************************************************
//
//		T-111 (Neo6502AigleDor) : opaque drawing, 8 bit modes. Writes the sprite colour into the high
//		nibble of every pixel where the image is not 0, inside the clip rectangle (x0,y0)-(x1,y1)
//		inclusive. The caller clears the area and draws the sprites back to front (sprites.cpp).
//
// ***************************************************************************************

void SPRPHYDrawOpaque(SPRITE_ACTION *s,int x0,int y0,int x1,int y1) {
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 >= gMode.xGSize) x1 = gMode.xGSize-1;
	if (y1 >= gMode.yGSize) y1 = gMode.yGSize-1;
	int xSize = s->xSize,ySize = s->ySize;
	int ya = (s->y > y0) ? s->y : y0,yb = (s->y+ySize-1 < y1) ? s->y+ySize-1 : y1;
	int xa = (s->x > x0) ? s->x : x0,xb = (s->x+xSize-1 < x1) ? s->x+xSize-1 : x1;
	for (int y = ya;y <= yb;y++) {
		int yImg = (s->flip & 2) ? ySize-1-(y-s->y) : y-s->y;
		const uint8_t *line = s->image + yImg * xSize / 2;
		uint8_t *display = gMode.graphicsMemory + y * gMode.xGSize + xa;
		for (int x = xa;x <= xb;x++,display++) {
			int xImg = (s->flip & 1) ? xSize-1-(x-s->x) : x-s->x;
			uint8_t p = line[xImg >> 1];
			p = (xImg & 1) ? (p & 0x0F) : (p >> 4);
			if (p != 0) *display = (*display & 0x0F) | (p << 4);
		}
	}
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//		15/01/24 	Fixes for better sprite clipping
//		07/10/26 	T-111 : opaque drawing (bmarty).
//
// ***************************************************************************************
