// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      toolbox_controls.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      18th September 2026
//      Purpose :   Toolbox group 36, Control Manager (see controls.h).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

struct Control {
    uint8_t kind,window,flags,part;                                             // part : part being tracked
    struct QDRect rect;                                                         // Relative to the window content
    const uint8_t *text;                                                        // Title (ptext) or text buffer (ptext) : 6502 RAM, or firmware (alerts, F-45)
    int16_t value,max;                                                          // Scroll bar : 0..max ; text : max length ; check/radio : 0/1
};

static struct Control controls[CT_MAX_CONTROLS];                                // id = index + 1

static struct Control *_CTGet(uint8_t id) {
    if (id < 1 || id > CT_MAX_CONTROLS || controls[id-1].kind == 0) return NULL;
    return &controls[id-1];
}

// Screen rectangle of a control (content rect of its window + relative rect), false if the window is gone.
static bool _CTScreenRect(const struct Control *c,struct QDRect *r) {
    struct QDRect content;
    if (!WMContentRectOf(c->window,&content)) return false;
    r->left = content.left + c->rect.left;r->top = content.top + c->rect.top;
    r->right = content.left + c->rect.right;r->bottom = content.top + c->rect.bottom;
    return true;
}

static bool _CTVertical(const struct Control *c) { return (c->rect.bottom - c->rect.top) > (c->rect.right - c->rect.left); }

// ***************************************************************************************
//
//      Drawing (clip = content of the window, set by the caller through 34,12 or here)
//
// ***************************************************************************************

static void _CTDrawOne(const struct Control *c) {
    struct QDRect r,content;
    if (!_CTScreenRect(c,&r) || !WMContentRectOf(c->window,&content)) return;
    if (!WMIsVisible(c->window)) return;
    struct QDRect save;QDGetClipRaw(&save);
    QDSetClipRaw(&content);
    uint8_t ink = (c->flags & CT_FLAG_DISABLED) ? CT_COL_DISABLED : CT_COL_TEXT;
    const uint8_t *title = c->text + 1;uint8_t len = c->text[0];
    switch (c->kind) {
        case CT_KIND_BUTTON: {
            bool hi = (c->flags & CT_FLAG_HILITE) != 0;
            QDFillRaw(&r,hi ? ink : CT_COL_BACK);
            QDFrameRaw(&r,ink);
            int tx = r.left + ((r.right - r.left) - len * 6) / 2,ty = r.top + ((r.bottom - r.top) - 8) / 2;
            QDTextRaw(title,len,tx,ty,hi ? CT_COL_BACK : ink);
            break;
        }
        case CT_KIND_CHECK:
        case CT_KIND_RADIO: {
            struct QDRect box = { r.left,r.top,(int16_t)(r.left+8),(int16_t)(r.top+8) };
            struct QDRect clear = { r.left,r.top,r.right,r.bottom };
            QDFillRaw(&clear,CT_COL_BACK);
            QDFrameRaw(&box,ink);
            if (c->value) {
                struct QDRect mark = (c->kind == CT_KIND_CHECK) ? (struct QDRect){ (int16_t)(r.left+2),(int16_t)(r.top+2),(int16_t)(r.left+6),(int16_t)(r.top+6) }
                                                                : (struct QDRect){ (int16_t)(r.left+3),(int16_t)(r.top+3),(int16_t)(r.left+5),(int16_t)(r.top+5) };
                QDFillRaw(&mark,ink);
            }
            QDTextRaw(title,len,r.left + 11,r.top,ink);
            break;
        }
        case CT_KIND_SCROLL: {
            QDFillRaw(&r,CT_COL_BACK);
            QDFrameRaw(&r,ink);
            bool v = _CTVertical(c);
            int len2 = v ? (r.bottom - r.top) : (r.right - r.left);
            int track = len2 - 2 * CT_ARROW;
            struct QDRect a1 = v ? (struct QDRect){ r.left,r.top,r.right,(int16_t)(r.top+CT_ARROW) } : (struct QDRect){ r.left,r.top,(int16_t)(r.left+CT_ARROW),r.bottom };
            struct QDRect a2 = v ? (struct QDRect){ r.left,(int16_t)(r.bottom-CT_ARROW),r.right,r.bottom } : (struct QDRect){ (int16_t)(r.right-CT_ARROW),r.top,r.right,r.bottom };
            QDFrameRaw(&a1,ink);QDFrameRaw(&a2,ink);
            if (track > 4 && c->max > 0) {                                      // Thumb : 4 pixels long, at value/max
                int pos = (int)((long)(track - 4) * c->value / c->max);
                struct QDRect th = v ? (struct QDRect){ (int16_t)(r.left+1),(int16_t)(r.top+CT_ARROW+pos),(int16_t)(r.right-1),(int16_t)(r.top+CT_ARROW+pos+4) }
                                     : (struct QDRect){ (int16_t)(r.left+CT_ARROW+pos),(int16_t)(r.top+1),(int16_t)(r.left+CT_ARROW+pos+4),(int16_t)(r.bottom-1) };
                QDFillRaw(&th,ink);
            }
            break;
        }
        case CT_KIND_TEXT: {
            QDFillRaw(&r,CT_COL_BACK);
            QDFrameRaw(&r,ink);
            struct QDRect inner = { (int16_t)(r.left+1),(int16_t)(r.top+1),(int16_t)(r.right-1),(int16_t)(r.bottom-1) };
            struct QDRect c2 = { std::max(inner.left,content.left),std::max(inner.top,content.top),std::min(inner.right,content.right),std::min(inner.bottom,content.bottom) };
            QDSetClipRaw(&c2);
            QDTextRaw(title,len,r.left + 2,r.top + 2,ink);
            struct QDRect cursor = { (int16_t)(r.left+2+len*6),(int16_t)(r.top+2),(int16_t)(r.left+3+len*6),(int16_t)(r.top+10) };
            if (!(c->flags & CT_FLAG_DISABLED)) QDFillRaw(&cursor,ink);
            break;
        }
    }
    QDSetClipRaw(&save);
}

// ***************************************************************************************
//
//      API
//
// ***************************************************************************************

void CTReset(void) { for (int i = 0;i < CT_MAX_CONTROLS;i++) controls[i].kind = 0; }

void CTWindowDisposed(uint8_t window) { for (int i = 0;i < CT_MAX_CONTROLS;i++) if (controls[i].window == window) controls[i].kind = 0; }

// Common creation : rect in firmware memory, ptext anywhere (6502 RAM or a firmware constant, F-45 alerts).
uint8_t CTNewRaw(uint8_t kind,uint8_t window,const struct QDRect *rect,const uint8_t *ptext,uint16_t max,uint8_t *id) {
    if (kind < CT_KIND_BUTTON || kind > CT_KIND_TEXT) return 1;
    struct QDRect content;
    if (!WMContentRectOf(window,&content)) return 1;
    if (rect->right <= rect->left || rect->bottom <= rect->top) return 1;
    if (kind == CT_KIND_TEXT && ptext[0] > max) return 1;                       // Buffer longer than its max
    int slot = -1;
    for (int i = 0;i < CT_MAX_CONTROLS;i++) if (controls[i].kind == 0) { slot = i;break; }
    if (slot < 0) return 2;
    struct Control *c = &controls[slot];
    c->rect = *rect;
    c->kind = kind;c->window = window;c->flags = 0;c->part = 0;c->text = ptext;c->value = 0;c->max = (int16_t)max;
    *id = slot + 1;
    _CTDrawOne(c);
    return 0;
}

uint8_t CTNew(uint8_t kind,uint8_t window,uint16_t rectAddr,uint16_t textAddr,uint16_t max,uint8_t *id) {
    if (rectAddr > 0xFF00 - 8 || textAddr > 0xFF00 - 1) return 1;
    struct QDRect r;
    const uint8_t *p = cpuMemory + rectAddr;
    r.left = (int16_t)(p[0] | (p[1] << 8));r.top = (int16_t)(p[2] | (p[3] << 8));
    r.right = (int16_t)(p[4] | (p[5] << 8));r.bottom = (int16_t)(p[6] | (p[7] << 8));
    return CTNewRaw(kind,window,&r,cpuMemory + textAddr,max,id);
}

uint8_t CTDispose(uint8_t id) {
    struct Control *c = _CTGet(id);
    if (c == NULL) return 1;
    struct QDRect r,content;
    if (_CTScreenRect(c,&r) && WMContentRectOf(c->window,&content) && WMIsVisible(c->window)) {
        struct QDRect save;QDGetClipRaw(&save);QDSetClipRaw(&content);QDFillRaw(&r,CT_COL_BACK);QDSetClipRaw(&save);
    }
    c->kind = 0;
    return 0;
}

uint8_t CTDrawAll(uint8_t window) {
    struct QDRect content;
    if (!WMContentRectOf(window,&content)) return 1;
    for (int i = 0;i < CT_MAX_CONTROLS;i++) if (controls[i].kind != 0 && controls[i].window == window) _CTDrawOne(&controls[i]);
    return 0;
}

uint8_t CTDraw(uint8_t id) {
    struct Control *c = _CTGet(id);
    if (c == NULL) return 1;
    _CTDrawOne(c);
    return 0;
}

uint8_t CTSetValue(uint8_t id,int16_t value) {
    struct Control *c = _CTGet(id);
    if (c == NULL) return 1;
    if (c->kind == CT_KIND_SCROLL) { if (value < 0) value = 0;if (value > c->max) value = c->max; }
    else if (c->kind == CT_KIND_CHECK || c->kind == CT_KIND_RADIO) value = value ? 1 : 0;
    c->value = value;
    _CTDrawOne(c);
    return 0;
}

uint8_t CTGetKind(uint8_t id) { struct Control *c = _CTGet(id);return c == NULL ? 0 : c->kind; }

uint8_t CTGetValue(uint8_t id,int16_t *value) {
    struct Control *c = _CTGet(id);
    if (c == NULL) return 1;
    *value = c->value;
    return 0;
}

uint8_t CTSetFlags(uint8_t id,uint8_t flags) {
    struct Control *c = _CTGet(id);
    if (c == NULL) return 1;
    c->flags = (c->flags & CT_FLAG_HILITE) | (flags & ~CT_FLAG_HILITE);
    _CTDrawOne(c);
    return 0;
}

static uint8_t _CTPartAt(const struct Control *c,const struct QDRect *r,int16_t x,int16_t y) {
    if (x < r->left || x >= r->right || y < r->top || y >= r->bottom) return CT_PART_NONE;
    if (c->kind != CT_KIND_SCROLL) return CT_PART_BODY;
    bool v = _CTVertical(c);
    int pos = v ? (y - r->top) : (x - r->left);
    int len = v ? (r->bottom - r->top) : (r->right - r->left);
    if (pos < CT_ARROW) return CT_PART_UP;
    if (pos >= len - CT_ARROW) return CT_PART_DOWN;
    int track = len - 2 * CT_ARROW;
    int thumb = (c->max > 0 && track > 4) ? (int)((long)(track - 4) * c->value / c->max) : 0;
    int p = pos - CT_ARROW;
    if (p >= thumb && p < thumb + 4) return CT_PART_THUMB;
    return (p < thumb) ? CT_PART_PAGEUP : CT_PART_PAGEDOWN;
}

void CTFind(int16_t x,int16_t y,uint8_t *id,uint8_t *part) {
    *id = 0;*part = CT_PART_NONE;
    uint8_t win,wpart;
    WMFindWindow(x,y,&win,&wpart);                                              // Only the frontmost window under the point
    if (win == 0 || wpart != WM_PART_CONTENT) return;
    for (int i = 0;i < CT_MAX_CONTROLS;i++) {
        const struct Control *c = &controls[i];
        if (c->kind == 0 || c->window != win) continue;
        struct QDRect r;
        if (!_CTScreenRect(c,&r)) continue;
        uint8_t p = _CTPartAt(c,&r,x,y);
        if (p != CT_PART_NONE) { *id = i + 1;*part = p;return; }
    }
}

// Tracking by phases. Returns acted = 1 when the control fired (button released inside, box toggled,
// scroll bar value changed, text field clicked).
uint8_t CTTrack(uint8_t id,int16_t x,int16_t y,uint8_t phase,uint8_t *acted) {
    struct Control *c = _CTGet(id);
    *acted = 0;
    if (c == NULL) return 1;
    if (c->flags & CT_FLAG_DISABLED) return 0;
    struct QDRect r;
    if (!_CTScreenRect(c,&r)) return 1;
    uint8_t part = _CTPartAt(c,&r,x,y);
    switch (c->kind) {
        case CT_KIND_BUTTON: {
            bool inside = part != CT_PART_NONE;
            if (phase == CT_PHASE_UP) {
                if (c->flags & CT_FLAG_HILITE) { c->flags &= ~CT_FLAG_HILITE;_CTDrawOne(c); }
                if (inside) *acted = 1;
            } else {
                bool hi = (c->flags & CT_FLAG_HILITE) != 0;
                if (inside != hi) { c->flags = inside ? (c->flags | CT_FLAG_HILITE) : (c->flags & ~CT_FLAG_HILITE);_CTDrawOne(c); }
            }
            break;
        }
        case CT_KIND_CHECK:
        case CT_KIND_RADIO:
            if (phase == CT_PHASE_UP && part != CT_PART_NONE) {
                c->value = (c->kind == CT_KIND_CHECK) ? !c->value : 1;
                _CTDrawOne(c);*acted = 1;
            }
            break;
        case CT_KIND_SCROLL: {
            int16_t v = c->value;
            int16_t page = c->max / 10;if (page < 1) page = 1;
            if (phase == CT_PHASE_DOWN) {
                c->part = part;
                if (part == CT_PART_UP) v--;
                else if (part == CT_PART_DOWN) v++;
                else if (part == CT_PART_PAGEUP) v -= page;
                else if (part == CT_PART_PAGEDOWN) v += page;
            }
            if (c->part == CT_PART_THUMB && (phase == CT_PHASE_MOVE || phase == CT_PHASE_UP)) {      // Drag : value follows the pointer
                bool vert = _CTVertical(c);
                int len = vert ? (r.bottom - r.top) : (r.right - r.left);
                int track = len - 2 * CT_ARROW - 4;
                int pos = (vert ? (y - r.top) : (x - r.left)) - CT_ARROW - 2;
                if (track > 0) v = (int16_t)((long)pos * c->max / track);
            }
            if (phase == CT_PHASE_UP) c->part = 0;
            if (v < 0) v = 0;
            if (v > c->max) v = c->max;
            if (v != c->value) { c->value = v;_CTDrawOne(c);*acted = 1; }
            break;
        }
        case CT_KIND_TEXT:
            if (phase == CT_PHASE_UP && part != CT_PART_NONE) *acted = 1;
            break;
    }
    return 0;
}

// Text fields : printable characters are appended, 8 (backspace) removes the last one. The buffer in
// 6502 RAM is a length-prefixed string of at most max characters.
uint8_t CTKey(uint8_t id,uint8_t key,uint8_t *changed) {
    struct Control *c = _CTGet(id);
    *changed = 0;
    if (c == NULL || c->kind != CT_KIND_TEXT) return 1;
    if (c->flags & CT_FLAG_DISABLED) return 0;
    uint8_t *len = (uint8_t *)c->text;                                          // Text fields always live in 6502 RAM
    if (key == 8 || key == 127) { if (*len > 0) { (*len)--;*changed = 1; } }
    else if (key >= 32 && key < 127) { if (*len < c->max && *len < 255) { len[1 + *len] = key;(*len)++;*changed = 1; } }
    if (*changed) _CTDrawOne(c);
    return 0;
}
