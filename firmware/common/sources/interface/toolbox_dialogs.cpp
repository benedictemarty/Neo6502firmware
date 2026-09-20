// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      toolbox_dialogs.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      18th September 2026
//      Purpose :   Toolbox group 37, Dialog Manager (see dialogs.h).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

struct DialogItem {
    uint8_t kind,flags,control;                                                 // control = 0 for static text
    uint16_t addr;                                                              // Item start in the descriptor (alerts : message ptext)
};

struct Dialog {
    uint8_t window;                                                             // 0 = unused
    bool alert;
    uint8_t count,tracking,focus;                                               // tracking / focus : item number, 0 none
    struct DialogItem items[DL_MAX_ITEMS];
};

static struct Dialog dialogs[DL_MAX_DIALOGS];                                   // id = index + 1

static const uint8_t alertOK[] = { 2,'O','K' };
static const uint8_t alertCancel[] = { 6,'C','a','n','c','e','l' };
static const uint8_t alertYes[] = { 3,'Y','e','s' };
static const uint8_t alertNo[] = { 2,'N','o' };

static struct Dialog *_DLGet(uint8_t id) {
    if (id < 1 || id > DL_MAX_DIALOGS || dialogs[id-1].window == 0) return NULL;
    return &dialogs[id-1];
}

static void _DLReadRect(uint16_t addr,struct QDRect *r) {
    const uint8_t *p = cpuMemory + addr;
    r->left = (int16_t)(p[0] | (p[1] << 8));r->top = (int16_t)(p[2] | (p[3] << 8));
    r->right = (int16_t)(p[4] | (p[5] << 8));r->bottom = (int16_t)(p[6] | (p[7] << 8));
}

// ***************************************************************************************
//
//      Drawing : static texts and the ring of the default button (controls draw themselves)
//
// ***************************************************************************************

static void _DLDrawText(const struct QDRect *content,const struct QDRect *r,const uint8_t *ptext,uint8_t colour) {
    struct QDRect clip = { std::max(content->left,(int16_t)(content->left + r->left)),std::max(content->top,(int16_t)(content->top + r->top)),
                           std::min(content->right,(int16_t)(content->left + r->right)),std::min(content->bottom,(int16_t)(content->top + r->bottom)) };
    QDSetClipRaw(&clip);
    QDFillRaw(&clip,CT_COL_BACK);
    int y = content->top + r->top;
    const uint8_t *p = ptext + 1;uint8_t left = ptext[0];
    while (left > 0) {                                                          // One line per 13
        uint8_t n = 0;
        while (n < left && p[n] != 13) n++;
        QDTextRaw(p,n,content->left + r->left,y,colour);
        y += 8;
        if (n < left) n++;
        p += n;left -= n;
    }
}

static void _DLDrawStatics(const struct Dialog *d) {
    struct QDRect content;
    if (!WMContentRectOf(d->window,&content) || !WMIsVisible(d->window)) return;
    struct QDRect save;QDGetClipRaw(&save);
    for (int i = 0;i < d->count;i++) {
        const struct DialogItem *it = &d->items[i];
        if (it->kind == DL_KIND_STATIC) {
            struct QDRect r;
            if (d->alert) r = (struct QDRect){ DL_MARGIN,DL_MARGIN,(int16_t)(content.right - content.left),(int16_t)(content.bottom - content.top - DL_BUTTON_H - 2 * DL_MARGIN) };
            else _DLReadRect(it->addr + 2,&r);
            _DLDrawText(&content,&r,cpuMemory + it->addr + (d->alert ? 0 : 12),(it->flags & DL_ITEM_DISABLED) ? CT_COL_DISABLED : CT_COL_TEXT);
        } else if (it->kind == CT_KIND_BUTTON && (it->flags & DL_ITEM_DEFAULT)) {  // Ring 2 pixels outside the button
            struct QDRect r;
            if (d->alert) r = (struct QDRect){ (int16_t)(content.right - content.left - DL_MARGIN - DL_BUTTON_W),(int16_t)(content.bottom - content.top - DL_MARGIN - DL_BUTTON_H),
                                               (int16_t)(content.right - content.left - DL_MARGIN),(int16_t)(content.bottom - content.top - DL_MARGIN) };
            else _DLReadRect(it->addr + 2,&r);
            struct QDRect ring = { (int16_t)(content.left + r.left - 2),(int16_t)(content.top + r.top - 2),(int16_t)(content.left + r.right + 2),(int16_t)(content.top + r.bottom + 2) };
            QDSetClipRaw(&content);
            QDFrameRaw(&ring,(it->flags & DL_ITEM_DISABLED) ? CT_COL_DISABLED : CT_COL_FRAME);
        }
    }
    QDSetClipRaw(&save);
}

// ***************************************************************************************
//
//      API
//
// ***************************************************************************************

void DLReset(void) { for (int i = 0;i < DL_MAX_DIALOGS;i++) dialogs[i].window = 0; }

static struct Dialog *_DLAlloc(uint8_t *id) {
    for (int i = 0;i < DL_MAX_DIALOGS;i++) if (dialogs[i].window == 0) { *id = i + 1;return &dialogs[i]; }
    return NULL;
}

static void _DLFocusFirst(struct Dialog *d) {
    d->focus = 0;d->tracking = 0;
    for (int i = 0;i < d->count;i++) if (d->items[i].kind == CT_KIND_TEXT && !(d->items[i].flags & DL_ITEM_DISABLED)) { d->focus = i + 1;break; }
}

uint8_t DLNew(uint16_t descAddr,uint8_t *id,uint8_t *window) {
    if (descAddr > 0xFF00 - 12) return 1;
    struct Dialog *d = _DLAlloc(id);
    if (d == NULL) return 2;
    struct QDRect frame;_DLReadRect(descAddr,&frame);
    uint8_t flags = cpuMemory[descAddr + 8] & WM_FLAG_TITLE;
    uint16_t p = descAddr + 9;
    uint8_t titleLen = cpuMemory[p];
    if ((uint32_t)p + 1 + titleLen > 0xFF00 - 2) return 1;
    uint8_t win = 0;
    uint8_t e = WMNewWindowRaw(&frame,cpuMemory + p + 1,titleLen,flags,&win);
    if (e != 0) return e;
    p += 1 + titleLen;
    d->window = win;d->alert = false;d->count = 0;
    while (cpuMemory[p] != 0) {
        uint8_t kind = cpuMemory[p],iflags = cpuMemory[p + 1];
        if (kind > DL_KIND_STATIC || d->count >= DL_MAX_ITEMS || p > 0xFF00 - 13) { WMDisposeWindow(win);d->window = 0;return 1; }
        struct DialogItem *it = &d->items[d->count];
        it->kind = kind;it->flags = iflags;it->control = 0;it->addr = p;
        uint16_t max = cpuMemory[p + 10] | (cpuMemory[p + 11] << 8);
        uint16_t text = p + 12;
        if (kind != DL_KIND_STATIC) {
            struct QDRect r;_DLReadRect(p + 2,&r);
            e = CTNewRaw(kind,win,&r,cpuMemory + text,max,&it->control);
            if (e != 0) { WMDisposeWindow(win);d->window = 0;return e; }
            if (iflags & DL_ITEM_DISABLED) CTSetFlags(it->control,CT_FLAG_DISABLED);
        }
        d->count++;
        p = text + 1 + ((kind == CT_KIND_TEXT) ? max : cpuMemory[text]);
    }
    _DLFocusFirst(d);
    _DLDrawStatics(d);
    *window = win;
    return 0;
}

uint8_t DLDispose(uint8_t id) {
    struct Dialog *d = _DLGet(id);
    if (d == NULL) return 1;
    WMDisposeWindow(d->window);                                                 // Frees its controls too
    d->window = 0;
    return 0;
}

uint8_t DLDraw(uint8_t id) {
    struct Dialog *d = _DLGet(id);
    if (d == NULL) return 1;
    CTDrawAll(d->window);
    _DLDrawStatics(d);
    return 0;
}

uint8_t DLGetItem(uint8_t id,uint8_t item,uint8_t *control,uint8_t *kind) {
    struct Dialog *d = _DLGet(id);
    if (d == NULL || item < 1 || item > d->count) return 1;
    *control = d->items[item-1].control;*kind = d->items[item-1].kind;
    return 0;
}

static uint8_t _DLItemOfControl(const struct Dialog *d,uint8_t control) {
    if (control == 0) return 0;
    for (int i = 0;i < d->count;i++) if (d->items[i].control == control) return i + 1;
    return 0;
}

static uint8_t _DLItemWithFlag(const struct Dialog *d,uint8_t flag) {
    for (int i = 0;i < d->count;i++) if ((d->items[i].flags & flag) && !(d->items[i].flags & DL_ITEM_DISABLED)) return i + 1;
    return 0;
}

// Modal event handling. item = item that acted (0 none) ; consumed = 1 when the event belonged to the
// dialog (mouse and keys always do while it is up ; update/activate of other windows and timers do not).
uint8_t DLEvent(uint8_t id,uint16_t recAddr,uint8_t *item,uint8_t *consumed) {
    struct Dialog *d = _DLGet(id);
    *item = 0;*consumed = 0;
    if (d == NULL || recAddr > 0xFF00 - 8) return 1;
    const uint8_t *r = cpuMemory + recAddr;
    uint8_t what = r[0],message = r[1];
    int16_t x = (int16_t)(r[4] | (r[5] << 8)),y = (int16_t)(r[6] | (r[7] << 8));
    uint8_t acted = 0;
    switch (what) {
        case EVT_UPDATE:
            if (message != d->window) return 0;
            DLDraw(id);WMEndUpdate(d->window);
            *consumed = 1;
            break;
        case EVT_ACTIVATE:
            if (message == d->window) *consumed = 1;
            break;
        case EVT_MOUSEDOWN: {
            *consumed = 1;
            uint8_t control,part;
            CTFind(x,y,&control,&part);
            uint8_t it = _DLItemOfControl(d,control);
            if (it == 0) break;
            d->tracking = it;
            if (d->items[it-1].kind == CT_KIND_TEXT) d->focus = it;
            CTTrack(control,x,y,CT_PHASE_DOWN,&acted);
            if (acted) *item = it;
            break;
        }
        case EVT_MOUSEMOVE:
        case EVT_MOUSEUP:
            *consumed = 1;
            if (d->tracking != 0) {
                uint8_t it = d->tracking;
                CTTrack(d->items[it-1].control,x,y,what == EVT_MOUSEUP ? CT_PHASE_UP : CT_PHASE_MOVE,&acted);
                if (acted) *item = it;
                if (what == EVT_MOUSEUP) d->tracking = 0;
            }
            break;
        case EVT_KEYDOWN:
        case EVT_AUTOKEY: {
            *consumed = 1;
            uint8_t it = 0;
            if (message == 13) it = _DLItemWithFlag(d,DL_ITEM_DEFAULT);
            else if (message == 27) it = _DLItemWithFlag(d,DL_ITEM_CANCEL);
            else if (message == 9) {                                            // Tab : next enabled text field
                for (int n = 0;n < d->count;n++) {
                    int i = (d->focus + n) % d->count;                          // focus is 1-based : starts after it
                    if (d->items[i].kind == CT_KIND_TEXT && !(d->items[i].flags & DL_ITEM_DISABLED)) { d->focus = i + 1;break; }
                }
            } else if (d->focus != 0 && message != 0) {
                uint8_t changed = 0;
                CTKey(d->items[d->focus-1].control,message,&changed);
                if (changed) *item = d->focus;
            }
            if (it != 0) *item = it;
            break;
        }
        case EVT_KEYUP:
            *consumed = 1;
            break;
    }
    return 0;
}

// Alert : message (lines split on 13) and one or two buttons, centred on the screen. Item 1 = first
// button (OK / Yes, default, at the right), item 2 = second (Cancel / No, cancel), item 3 = the message.
uint8_t DLAlert(uint16_t msgAddr,uint8_t buttons,uint8_t *id,uint8_t *window) {
    if (msgAddr > 0xFF00 - 1 || buttons > DL_ALERT_YESNO) return 1;
    struct Dialog *d = _DLAlloc(id);
    if (d == NULL) return 2;
    const uint8_t *p = cpuMemory + msgAddr + 1;uint8_t left = cpuMemory[msgAddr];
    if ((uint32_t)msgAddr + 1 + left > 0xFF00) return 1;
    int lines = 0,width = 0;
    while (left > 0) {
        uint8_t n = 0;
        while (n < left && p[n] != 13) n++;
        if (n > width) width = n;
        lines++;
        if (n < left) n++;
        p += n;left -= n;
    }
    if (lines == 0) lines = 1;
    int nb = (buttons == DL_ALERT_OK) ? 1 : 2;
    int w = width * 6,minW = nb * DL_BUTTON_W + (nb - 1) * DL_MARGIN;
    if (w < minW) w = minW;
    w += 2 * DL_MARGIN;
    int h = lines * 8 + DL_BUTTON_H + 3 * DL_MARGIN;
    if (w + 2 > gMode.xGSize) w = gMode.xGSize - 2;
    if (h + 2 > gMode.yGSize) h = gMode.yGSize - 2;
    struct QDRect frame;
    frame.left = (gMode.xGSize - w - 2) / 2;frame.top = (gMode.yGSize - h - 2) / 2;
    frame.right = frame.left + w + 2;frame.bottom = frame.top + h + 2;
    uint8_t win = 0;
    uint8_t e = WMNewWindowRaw(&frame,alertOK,0,0,&win);
    if (e != 0) return e;
    d->window = win;d->alert = true;d->count = 0;
    const uint8_t *titles[2] = { buttons == DL_ALERT_YESNO ? alertYes : alertOK,buttons == DL_ALERT_YESNO ? alertNo : alertCancel };
    for (int i = 0;i < nb;i++) {
        struct DialogItem *it = &d->items[d->count];
        struct QDRect r = { (int16_t)(w - DL_MARGIN - DL_BUTTON_W - i * (DL_BUTTON_W + DL_MARGIN)),(int16_t)(h - DL_MARGIN - DL_BUTTON_H),0,0 };
        r.right = r.left + DL_BUTTON_W;r.bottom = r.top + DL_BUTTON_H;
        it->kind = CT_KIND_BUTTON;it->flags = (i == 0) ? DL_ITEM_DEFAULT : DL_ITEM_CANCEL;it->addr = 0;
        e = CTNewRaw(CT_KIND_BUTTON,win,&r,titles[i],0,&it->control);
        if (e != 0) { WMDisposeWindow(win);d->window = 0;return e; }
        d->count++;
    }
    struct DialogItem *msg = &d->items[d->count++];
    msg->kind = DL_KIND_STATIC;msg->flags = 0;msg->control = 0;msg->addr = msgAddr;
    _DLFocusFirst(d);
    _DLDrawStatics(d);
    *window = win;
    return 0;
}
