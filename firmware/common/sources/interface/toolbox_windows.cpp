// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      toolbox_windows.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      17th September 2026
//      Purpose :   Toolbox group 34, Window Manager (see windows.h).
//
//      Any change of the window set (new, dispose, show, hide, select, move, size) repaints
//      every visible frame back to front, erases the content areas and posts an update event
//      per visible window, back to front : the 6502 redraws each content (clip = 34,12) and
//      calls 34,11 End Update, which repaints the frames of the windows above it.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

struct Window {
    bool used,visible;
    uint8_t flags;
    struct QDRect frame;                                                        // Screen coordinates, exclusive
    uint8_t titleLen;
    uint8_t title[WM_TITLE_MAX];
};

static struct Window windows[WM_MAX_WINDOWS];                                   // id = index + 1
static uint8_t zOrder[WM_MAX_WINDOWS];                                          // ids, back (0) to front
static uint8_t zCount = 0;
static uint8_t frontId = 0;                                                     // Last activated front (for activate events)

// ***************************************************************************************
//
//      Geometry
//
// ***************************************************************************************

static struct Window *_WMGet(uint8_t id) {
    if (id < 1 || id > WM_MAX_WINDOWS || !windows[id-1].used) return NULL;
    return &windows[id-1];
}

static struct QDRect _WMContentRect(const struct Window *w) {
    struct QDRect r = w->frame;
    r.left += 1;r.right -= 1;r.bottom -= 1;
    r.top += (w->flags & WM_FLAG_TITLE) ? 1 + WM_TITLE_HEIGHT : 1;
    if (r.right < r.left) r.right = r.left;
    if (r.bottom < r.top) r.bottom = r.top;
    return r;
}

static bool _WMIntersects(const struct QDRect *a,const struct QDRect *b) {
    return a->left < b->right && b->left < a->right && a->top < b->bottom && b->top < a->bottom;
}

static int _WMZIndex(uint8_t id) {
    for (int i = 0;i < zCount;i++) if (zOrder[i] == id) return i;
    return -1;
}

static void _WMBringToFront(uint8_t id) {
    int i = _WMZIndex(id);
    if (i < 0) return;
    for (;i < zCount-1;i++) zOrder[i] = zOrder[i+1];
    zOrder[zCount-1] = id;
}

static void _WMRemoveZ(uint8_t id) {
    int i = _WMZIndex(id);
    if (i < 0) return;
    for (;i < zCount-1;i++) zOrder[i] = zOrder[i+1];
    zCount--;
}

static uint8_t _WMVisibleFront(void) {
    for (int i = zCount-1;i >= 0;i--) if (windows[zOrder[i]-1].visible) return zOrder[i];
    return 0;
}

// ***************************************************************************************
//
//      Drawing (frames only ; contents belong to the 6502)
//
// ***************************************************************************************

static void _WMDrawFrame(const struct Window *w,bool isFront) {
    struct QDRect screen = { 0,0,(int16_t)gMode.xGSize,(int16_t)gMode.yGSize };
    QDSetClipRaw(&screen);
    QDFrameRaw(&w->frame,WM_COL_FRAME);
    if (w->flags & WM_FLAG_TITLE) {
        struct QDRect bar = { (int16_t)(w->frame.left+1),(int16_t)(w->frame.top+1),(int16_t)(w->frame.right-1),(int16_t)(w->frame.top+1+WM_TITLE_HEIGHT) };
        QDSetClipRaw(&bar);
        QDFillRaw(&bar,isFront ? WM_COL_TITLE_ON : WM_COL_TITLE_OFF);
        int x = bar.left + 2;
        if (w->flags & WM_FLAG_CLOSE) {                                         // Close box : 7x7 square
            struct QDRect box = { (int16_t)(bar.left+2),(int16_t)(bar.top+1),(int16_t)(bar.left+9),(int16_t)(bar.top+8) };
            QDFrameRaw(&box,WM_COL_TEXT);
            x = bar.left + 12;
        }
        QDTextRaw(w->title,w->titleLen,x,bar.top+1,WM_COL_TEXT);
    }
    if (w->flags & WM_FLAG_GROW) {                                              // Grow box : bottom right of the content
        struct QDRect c = _WMContentRect(w);
        struct QDRect g = { (int16_t)(c.right-WM_GROW_SIZE),(int16_t)(c.bottom-WM_GROW_SIZE),c.right,c.bottom };
        QDSetClipRaw(&c);
        QDFrameRaw(&g,WM_COL_FRAME);
    }
}

// Repaint everything : frames back to front, contents erased, one update event per visible window (back to front).
static void _WMRepaintAll(void) {
    struct QDRect save;QDGetClipRaw(&save);
    uint8_t front = _WMVisibleFront();
    for (int i = 0;i < zCount;i++) {
        struct Window *w = &windows[zOrder[i]-1];
        if (!w->visible) continue;
        struct QDRect c = _WMContentRect(w);
        struct QDRect screen = { 0,0,(int16_t)gMode.xGSize,(int16_t)gMode.yGSize };
        QDSetClipRaw(&screen);
        QDFillRaw(&c,WM_COL_CONTENT);
        _WMDrawFrame(w,zOrder[i] == front);
        EVTPostWindow(EVT_UPDATE,zOrder[i],0);
    }
    if (front != frontId) {
        if (frontId != 0 && _WMGet(frontId) != NULL) EVTPostWindow(EVT_ACTIVATE,frontId,0);
        if (front != 0) EVTPostWindow(EVT_ACTIVATE,front,1);
        frontId = front;
    }
    QDSetClipRaw(&save);
}

// Erase the area a window leaves behind (the desktop is colour 0 ; windows below get an update).
static void _WMRepaintAfterChange(void) { _WMRepaintAll(); }

// ***************************************************************************************
//
//      API
//
// ***************************************************************************************

void WMReset(void) {
    for (int i = 0;i < WM_MAX_WINDOWS;i++) windows[i].used = false;
    zCount = 0;frontId = 0;
}

static bool _WMReadRect(uint16_t addr,struct QDRect *r) {
    if (addr > 0xFF00 - 8) return false;
    const uint8_t *p = cpuMemory + addr;
    r->left = (int16_t)(p[0] | (p[1] << 8));r->top = (int16_t)(p[2] | (p[3] << 8));
    r->right = (int16_t)(p[4] | (p[5] << 8));r->bottom = (int16_t)(p[6] | (p[7] << 8));
    return r->right > r->left && r->bottom > r->top;
}

static bool _WMReadTitle(uint16_t addr,struct Window *w) {
    if (addr > 0xFF00 - 1) return false;
    uint8_t len = cpuMemory[addr];
    if (len > WM_TITLE_MAX) len = WM_TITLE_MAX;
    if ((uint32_t)addr + len > 0xFF00 - 1) return false;
    memcpy(w->title,cpuMemory + addr + 1,len);
    w->titleLen = len;
    return true;
}

uint8_t WMNewWindow(uint16_t rectAddr,uint16_t titleAddr,uint8_t flags,uint8_t *id) {
    int slot = -1;
    for (int i = 0;i < WM_MAX_WINDOWS;i++) if (!windows[i].used) { slot = i;break; }
    if (slot < 0) return 2;                                                     // No free window
    struct Window *w = &windows[slot];
    if (!_WMReadRect(rectAddr,&w->frame)) return 1;
    if (!_WMReadTitle(titleAddr,w)) return 1;
    w->flags = flags;w->used = true;w->visible = true;
    zOrder[zCount++] = slot + 1;
    *id = slot + 1;
    _WMRepaintAfterChange();
    return 0;
}

uint8_t WMDisposeWindow(uint8_t id) {
    struct Window *w = _WMGet(id);
    if (w == NULL) return 1;
    struct QDRect save;QDGetClipRaw(&save);
    struct QDRect screen = { 0,0,(int16_t)gMode.xGSize,(int16_t)gMode.yGSize };
    QDSetClipRaw(&screen);QDFillRaw(&w->frame,WM_COL_CONTENT);QDSetClipRaw(&save);    // Clear what it covered
    w->used = false;_WMRemoveZ(id);
    _WMRepaintAfterChange();
    return 0;
}

uint8_t WMShowWindow(uint8_t id,uint8_t visible) {
    struct Window *w = _WMGet(id);
    if (w == NULL) return 1;
    if (w->visible == (visible != 0)) return 0;
    w->visible = (visible != 0);
    if (!w->visible) {
        struct QDRect save;QDGetClipRaw(&save);
        struct QDRect screen = { 0,0,(int16_t)gMode.xGSize,(int16_t)gMode.yGSize };
        QDSetClipRaw(&screen);QDFillRaw(&w->frame,WM_COL_CONTENT);QDSetClipRaw(&save);
    }
    _WMRepaintAfterChange();
    return 0;
}

uint8_t WMSelectWindow(uint8_t id) {
    struct Window *w = _WMGet(id);
    if (w == NULL) return 1;
    _WMBringToFront(id);
    w->visible = true;
    _WMRepaintAfterChange();
    return 0;
}

uint8_t WMMoveWindow(uint8_t id,int16_t x,int16_t y) {
    struct Window *w = _WMGet(id);
    if (w == NULL) return 1;
    struct QDRect save;QDGetClipRaw(&save);
    struct QDRect screen = { 0,0,(int16_t)gMode.xGSize,(int16_t)gMode.yGSize };
    QDSetClipRaw(&screen);QDFillRaw(&w->frame,WM_COL_CONTENT);QDSetClipRaw(&save);
    int16_t dw = w->frame.right - w->frame.left,dh = w->frame.bottom - w->frame.top;
    w->frame.left = x;w->frame.top = y;w->frame.right = x + dw;w->frame.bottom = y + dh;
    _WMRepaintAfterChange();
    return 0;
}

uint8_t WMSizeWindow(uint8_t id,int16_t width,int16_t height) {
    struct Window *w = _WMGet(id);
    if (w == NULL || width < 3 || height < 3) return 1;
    struct QDRect save;QDGetClipRaw(&save);
    struct QDRect screen = { 0,0,(int16_t)gMode.xGSize,(int16_t)gMode.yGSize };
    QDSetClipRaw(&screen);QDFillRaw(&w->frame,WM_COL_CONTENT);QDSetClipRaw(&save);
    w->frame.right = w->frame.left + width;w->frame.bottom = w->frame.top + height;
    _WMRepaintAfterChange();
    return 0;
}

void WMFindWindow(int16_t x,int16_t y,uint8_t *id,uint8_t *part) {
    *id = 0;*part = WM_PART_NONE;
    for (int i = zCount-1;i >= 0;i--) {                                         // Front to back
        struct Window *w = &windows[zOrder[i]-1];
        if (!w->visible) continue;
        if (x < w->frame.left || x >= w->frame.right || y < w->frame.top || y >= w->frame.bottom) continue;
        *id = zOrder[i];
        struct QDRect c = _WMContentRect(w);
        if (x >= c.left && x < c.right && y >= c.top && y < c.bottom) {
            *part = WM_PART_CONTENT;
            if ((w->flags & WM_FLAG_GROW) && x >= c.right - WM_GROW_SIZE && y >= c.bottom - WM_GROW_SIZE) *part = WM_PART_GROW;
        } else if ((w->flags & WM_FLAG_TITLE) && y < c.top) {
            *part = WM_PART_DRAG;
            if ((w->flags & WM_FLAG_CLOSE) && x >= w->frame.left + 3 && x < w->frame.left + 10 && y >= w->frame.top + 2 && y < w->frame.top + 9) *part = WM_PART_CLOSE;
        } else *part = WM_PART_DRAG;                                            // Frame line
        return;
    }
}

uint8_t WMGetContentRect(uint8_t id,uint16_t rectAddr) {
    struct Window *w = _WMGet(id);
    if (w == NULL || rectAddr > 0xFF00 - 8) return 1;
    struct QDRect c = _WMContentRect(w);
    uint8_t *p = cpuMemory + rectAddr;
    p[0] = c.left & 0xFF;p[1] = c.left >> 8;p[2] = c.top & 0xFF;p[3] = c.top >> 8;
    p[4] = c.right & 0xFF;p[5] = c.right >> 8;p[6] = c.bottom & 0xFF;p[7] = c.bottom >> 8;
    return 0;
}

uint8_t WMSetTitle(uint8_t id,uint16_t titleAddr) {
    struct Window *w = _WMGet(id);
    if (w == NULL || !_WMReadTitle(titleAddr,w)) return 1;
    if (w->visible) { struct QDRect save;QDGetClipRaw(&save);_WMDrawFrame(w,id == _WMVisibleFront());QDSetClipRaw(&save); }
    return 0;
}

uint8_t WMFrontWindow(void) { return _WMVisibleFront(); }

// After the 6502 redrew the content of a window : repaint the frames of the visible windows above it.
uint8_t WMEndUpdate(uint8_t id) {
    struct Window *w = _WMGet(id);
    if (w == NULL) return 1;
    int z = _WMZIndex(id);
    struct QDRect save;QDGetClipRaw(&save);
    uint8_t front = _WMVisibleFront();
    for (int i = z+1;i < zCount;i++) {
        struct Window *above = &windows[zOrder[i]-1];
        if (above->visible && _WMIntersects(&above->frame,&w->frame)) _WMDrawFrame(above,zOrder[i] == front);
    }
    QDSetClipRaw(&save);
    return 0;
}

// QuickDraw clip = content of the window (the 6502 then draws with group 32).
uint8_t WMSetPort(uint8_t id) {
    struct Window *w = _WMGet(id);
    if (w == NULL) return 1;
    struct QDRect c = _WMContentRect(w);
    QDSetClipRaw(&c);
    return 0;
}

// An area of the screen was erased (a pull-down menu closed) : the visible windows it touched get their
// content erased, their frame repainted and an update event, back to front.
void WMInvalidate(const struct QDRect *r) {
    struct QDRect save;QDGetClipRaw(&save);
    uint8_t front = _WMVisibleFront();
    for (int i = 0;i < zCount;i++) {
        struct Window *w = &windows[zOrder[i]-1];
        if (!w->visible || !_WMIntersects(&w->frame,r)) continue;
        struct QDRect c = _WMContentRect(w);
        struct QDRect screen = { 0,0,(int16_t)gMode.xGSize,(int16_t)gMode.yGSize };
        QDSetClipRaw(&screen);
        QDFillRaw(&c,WM_COL_CONTENT);
        _WMDrawFrame(w,zOrder[i] == front);
        EVTPostWindow(EVT_UPDATE,zOrder[i],0);
    }
    QDSetClipRaw(&save);
}
