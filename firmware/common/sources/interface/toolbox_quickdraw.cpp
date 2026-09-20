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
static uint8_t pattern[8] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };       // 8x8 fill pattern, bit 7 = left

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
    memset(pattern,0xFF,sizeof(pattern));
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

// ***************************************************************************************
//
//      32,11 LineTo : Bresenham from the pen, every pixel clipped ; pen moves
//
// ***************************************************************************************

static inline bool _QDInClip(int x,int y) {
    return x >= clipRect.left && x < clipRect.right && y >= clipRect.top && y < clipRect.bottom;
}

uint8_t QDLineTo(int16_t x,int16_t y) {
    int x0 = penX,y0 = penY,x1 = x,y1 = y;
    int dx = abs(x1-x0),sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1-y0),sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        if (_QDInClip(x0,y0)) GFXWritePixelRaw(x0,y0,penColour);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy;x0 += sx; }
        if (e2 <= dx) { err += dx;y0 += sy; }
    }
    penX = x;penY = y;
    return QD_ERR_OK;
}

// ***************************************************************************************
//
//      32,12 / 32,13 pattern fill : bit set = pen colour, clear = background colour
//
// ***************************************************************************************

uint8_t QDSetPattern(uint16_t patAddr) {
    if (patAddr > 0xFF00 - 8) return QD_ERR_PARAM;
    memcpy(pattern,cpuMemory + patAddr,8);
    return QD_ERR_OK;
}

uint8_t QDFillRect(uint16_t rectAddr,uint8_t backColour) {
    struct QDRect r;
    if (!_QDReadRect(rectAddr,&r)) return QD_ERR_PARAM;
    struct QDRect c = _QDIntersect(&r,&clipRect);
    for (int y = c.top;y < c.bottom;y++) {
        uint8_t row = pattern[y & 7];                                           // Pattern aligned on screen coordinates
        for (int x = c.left;x < c.right;x++) {
            GFXWritePixelRaw(x,y,(row & (0x80 >> (x & 7))) ? penColour : backColour);
        }
    }
    return QD_ERR_OK;
}

// ***************************************************************************************
//
//      32,14 CopyBits : blit a 12,3 source area at (x,y) of the draw page, clipped to
//      the clip rectangle (left clip rounds up to the source byte boundary, as 12,4).
//
// ***************************************************************************************

uint8_t QDCopyBits(uint8_t action,uint16_t areaAddr,int16_t x,int16_t y) {
    if (areaAddr > 0xFF00 - 12 || action > BLTACT_SOLID) return QD_ERR_PARAM;
    struct BlitterArea src;
    BLTLoadArea(areaAddr,&src);
    int unit = (src.format == BLTFMT_BYTE) ? 1 : (src.format == BLTFMT_PAIR) ? 2 : (src.format == BLTFMT_BITS) ? 8 : 0;
    if (unit == 0) return QD_ERR_PARAM;
    int w = src.width,h = src.height;
    if (x < clipRect.left) {                                                    // Left : skip whole source bytes
        int adj = ((clipRect.left - x) + unit - 1) / unit * unit;
        src.address += adj / unit;w -= adj;x += adj;
    }
    if (x + w > clipRect.right) w = clipRect.right - x;
    if (y < clipRect.top) { int adj = clipRect.top - y;h -= adj;y += adj;src.address += adj * src.stride; }
    if (y + h > clipRect.bottom) h = clipRect.bottom - y;
    if (w <= 0 || h <= 0) return QD_ERR_OK;                                     // Fully clipped
    src.width = w;src.height = h;
    if (gMode.bitsPerPixel != 8) return QD_ERR_NOGFX;                          // Blitter targets are byte pixels (mode 0)
    uint32_t offset = (uint32_t)y * gMode.stride + x + (uint32_t)(gMode.graphicsMemory - graphicsMemory);
    struct BlitterArea target;
    target.address = (uint16_t)(offset & 0xFFFF);target.page = (uint8_t)(0x80 + (offset >> 16));
    target.padding = 0;target.stride = gMode.stride;target.format = BLTFMT_BYTE;
    target.transparent = 0;target.solid = 0;target.height = 0;target.width = 0;
    return BLTCopyArea(action,&src,&target) ? QD_ERR_PARAM : QD_ERR_OK;
}

// ***************************************************************************************
//
//      32,15 .. 32,18 proportional fonts (NF1) ; default = the 6x8 system font
//
// ***************************************************************************************

static const uint8_t *fontGlyphs = NULL;                                        // NULL : system font
static uint8_t fontHeight = 8,fontFirst = 32,fontCount = 96,fontSpacing = 0,fontRowBytes = 1;

static bool _QDGlyph(uint8_t ch,const uint8_t **rows,uint8_t *width) {         // Glyph rows and advance width
    if (fontGlyphs == NULL) {
        if (ch < 32 || ch == 127 || (ch >= 0x80 && ch < 0xA0)) return false;
        *rows = CONGlyph(ch);*width = 6;                                        // 6x8 cells, 8 rows, bit 7 left ; $C0-$FF UDG
        return true;
    }
    if (ch < fontFirst || ch >= fontFirst + fontCount) return false;
    const uint8_t *g = fontGlyphs + (ch - fontFirst) * (1 + fontHeight * fontRowBytes);
    *width = g[0];*rows = g + 1;
    return true;
}

uint8_t QDSetFont(uint8_t page,uint16_t addr) {
    if (page == 0 && addr == 0) {                                               // Back to the system font
        fontGlyphs = NULL;fontHeight = 8;fontFirst = 32;fontCount = 96;fontSpacing = 0;fontRowBytes = 1;
        return QD_ERR_OK;
    }
    const uint8_t *h = BLTGetRealAddress(page,addr);
    if (h == NULL || BLTGetRealAddress(page,addr + 7) == NULL) return QD_ERR_PARAM;
    if (h[0] != QD_FONT_MAGIC0 || h[1] != QD_FONT_MAGIC1 || h[2] != QD_FONT_VERSION) return QD_ERR_PARAM;
    uint8_t height = h[3],first = h[4],count = h[5],spacing = h[6],rowBytes = h[7];
    if (height == 0 || count == 0 || (rowBytes != 1 && rowBytes != 2) || first + count > 256) return QD_ERR_PARAM;
    uint32_t size = 8 + (uint32_t)count * (1 + height * rowBytes);
    if (addr + size - 1 > 0xFFFF || BLTGetRealAddress(page,(uint16_t)(addr + size - 1)) == NULL) return QD_ERR_PARAM;
    fontGlyphs = h + 8;fontHeight = height;fontFirst = first;fontCount = count;fontSpacing = spacing;fontRowBytes = rowBytes;
    return QD_ERR_OK;
}

void QDGetFontInfo(uint8_t *height,uint8_t *first,uint8_t *count) {
    *height = fontHeight;*first = fontFirst;*count = fontCount;
}

// Draw one glyph at (x,y) : set bits in the pen colour, transparent elsewhere, clipped.
static void _QDDrawGlyph(const uint8_t *rows,uint8_t width,int x,int y) {
    for (int r = 0;r < fontHeight;r++) {
        int py = y + r;
        if (py < clipRect.top || py >= clipRect.bottom) continue;
        const uint8_t *row = rows + r * fontRowBytes;
        for (int c = 0;c < width;c++) {
            int px = x + c;
            if (px < clipRect.left || px >= clipRect.right) continue;
            if (row[c >> 3] & (0x80 >> (c & 7))) GFXWritePixelRaw(px,py,penColour);
        }
    }
}

uint8_t QDDrawString(uint16_t strAddr) {
    if (strAddr > 0xFF00 - 1) return QD_ERR_PARAM;
    uint8_t len = cpuMemory[strAddr];
    if ((uint32_t)strAddr + len > 0xFF00 - 1) return QD_ERR_PARAM;
    for (int i = 0;i < len;i++) {
        const uint8_t *rows;uint8_t width;
        if (!_QDGlyph(cpuMemory[strAddr + 1 + i],&rows,&width)) continue;      // Unknown : no advance
        _QDDrawGlyph(rows,width,penX,penY);
        penX += width + fontSpacing;
    }
    return QD_ERR_OK;
}

uint8_t QDTextWidth(uint16_t strAddr,uint16_t *width) {
    if (strAddr > 0xFF00 - 1) return QD_ERR_PARAM;
    uint8_t len = cpuMemory[strAddr];
    if ((uint32_t)strAddr + len > 0xFF00 - 1) return QD_ERR_PARAM;
    uint16_t w = 0;
    for (int i = 0;i < len;i++) {
        const uint8_t *rows;uint8_t gw;
        if (_QDGlyph(cpuMemory[strAddr + 1 + i],&rows,&gw)) w += gw + fontSpacing;
    }
    *width = w;
    return QD_ERR_OK;
}

// ***************************************************************************************
//
//      Firmware-side helpers (Window Manager)
//
// ***************************************************************************************

void QDGetClipRaw(struct QDRect *r) { *r = clipRect; }
void QDSetClipRaw(const struct QDRect *r) { struct QDRect s = _QDScreenRect();clipRect = _QDIntersect(r,&s); }
void QDFillRaw(const struct QDRect *r,int colour) { _QDFill(r,colour); }

void QDFrameRaw(const struct QDRect *r,uint8_t colour) {
    if (r->right <= r->left || r->bottom <= r->top) return;
    struct QDRect e = { r->left,r->top,r->right,(int16_t)(r->top+1) };_QDFill(&e,colour);
    e = { r->left,(int16_t)(r->bottom-1),r->right,r->bottom };_QDFill(&e,colour);
    e = { r->left,r->top,(int16_t)(r->left+1),r->bottom };_QDFill(&e,colour);
    e = { (int16_t)(r->right-1),r->top,r->right,r->bottom };_QDFill(&e,colour);
}

void QDTextRaw(const uint8_t *text,uint8_t len,int x,int y,uint8_t colour) {
    uint8_t saveColour = penColour;penColour = colour;
    for (int i = 0;i < len;i++) {
        uint8_t ch = text[i];
        if (ch < 32 || ch == 127 || (ch >= 0x80 && ch < 0xA0)) continue;
        const uint8_t *save = fontGlyphs;uint8_t h = fontHeight,rb = fontRowBytes;
        fontGlyphs = NULL;fontHeight = 8;fontRowBytes = 1;                        // System font, whatever is selected
        _QDDrawGlyph(CONGlyph(ch),6,x,y);
        fontGlyphs = save;fontHeight = h;fontRowBytes = rb;
        x += 6;
    }
    penColour = saveColour;
}
