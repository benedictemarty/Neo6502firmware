// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      sprites.h
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      4th January 2024
//      Reviewed :  No
//      Purpose :   Sprite drawing code.
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _SPRITES_H
#define _SPRITES_H

#define MAX_SPRITES (128) 														// Max supported. Doesn't mean it's a good idea :)

typedef struct _sprite_internal {
	bool isDrawn;  																// TRUE if currently drawn
	bool isVisible;  															// TRUE if currently visible
	int16_t x,y;  																// Current drawn position (e.g. passed in)
	int16_t xc,yc;  															// Centre position
	uint16_t xSize,ySize;  														// Sprite horizontal/vertical size.
	uint8_t imageSize;  														// image (0:5) size (6) value, $FE image set by 6,7
	uint8_t bpp;  																// T-113 : 0 standard image, else 1/2/4/8 bits a pixel (6,7)
	uint8_t *imageAddress; 	 													// Physical graphic address in gfxMemory
	uint8_t flip;  																// flip 0:x 1:y
	uint8_t anchor;  															// anchor 0-9
	uint16_t mapOffset;  														// T-113 : colour table in gfxMemory, $FFFF none
} SPRITE_INTERNAL;

//		T-113 : the new fields use the padding, the array of 128 sprites does not grow (T-13).
static_assert(sizeof(void *) != 4 || sizeof(SPRITE_INTERNAL) == 24,"SPRITE_INTERNAL must stay 24 bytes");

typedef struct _sprite_action {
	uint8_t *display;  															// display position
	uint8_t *image;  															// image data 
	uint8_t flip; 																// flip position
	int16_t x,y;  																// top left.
	uint16_t xSize,ySize;  														// Sprite size on the screen (T-160 : image size x scale)
	uint8_t scale;  															// T-160 : 1-4, bits 2-3 of the flip byte of 6,2
	uint8_t xBytes; 															// Bytes to copy.
	uint8_t bpp;  																// T-113 : 0 standard (4 bits, fast path), 1/2/4/8
	uint16_t stride;  															// T-113 : bytes a line of the image
	const uint8_t *map;  														// T-113 : colour table or NULL
} SPRITE_ACTION;

void SPRReset(void);  															// Sprite methods
void SPRResetAll(void);
void SPRHide(uint8_t *paramData);
int SPRUpdate(uint8_t *paramData);
uint8_t SPRCollisionCheck(uint8_t *error,uint8_t s1,uint8_t s2,uint8_t distance);
uint8_t SPRGetSpriteData(uint8_t *param);
bool SPRSpritesInUse(void);
void SPRSetTurtleSprite(int16_t spriteID,int16_t rotation,int16_t colour);
void SPRScreenCleared(void);  													// Packed modes : drawn sprites were wiped with the screen

void SPRPHYErase(SPRITE_ACTION *s); 											// Sprite draw/erase routines
void SPRPHYDraw(SPRITE_ACTION *s);
void SPRPHYDrawOpaque(SPRITE_ACTION *s,int x0,int y0,int x1,int y1); 		// T-111 : opaque, clipped
int SPRSetDrawMode(uint8_t mode); 												// T-111 : 0 XOR (default), 1 opaque
int SPRSetImage(uint8_t *paramData);  											// T-113 : 6,7
int SPRSetImagePage(uint8_t page);  												// T-116 : 6,8, $90 or bank page $A0+n
void SPRPHYDrawByte(SPRITE_ACTION *s,uint8_t *save);  							// T-146 : whole byte, background saved
void SPRPHYRestoreByte(SPRITE_ACTION *s,const uint8_t *save);  					// T-146 : background put back
int SPRSetSaveArea(uint8_t *paramData);  										// T-146 : 6,9
int SPRBackgroundRedrawn(void);  												// T-146 : 6,10
uint8_t SPRRectCollision(uint8_t *error,uint8_t s1,uint8_t s2);  				// T-161 : 6,12
int SPRUpdateMany(uint8_t *paramData);  										// T-161 : 6,13
int SPRSetForeground(uint8_t threshold);  										// T-159 : 6,11
void SPRPHYSetForeground(uint8_t threshold);  									// T-159 : byte mode, background >= threshold stays
uint8_t SPRPHYGetForeground(void);

#endif

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
