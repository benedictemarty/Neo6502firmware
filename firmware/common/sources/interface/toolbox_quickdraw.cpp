// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      toolbox_quickdraw.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      17th September 2026
//      Purpose :   Toolbox group 32, QuickDraw (F-40 skeleton : port, clip, pen,
//                  rectangles ; lines, patterns, fonts and CopyBits come with F-41).
//
//      Drawing goes through GFXWritePixelRaw/GFXReadPixelRaw : any video mode, draw
//      page, no sprite layer, always clipped to the current clip rectangle.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

static struct QDRect clipRect;                                                  // Current clip (inside the screen)
static int16_t penX = 0,penY = 0;                                               // Pen position
static uint8_t penColour = 15;                                                  // Pen colour (palette index)

// ***************************************************************************************
//
//      Rect helpers : read/write a Rect in 6502 RAM, intersect
//
// ***************************************************************************************

static bool _QDReadRect(uint16_t addr,struct QDRect *r) {
    if (addr > 0xFF00 - 8) return false;                                        // Must lie in RAM below the API page
    const uint8_t *p = cpuMemory + addr;
    r->left = (int16_t)(p[0] | (p[1] << 8));
    r->top = (int16_t)(p[2] | (p[3] << 8));
    r->right = (int16_t)(p[4] | (p[5] << 8));
    r->bottom = (int16_t)(p[6] | (p[7] << 8));
    return r->right >= r->left && r->bottom >= r->top;                          // Empty allowed, inverted refused
}

static bool _QDWriteRect(uint16_t addr,const struct QDRect *r) {
    if (addr > 0xFF00 - 8) return false;
    uint8_t *p = cpuMemory + addr;
    p[0] = r->left & 0xFF;p[1] = r->left >> 8;
    p[2] = r->top & 0xFF;p[3] = r->top >> 8;
    p[4] = r->right & 0xFF;p[5] = r->right >> 8;
    p[6] = r->bottom & 0xFF;p[7] = r->bottom >> 8;
    return true;
}

static struct QDRect _QDIntersect(const struct QDRect *a,const struct QDRect *b) {
    struct QDRect r;
    r.left = std::max(a->left,b->left);r.top = std::max(a->top,b->top);
    r.right = std::min(a->right,b->right);r.bottom = std::min(a->bottom,b->bottom);
    if (r.right < r.left) r.right = r.left;
    if (r.bottom < r.top) r.bottom = r.top;
    return r;
}

static struct QDRect _QDScreenRect(void) {
    struct QDRect r = { 0,0,(int16_t)gMode.xGSize,(int16_t)gMode.yGSize };
    return r;
}

// Fill (or invert) the intersection of r and the clip.
static void _QDFill(const struct QDRect *r,int colour) {                        // colour < 0 : invert
    struct QDRect c = _QDIntersect(r,&clipRect);
    uint8_t mask = (gMode.bitsPerPixel == 8) ? 0xFF : (gMode.bitsPerPixel == 4) ? 0x0F : 0x01;
    for (int y = c.top;y < c.bottom;y++) {
        for (int x = c.left;x < c.right;x++) {
            if (colour < 0) GFXWritePixelRaw(x,y,GFXReadPixelRaw(x,y) ^ mask);
            else GFXWritePixelRaw(x,y,(uint8_t)colour);
        }
    }
}

// ***************************************************************************************
//
//      32,1 InitGraf : clip = screen, pen home, colour 15
//
// ***************************************************************************************

void QDInitGraf(void) {
    clipRect = _QDScreenRect();
    penX = penY = 0;
    penColour = 15;
}

// ***************************************************************************************
//
//      32,2 / 32,3 clip rectangle (always kept inside the screen)
//
// ***************************************************************************************

uint8_t QDSetClip(uint16_t rectAddr) {
    struct QDRect r,s = _QDScreenRect();
    if (!_QDReadRect(rectAddr,&r)) return QD_ERR_PARAM;
    clipRect = _QDIntersect(&r,&s);
    return QD_ERR_OK;
}

uint8_t QDGetClip(uint16_t rectAddr) {
    return _QDWriteRect(rectAddr,&clipRect) ? QD_ERR_OK : QD_ERR_PARAM;
}

// ***************************************************************************************
//
//      32,4 .. 32,6 pen
//
// ***************************************************************************************

void QDSetPenColour(uint8_t colour) { penColour = colour; }
void QDMoveTo(int16_t x,int16_t y) { penX = x;penY = y; }
void QDGetPen(int16_t *x,int16_t *y,uint8_t *colour) { *x = penX;*y = penY;*colour = penColour; }

// ***************************************************************************************
//
//      32,7 .. 32,10 rectangles (right/bottom exclusive), clipped
//
// ***************************************************************************************

uint8_t QDFrameRect(uint16_t rectAddr) {
    struct QDRect r;
    if (!_QDReadRect(rectAddr,&r)) return QD_ERR_PARAM;
    if (r.right == r.left || r.bottom == r.top) return QD_ERR_OK;                // Empty
    struct QDRect e = { r.left,r.top,r.right,(int16_t)(r.top+1) };_QDFill(&e,penColour);        // Top
    e = { r.left,(int16_t)(r.bottom-1),r.right,r.bottom };_QDFill(&e,penColour);                // Bottom
    e = { r.left,r.top,(int16_t)(r.left+1),r.bottom };_QDFill(&e,penColour);                    // Left
    e = { (int16_t)(r.right-1),r.top,r.right,r.bottom };_QDFill(&e,penColour);                  // Right
    return QD_ERR_OK;
}

uint8_t QDPaintRect(uint16_t rectAddr) {
    struct QDRect r;
    if (!_QDReadRect(rectAddr,&r)) return QD_ERR_PARAM;
    _QDFill(&r,penColour);
    return QD_ERR_OK;
}

uint8_t QDEraseRect(uint16_t rectAddr,uint8_t colour) {
    struct QDRect r;
    if (!_QDReadRect(rectAddr,&r)) return QD_ERR_PARAM;
    _QDFill(&r,colour);
    return QD_ERR_OK;
}

uint8_t QDInvertRect(uint16_t rectAddr) {
    struct QDRect r;
    if (!_QDReadRect(rectAddr,&r)) return QD_ERR_PARAM;
    _QDFill(&r,-1);
    return QD_ERR_OK;
}
