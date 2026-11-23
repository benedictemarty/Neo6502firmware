// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      sprites.cpp
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      4th January 2024
//      Reviewed :  No
//      Purpose :   Sprite drawing code.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

static int16_t spriteVisibleCount; 												// How many currently visible ?
static int16_t turtleSpriteID = -1; 											// Sprite allocated to rotation
static int16_t turtleRotation = 0; 												// Rotation of turtle.
static int16_t turtleColour = 7; 												// Turtle drawing colour.
static uint8_t drawMode = 0; 													// T-111 : 0 XOR (upstream), 1 opaque, 2 byte (T-146)
static uint8_t imagePage = 0x90;  												// T-116 : page of the images of 6,7 ($90 or a bank)

// ***************************************************************************************
//
//									Sprite data
//
// ***************************************************************************************

static SPRITE_INTERNAL sprites[MAX_SPRITES];
static uint8_t turtleImage[16*16/2];
static SPRITE_ACTION saHide;
static void SPRSetupAction(SPRITE_ACTION *sa,SPRITE_INTERNAL *p);
static void _SPRUndraw(SPRITE_INTERNAL *p);
static bool _SPRDraw(SPRITE_INTERNAL *p);

//		T-146 : byte mode, the background under each drawn sprite is kept in graphics memory (page $90),
//		in the area given by 6,9. A slot is kept by its sprite until 6,3, 6,1 or 6,9 (saveLen 0 : none).
static uint16_t saveBase = 0,saveSize = 0;
static uint16_t saveOff[MAX_SPRITES],saveLen[MAX_SPRITES];
static void _SPRByteRestoreAll(void);

// ***************************************************************************************
//
//								 Anchor positions
//
// ***************************************************************************************

static const int16_t anchorX[] = { 	1,
	0,1,2,
	0,1,2,
	0,1,2,
};

static const int16_t anchorY[] = { 	1,
	2,2,2,
	1,1,1,
	0,0,0
};

// ***************************************************************************************
//
//								 Triangle bitmap data
//
// ***************************************************************************************

#include "data/triangle.h"

// ***************************************************************************************
//
//										Reset a sprites
//
// ***************************************************************************************

static void _SPRResetSprite(int n) {
	SPRITE_INTERNAL *p = &sprites[n];  											// Set everything to default values.
	p->isDrawn = p->isVisible = false;
	p->xc = p->yc = p->x = p->y = -1;
	p->imageSize = 0xFF;
	p->anchor = p->flip = 0;
	p->xSize = p->ySize = 0;	
	p->bpp = 0;p->mapOffset = 0xFFFF;  											// T-113 : standard image
}

// ***************************************************************************************
//
//									Reset all sprites
//
// ***************************************************************************************

//		"The sprite layer (high nibbles) is in use" : the drawing of group 5 and the console then keep the
//		high nibbles. Not in byte mode (T-146) : the background uses all 8 bits.
bool SPRSpritesInUse(void) {
	return spriteVisibleCount > 0 && !(drawMode == 2 && !GFXIsPackedMode());
}

// ***************************************************************************************
//
//									Reset all sprites
//
// ***************************************************************************************

void SPRResetAll(void) {
	for (int i = 0;i < MAX_SPRITES;i++) {  										// Reset all sprites.
		_SPRResetSprite(i);
	}
	turtleSpriteID = -1;turtleRotation = 0;turtleColour = 7; 					// Reset turtle setup
	SPRPHYSetForeground(0);  													// T-159 : no foreground
}

// ***************************************************************************************
//
//							Reset all sprites and clear display
//
// ***************************************************************************************

void SPRReset(void) {
	if (GFXIsPackedMode()) {  													// No sprite layer : erase (XOR) what is drawn.
		for (int i = 0;i < MAX_SPRITES;i++) {
			if (sprites[i].isDrawn) _SPRUndraw(&sprites[i]);
		}
		spriteVisibleCount = 0;
		SPRResetAll();
		drawMode = 0;imagePage = 0x90;  										// T-121 : 6,6 and 6,8 back to their boot value
		saveBase = saveSize = 0;memset(saveLen,0,sizeof(saveLen));  			// T-146
		return;
	}
	if (drawMode == 2) {  														// T-146 : byte mode, the background is put back
		_SPRByteRestoreAll();
		spriteVisibleCount = 0;
		SPRResetAll();
		drawMode = 0;imagePage = 0x90;
		saveBase = saveSize = 0;memset(saveLen,0,sizeof(saveLen));
		return;
	}
	saveBase = saveSize = 0;memset(saveLen,0,sizeof(saveLen));  				// T-146
	spriteVisibleCount = 0;
	SPRResetAll();  	
	drawMode = 0;imagePage = 0x90;  											// T-121
	for (int i = 0;i < gMode.xGSize * gMode.yGSize;i++) {  						// Clear the sprite layer
		gMode.graphicsMemory[i] &= 0x0F;  										// top 4 bits og graphics memory.
	}
}

// ***************************************************************************************
//
//				Set the current sprite, rotation and colour for the turtle
//
// ***************************************************************************************

void SPRSetTurtleSprite(int16_t spriteID,int16_t rotation,int16_t colour) {
	turtleSpriteID = spriteID;
	while (rotation < 0) rotation += 360;
	turtleRotation = rotation % 360;
	turtleColour = colour & 0xF;
}

// ***************************************************************************************
//
//						Set up structure for sprite action
//
// ***************************************************************************************

//
//		The draw page was cleared (packed modes) : the drawn sprites are gone with it,
//		mark them not drawn so that the next update does not XOR garbage back in.
//
void SPRScreenCleared(void) {
	if (drawMode == 2 && !GFXIsPackedMode()) { SPRBackgroundRedrawn();return; } // T-146 : saved again, drawn again
	if (!GFXIsPackedMode()) return;
	for (int i = 0;i < MAX_SPRITES;i++) {
		if (sprites[i].isDrawn) { sprites[i].isDrawn = false;spriteVisibleCount--; }
	}
}

static inline int _SPRScale(uint8_t flip) { return ((flip >> 2) & 3) + 1; } 	// T-160 : bits 2-3 of the flip byte

static void SPRSetupAction(SPRITE_ACTION *sa,SPRITE_INTERNAL *p) {
	sa->display = gMode.graphicsMemory + p->x + p->y*gMode.xGSize; 				// Work out the draw address top left of sprite.
	sa->image = p->imageAddress; 												// Where graphic data comes from.
	sa->x = p->x;sa->y = p->y;  												// Coordinates
	sa->xSize = p->xSize;sa->ySize = p->ySize;  								// Size and flip.
	sa->flip = p->flip & 3;
	sa->scale = _SPRScale(p->flip);  											// T-160
	sa->bpp = p->bpp;  															// T-113
	int bw = p->xSize / sa->scale;  											// Width of the image itself
	sa->stride = (p->bpp == 0) ? bw / 2 : (bw * p->bpp + 7) / 8;
	sa->map = (p->bpp != 0 && p->mapOffset != 0xFFFF) ? gfxObjectMemory + p->mapOffset : NULL;
}

// ***************************************************************************************
//
//		T-111 (Neo6502AigleDor) : opaque sprites, 8 bit modes only. An area is redrawn by
//		clearing its high nibbles (the sprite layer) and drawing every drawn sprite that
//		touches it, highest number first : the lowest number ends in front, colour 0 of
//		an image stays transparent. Erasing no longer depends on what is in the layer.
//
// ***************************************************************************************

static bool _SPROpaque(void) {
	return drawMode == 1 && !GFXIsPackedMode();
}

static void _SPRRefresh(int x0,int y0,int x1,int y1) {
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 >= gMode.xGSize) x1 = gMode.xGSize-1;
	if (y1 >= gMode.yGSize) y1 = gMode.yGSize-1;
	if (x0 > x1 || y0 > y1) return;
	for (int y = y0;y <= y1;y++) {  											// Clear the layer in the area
		uint8_t *d = gMode.graphicsMemory + y * gMode.xGSize + x0;
		for (int x = x0;x <= x1;x++) *d++ &= 0x0F;
	}
	SPRITE_ACTION sa;
	for (int i = MAX_SPRITES-1;i >= 0;i--) {  									// Back to front
		SPRITE_INTERNAL *q = &sprites[i];
		if (!q->isDrawn) continue;
		if (q->x > x1 || q->y > y1 || q->x+q->xSize-1 < x0 || q->y+q->ySize-1 < y0) continue;
		SPRSetupAction(&sa,q);
		SPRPHYDrawOpaque(&sa,x0,y0,x1,y1);
	}
}

// ***************************************************************************************
//
//		T-146 (NeoWoW) : byte sprites over an 8 bit background. A sprite is drawn over the whole
//		byte after its rectangle was saved, and erased by putting the save back. Saves only stay
//		right if, in each group of drawn sprites whose rectangles touch (step by step), the sprites
//		were drawn from the highest number to the lowest : so any change restores the whole group,
//		lowest number first (reverse order of drawing), and draws it again highest number first.
//		The lowest number ends in front, colour 0 of an image stays transparent.
//
// ***************************************************************************************

static bool _SPRByte(void) {
	return drawMode == 2 && !GFXIsPackedMode();
}

static bool _SPRTouches(const SPRITE_INTERNAL *q,int x0,int y0,int x1,int y1) {
	return !(q->x > x1 || q->y > y1 || q->x+q->xSize-1 < x0 || q->y+q->ySize-1 < y0);
}

static void _SPRByteGroup(const SPRITE_INTERNAL *p,bool *in) {  				// Drawn sprites touching p, step by step
	uint8_t queue[MAX_SPRITES];
	int head = 0,tail = 0;
	memset(in,0,MAX_SPRITES*sizeof(bool));
	for (int i = 0;i < MAX_SPRITES;i++) {
		if (sprites[i].isDrawn && _SPRTouches(&sprites[i],p->x,p->y,p->x+p->xSize-1,p->y+p->ySize-1)) {
			in[i] = true;queue[tail++] = i;
		}
	}
	while (head < tail) {
		const SPRITE_INTERNAL *q = &sprites[queue[head++]];
		for (int i = 0;i < MAX_SPRITES;i++) {
			if (!in[i] && sprites[i].isDrawn && _SPRTouches(&sprites[i],q->x,q->y,q->x+q->xSize-1,q->y+q->ySize-1)) {
				in[i] = true;queue[tail++] = i;
			}
		}
	}
}

static void _SPRByteRestore(int n) {
	SPRITE_ACTION sa;
	SPRSetupAction(&sa,&sprites[n]);
	SPRPHYRestoreByte(&sa,gfxObjectMemory + saveOff[n]);
}

static void _SPRByteSaveDraw(int n) {
	SPRITE_ACTION sa;
	SPRSetupAction(&sa,&sprites[n]);
	SPRPHYDrawByte(&sa,gfxObjectMemory + saveOff[n]);
}

static void _SPRByteRestoreAll(void) {  										// Every drawn sprite erased (groups do not touch)
	for (int i = 0;i < MAX_SPRITES;i++) {
		if (sprites[i].isDrawn) { _SPRByteRestore(i);sprites[i].isDrawn = false;spriteVisibleCount--; }
	}
}

static bool _SPRByteSlot(int n) {  												// A save of xSize x ySize bytes, first fit
	uint32_t need = (uint32_t)sprites[n].xSize * sprites[n].ySize;
	if (saveLen[n] != 0 && saveLen[n] >= need) return true;  					// The slot it has is big enough
	saveLen[n] = 0;
	if (need == 0 || need > saveSize) return false;
	uint32_t cand = saveBase,end = (uint32_t)saveBase + saveSize;
	bool moved = true;
	while (moved) {
		moved = false;
		if (cand + need > end) return false;
		for (int j = 0;j < MAX_SPRITES;j++) {
			if (saveLen[j] != 0 && cand < (uint32_t)saveOff[j] + saveLen[j] && saveOff[j] < cand + need) {
				cand = (uint32_t)saveOff[j] + saveLen[j];moved = true;
			}
		}
	}
	saveOff[n] = cand;saveLen[n] = need;
	return true;
}

static void _SPRUndraw(SPRITE_INTERNAL *p) {  									// Erase a drawn sprite
	if (_SPRByte()) {  															// T-146
		bool in[MAX_SPRITES];
		_SPRByteGroup(p,in);
		for (int i = 0;i < MAX_SPRITES;i++) if (in[i]) _SPRByteRestore(i);
		p->isDrawn = false;
		spriteVisibleCount--;
		for (int i = MAX_SPRITES-1;i >= 0;i--) if (in[i] && &sprites[i] != p) _SPRByteSaveDraw(i);
		return;
	}
	p->isDrawn = false;
	spriteVisibleCount--;
	if (_SPROpaque()) {
		_SPRRefresh(p->x,p->y,p->x+p->xSize-1,p->y+p->ySize-1);
	} else {
		SPRSetupAction(&saHide,p);
		SPRPHYErase(&saHide);
	}
}

static bool _SPRDraw(SPRITE_INTERNAL *p) {  									// Draw a sprite not drawn
	if (_SPRByte()) {  															// T-146 : false if no room to save
		int n = p - sprites;
		if (!_SPRByteSlot(n)) return false;
		bool in[MAX_SPRITES];
		_SPRByteGroup(p,in);
		for (int i = 0;i < MAX_SPRITES;i++) if (in[i]) _SPRByteRestore(i);
		p->isDrawn = true;
		spriteVisibleCount++;
		in[n] = true;
		for (int i = MAX_SPRITES-1;i >= 0;i--) if (in[i]) _SPRByteSaveDraw(i);
		return true;
	}
	p->isDrawn = true;
	spriteVisibleCount++;
	if (_SPROpaque()) {
		_SPRRefresh(p->x,p->y,p->x+p->xSize-1,p->y+p->ySize-1);
	} else {
		SPRSetupAction(&saHide,p);
		SPRPHYDraw(&saHide);
	}
	return true;
}

//		6,6 : the drawn sprites are erased the old way and drawn back the new way.
int SPRSetDrawMode(uint8_t mode) {
	if (mode > 2) return 1;
	if (mode == drawMode) return 0;
	bool wasDrawn[MAX_SPRITES];
	for (int i = 0;i < MAX_SPRITES;i++) {
		wasDrawn[i] = sprites[i].isDrawn;
		if (wasDrawn[i]) _SPRUndraw(&sprites[i]);
	}
	drawMode = mode;
	for (int i = MAX_SPRITES-1;i >= 0;i--) {
		if (wasDrawn[i]) _SPRDraw(&sprites[i]);
	}
	return 0;
}

// ***************************************************************************************
//
//									Hide a sprite
//
// ***************************************************************************************

void SPRHide(uint8_t *paramData) {
	int spriteID = *paramData;  												// Sprite ID
	if (spriteID < MAX_SPRITES) {  												// Legit ?
		SPRITE_INTERNAL *s = &sprites[spriteID];
		if (s->isDrawn) _SPRUndraw(s);  										// If drawn, erase it and mark not drawn.
		sprites[spriteID].isVisible = false;  									// Mark not visible.
		saveLen[spriteID] = 0;  												// T-146 : its save slot is free
	}
}

// ***************************************************************************************
//
//								Get sprite position.
//
// ***************************************************************************************

uint8_t SPRGetSpriteData(uint8_t *param) {
	if (*param >= MAX_SPRITES) return 1; 										// Bad sprite #
	SPRITE_INTERNAL *p = &sprites[*param];										// Sprite structure
	param[1] = p->xc & 0xFF;param[2] = p->xc >> 8;
	param[3] = p->yc & 0xFF;param[4] = p->yc >> 8;
	return 0;
}

// ***************************************************************************************
//
//					 Unpack turtle graphic for given angle
//
// ***************************************************************************************

static uint8_t *SPRUnpackTurtleGraphic(uint16_t angle) {
	uint8_t *gfx = turtleImage;
	for (int i = 0;i < 16;i++) {  												// One 16 bit bitmap at a time.
		uint16_t bits = triangleBitData[angle * 16 + i];  						// Get bit pattern
		for (int j = 0;j < 8;j++) {  											// 2 colours per pixel
			uint8_t b = 0;
			if (bits & 0x8000) b |= (turtleColour << 4);
			if (bits & 0x4000) b |= (turtleColour & 0x0F);
			*gfx++ = b;
			bits = bits << 2;
		}
	}
	return turtleImage;
}
// ***************************************************************************************
//
//								Update a sprite
//
// ***************************************************************************************


int SPRUpdate(uint8_t *paramData) {

	uint8_t spriteID = paramData[0];  											// Sprite ID
	if (spriteID >= MAX_SPRITES) return 1;  									// Invalid

	uint16_t x = paramData[1] + (paramData[2] << 8);  							// Extract new data.
	uint16_t y = paramData[3] + (paramData[4] << 8);
	uint8_t  imageSize = paramData[5];
	uint8_t flip = paramData[6];
	uint8_t anchor = paramData[7];
	//		T-114 (Neo6502AigleDor) : bit 6 of the anchor byte forces the redraw, the rest is the anchor
	//		($80 unchanged, so $C0 = force only). The image address is looked up again (image rewritten
	//		in place, header changed) and a sprite hidden by 6,3 becomes visible where it was.
	bool forced = (anchor & 0x40) != 0;
	anchor &= ~0x40;

	SPRITE_INTERNAL *p = &sprites[spriteID];  									// Pointer to sprite structures

	bool xyChanged = ((x & 0xFF00) != 0x8000) && (p->x != x || p->y != y); 		// Check to see if elements changed.
	bool isChanged = (imageSize != 0x80) && (imageSize != p->imageSize);
	bool flipChanged = (flip != 0x80) && (flip != p->flip);
	bool anchorChanged = (anchor != 0x80) && (anchor != p->anchor);
	bool isTurtle = (spriteID == turtleSpriteID);
	if (forced && !isChanged && p->imageSize != 0xFF && p->bpp == 0) {  		// Same image : find it again
		imageSize = p->imageSize;isChanged = true;
	}

	//printf("Sprite #%d to (%d,%d) ImSize:%x Flip:%x Anchor:%d %d %d %d %d @ %d,%d %d:%d\n",
	// 	*paramData,x,y,paramData[5],paramData[6],paramData[7],xyChanged,isChanged,flipChanged,anchorChanged,p->x,p->y,p->isVisible,p->isDrawn);

	if (xyChanged || isChanged || flipChanged || anchorChanged || isTurtle || forced) {  // Some change made.
		if (p->isDrawn) _SPRUndraw(p);  										// Erase if currently drawn

		int oldScale = _SPRScale(p->flip);  									// T-160
		if (flipChanged) { 														// Flip has changed
			p->flip = flip; 
		}
		if (isTurtle) p->flip &= 3;  											// The turtle is never zoomed
		int scale = _SPRScale(p->flip);									

		if (anchorChanged) {  													// Anchor has changed.
			if (anchor > 9) return 1;  											// Bad anchor.
			p->anchor = anchor;
		}

		if (isChanged != 0 && isTurtle == 0) {  								// Image or size changed, not turtle
			p->imageSize = imageSize;  											// Update image size.
			p->bpp = 0;p->mapOffset = 0xFFFF;  									// T-113 : back to a standard image
			p->xSize = p->ySize = ((imageSize & 0x40) ? 32:16) * scale;   		// Size of image (T-160 : zoomed).
			int img = GFXFindImage((imageSize & 0x40) ? 2 : 1,imageSize & 0x3F);  	// T-160 : the size no longer tells the set	// Address of image (offset in gfx memory)													
			if (img < 0) return 2;   											// Bad image number
			p->imageAddress = gfxObjectMemory + img;  							// Physical address
		}

		if (isTurtle) {  														// If the turtle sprite
			p->imageSize = p->xSize = p->ySize = 16;   							// Set up as the correct size.
			p->bpp = 0;p->mapOffset = 0xFFFF;
			p->imageAddress = SPRUnpackTurtleGraphic(turtleRotation);
		}

		bool rescaled = (scale != oldScale) && !isChanged && !isTurtle && p->imageSize != 0xFF;
		if (rescaled) {  														// T-160 : zoom changed, same image
			p->xSize = p->xSize / oldScale * scale;
			p->ySize = p->ySize / oldScale * scale;
			if (!xyChanged && (p->xc != -1 || p->yc != -1)) {  					// Same anchor point
				p->x = p->xc - anchorX[p->anchor] * p->xSize / 2;
				p->y = p->yc - anchorY[p->anchor] * p->ySize / 2;
			}
		}
		if (xyChanged) {  														// Positions changed.
			p->x = x - anchorX[p->anchor] * p->xSize / 2;   					// Initially anchor point is centre.
			p->y = y - anchorY[p->anchor] * p->ySize / 2;
			p->xc = x;p->yc = y;  												// Remember centre position
			p->isVisible = true;
			//printf("changed %d %d %d\n",p->x,p->y,p->isVisible);
		}

		if (forced && p->imageSize != 0xFF && (p->xc != -1 || p->yc != -1)) {  	// T-114 : shown again where it was
			p->isVisible = true;
		}

		if (p->isVisible && !_SPRDraw(p)) return 3;  							// Redraw if possible ; T-146 : 3 no room to save
	}

	return 0; 
}

// ***************************************************************************************
//
//		T-113 (Neo6502AigleDor, Neo6502Bagman) : 6,7, image of any size in graphics memory.
//		Parameters : sprite, offset in graphics memory (2 bytes), width, height (1-255), bits a
//		pixel (1, 2, 4, 8), offset of a colour table of 2^bpp bytes ($FFFF none). The sprite
//		keeps its anchor point ; 6,2 with an image number goes back to the standard images.
//
// ***************************************************************************************

int SPRSetImage(uint8_t *paramData) {
	uint8_t spriteID = paramData[0];
	if (spriteID >= MAX_SPRITES) return 1;
	uint16_t offset = paramData[1] + (paramData[2] << 8);
	uint8_t width = paramData[3],height = paramData[4],bpp = paramData[5];
	uint16_t map = paramData[6] + (paramData[7] << 8);
	if (width == 0 || height == 0) return 1;
	if (bpp != 1 && bpp != 2 && bpp != 4 && bpp != 8) return 1;
	uint32_t stride = ((uint32_t)width * bpp + 7) / 8;
	const uint8_t *image = BLTGetRealAddress(imagePage,offset);  				// T-116 : graphics memory or a bank (6,8)
	const uint8_t *imageEnd = BLTGetAreaEnd(imagePage);
	if (image == NULL || imageEnd == NULL || (uint32_t)(imageEnd - image) < stride * height) return 2;
	if (map != 0xFFFF && (uint32_t)map + (1u << bpp) > GFX_MEMORY_SIZE) return 2;
	SPRITE_INTERNAL *p = &sprites[spriteID];
	if (p->isDrawn) _SPRUndraw(p);
	p->imageSize = 0xFE;p->bpp = bpp;p->mapOffset = map;
	p->imageAddress = (uint8_t *)image;  										// Read only when it is in a bank
	p->xSize = width * _SPRScale(p->flip);p->ySize = height * _SPRScale(p->flip);  // T-160
	if (p->xc != -1 || p->yc != -1) {  											// Placed : same anchor point
		p->x = p->xc - anchorX[p->anchor] * p->xSize / 2;
		p->y = p->yc - anchorY[p->anchor] * p->ySize / 2;
	}
	if (p->isVisible && !_SPRDraw(p)) return 3;  								// T-146 : no room to save
	return 0;
}

// ***************************************************************************************
//
//		T-146 : 6,9, area of graphics memory (page $90) where byte mode saves the background :
//		offset (2 bytes), size (2 bytes). xSize x ySize bytes a sprite (256 for 16 x 16).
//		Sprites drawn in byte mode are erased and drawn again with the new area.
//
// ***************************************************************************************

int SPRSetSaveArea(uint8_t *paramData) {
	uint16_t offset = paramData[0] + (paramData[1] << 8);
	uint16_t size = paramData[2] + (paramData[3] << 8);
	if ((uint32_t)offset + size > GFX_MEMORY_SIZE) return 1;
	bool wasDrawn[MAX_SPRITES];
	bool byteMode = _SPRByte();
	for (int i = 0;i < MAX_SPRITES;i++) wasDrawn[i] = byteMode && sprites[i].isDrawn;
	if (byteMode) _SPRByteRestoreAll();
	memset(saveLen,0,sizeof(saveLen));
	saveBase = offset;saveSize = size;
	for (int i = MAX_SPRITES-1;i >= 0;i--) {
		if (wasDrawn[i]) _SPRDraw(&sprites[i]);  								// No room : stays hidden until 6,2
	}
	return 0;
}

// ***************************************************************************************
//
//		T-146 : 6,10, the program has drawn a new background under the sprites (12,3, group 5...) :
//		the saves are forgotten without being put back, the new background is saved and the
//		sprites drawn again. Every pixel under every drawn sprite must have been redrawn,
//		otherwise the old sprite pixels left there are saved as background. No effect in modes 0 and 1.
//
// ***************************************************************************************

int SPRBackgroundRedrawn(void) {
	if (!_SPRByte()) return 0;
	for (int i = MAX_SPRITES-1;i >= 0;i--) {
		if (sprites[i].isDrawn) _SPRByteSaveDraw(i);
	}
	return 0;
}

//		T-159 : 6,11, foreground threshold. Byte mode (6,6 = 2) : a background byte >= threshold is not
//		overwritten by a sprite pixel. 0 = off. The sprites drawn are erased and drawn again.
int SPRSetForeground(uint8_t threshold) {
	if (threshold == SPRPHYGetForeground()) return 0;
	bool wasDrawn[MAX_SPRITES];
	for (int i = 0;i < MAX_SPRITES;i++) {
		wasDrawn[i] = _SPRByte() && sprites[i].isDrawn;
	}
	if (_SPRByte()) _SPRByteRestoreAll();
	SPRPHYSetForeground(threshold);
	for (int i = MAX_SPRITES-1;i >= 0;i--) {
		if (wasDrawn[i]) _SPRDraw(&sprites[i]);
	}
	return 0;
}

// 6,8 (T-116) : the page later 6,7 calls take their image from. A bank is flash (written by 1,22) : read
// from core 0 during the API call that draws the sprite, never by the display core or DSPSync.
int SPRSetImagePage(uint8_t page) {
	if (page != 0x90 && (page < BANK_PAGE || page >= BANK_PAGE + BANK_COUNT)) return 1;
	imagePage = page;
	return 0;
}

// ***************************************************************************************
//
//								Collision check
//
// ***************************************************************************************

uint8_t SPRCollisionCheck(uint8_t *error,uint8_t s1,uint8_t s2,uint8_t distance) {
	if (s1 >= MAX_SPRITES || s2 >= MAX_SPRITES) { 								// Validate parameters
		*error = 1;
		return 0;
	}
	if ((!sprites[s1].isVisible) || (!sprites[s2].isVisible)) return false;  	// check both visible.

	SPRITE_INTERNAL *p1 = &sprites[s1],*p2 = &sprites[s2]; 						// Structure pointers.
	int sdist = (p1->xc-p2->xc)*(p1->xc-p2->xc)+(p1->yc-p2->yc)*(p1->yc-p2->yc);// Square of distance
	return sdist <= distance*distance;
}

// ***************************************************************************************
//
//		T-161 : 6,12, collision of the rectangles on the screen (size times zoom, T-160) of two shown
//		sprites : 1 if they overlap by at least one pixel, whatever their images. Error 1 : bad number.
//
// ***************************************************************************************

uint8_t SPRRectCollision(uint8_t *error,uint8_t s1,uint8_t s2) {
	if (s1 >= MAX_SPRITES || s2 >= MAX_SPRITES) { *error = 1;return 0; }
	const SPRITE_INTERNAL *a = &sprites[s1],*b = &sprites[s2];
	if (!a->isVisible || !b->isVisible) return 0;
	return _SPRTouches(b,a->x,a->y,a->x+a->xSize-1,a->y+a->ySize-1) ? 1 : 0;
}

// ***************************************************************************************
//
//		T-161 : 6,13, moves several sprites in one call. A list in graphics memory (page $90), 5 bytes each :
//		sprite, X low, X high, Y low, Y high ; image, flip and anchor are left as they are ($80 of 6,2).
//		Parameters : 0,1 offset of the list, 2 number of entries (1-255). On return Parameter:2 is the number
//		of entries done ; the first error stops the call and is returned (1 : list outside graphics memory).
//
// ***************************************************************************************

int SPRUpdateMany(uint8_t *paramData) {
	uint16_t offset = paramData[0] + (paramData[1] << 8);
	uint8_t count = paramData[2];
	paramData[2] = 0;
	if (count == 0 || (uint32_t)offset + (uint32_t)count * 5 > GFX_MEMORY_SIZE) return 1;
	for (int i = 0;i < count;i++) {
		const uint8_t *e = gfxObjectMemory + offset + i * 5;
		uint8_t q[8] = { e[0],e[1],e[2],e[3],e[4],0x80,0x80,0x80 };
		int err = SPRUpdate(q);
		if (err != 0) return err;
		paramData[2] = i + 1;
	}
	return 0;
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//		12-01-24	Changed initial image size to $FF as not picked up if sprite initialise to $80
//		15/01/24 	Fixes for better sprite clipping
//					Not setting x,y on saRemove caused issues.
//		16/01/24 	Added SpriteVisibleCount functionality.
//		23/01/24 	Rationalised SPRITE_ACTION initialisation code.
//					Added turtle rendering on demand code.
//		18/03/24 	Sprite collision requires visible sprites.
//		07/10/26 	T-114 : anchor bit 6 forces the redraw (bmarty).
//		07/10/26 	T-111 : opaque drawing mode with priority, 6,6 (bmarty).
//		07/10/26 	T-113 : 6,7 images of any size, 1/2/4/8 bits a pixel, colour table (bmarty).
//		07/10/26 	T-116 : 6,8 images of 6,7 taken in a flash bank (bmarty).
//		07/10/26 	T-121 : 6,1 also sets 6,6 back to 0 and 6,8 back to $90 (bmarty).
//		16/11/26 	T-146 : 6,6 mode 2 byte sprites with a saved background, 6,9, 6,10 (bmarty).
//
// ***************************************************************************************
