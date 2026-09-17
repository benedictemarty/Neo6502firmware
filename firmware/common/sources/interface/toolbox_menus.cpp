// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      toolbox_menus.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      18th September 2026
//      Purpose :   Toolbox group 35, Menu Manager (see menus.h). No screen is saved under
//                  a pull-down : when it closes the area is erased and the windows it
//                  covered get an update event (group 34 model).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

struct Menu { bool used;uint16_t desc;int16_t x,width; };                       // Descriptor address, title position in the bar

static struct Menu menus[MN_MAX_MENUS];
static uint8_t openMenu = 0;                                                    // Pulled down menu (0 none)
static uint8_t openItem = 0;                                                    // Highlighted item (0 none)
static struct QDRect openRect;                                                  // Pull-down rectangle

// ***************************************************************************************
//
//      Descriptor access (6502 RAM, read in place)
//
// ***************************************************************************************

static const uint8_t *_MNTitle(const struct Menu *m,uint8_t *len) {
    *len = cpuMemory[m->desc];
    return cpuMemory + m->desc + 1;
}

// Address of the flags byte of item n (1..), NULL past the end. Item text follows.
static uint16_t _MNItem(const struct Menu *m,uint8_t n) {
    uint16_t a = m->desc + 1 + cpuMemory[m->desc];
    for (uint8_t i = 1;;i++) {
        if (a > 0xFF00 - 2 || cpuMemory[a] == MN_END || i > MN_MAX_ITEMS) return 0;
        if (i == n) return a;
        a += 2 + cpuMemory[a+1];
    }
}

static uint8_t _MNItemCount(const struct Menu *m) {
    uint8_t n = 0;
    while (_MNItem(m,n+1) != 0) n++;
    return n;
}

static int _MNWidest(const struct Menu *m) {
    int w = 0;
    for (uint8_t i = 1;;i++) {
        uint16_t a = _MNItem(m,i);
        if (a == 0) break;
        if (cpuMemory[a+1] > w) w = cpuMemory[a+1];
    }
    return w;
}

static void _MNLayout(void) {                                                   // Title positions in the bar
    int x = 4;
    for (int i = 0;i < MN_MAX_MENUS;i++) {
        if (!menus[i].used) continue;
        uint8_t len;_MNTitle(&menus[i],&len);
        menus[i].x = x;menus[i].width = len * 6;
        x += len * 6 + MN_TITLE_GAP;
    }
}

// ***************************************************************************************
//
//      Drawing
//
// ***************************************************************************************

static void _MNDrawTitle(const struct Menu *m,bool hilite) {
    struct QDRect r = { (int16_t)(m->x-4),0,(int16_t)(m->x+m->width+4),MN_BAR_HEIGHT };
    QDFillRaw(&r,hilite ? MN_COL_TEXT : MN_COL_BAR);
    uint8_t len;const uint8_t *t = _MNTitle(m,&len);
    QDTextRaw(t,len,m->x,2,hilite ? MN_COL_BAR : MN_COL_TEXT);
}

void MNDrawBar(void) {
    struct QDRect save;QDGetClipRaw(&save);
    struct QDRect bar = { 0,0,(int16_t)gMode.xGSize,MN_BAR_HEIGHT };
    QDSetClipRaw(&bar);
    QDFillRaw(&bar,MN_COL_BAR);
    for (int i = 0;i < MN_MAX_MENUS;i++) if (menus[i].used) _MNDrawTitle(&menus[i],openMenu == i+1);
    QDSetClipRaw(&save);
}

static void _MNDrawItem(const struct Menu *m,uint8_t n,bool hilite) {
    uint16_t a = _MNItem(m,n);
    if (a == 0) return;
    uint8_t flags = cpuMemory[a];
    struct QDRect r = { (int16_t)(openRect.left+1),(int16_t)(openRect.top+1+(n-1)*MN_ITEM_HEIGHT),(int16_t)(openRect.right-1),(int16_t)(openRect.top+1+n*MN_ITEM_HEIGHT) };
    bool active = !(flags & (MN_ITEM_DISABLED | MN_ITEM_SEP));
    QDFillRaw(&r,(hilite && active) ? MN_COL_TEXT : MN_COL_BAR);
    if (flags & MN_ITEM_SEP) {
        struct QDRect l = { r.left,(int16_t)(r.top+MN_ITEM_HEIGHT/2),r.right,(int16_t)(r.top+MN_ITEM_HEIGHT/2+1) };
        QDFillRaw(&l,MN_COL_DISABLED);
        return;
    }
    uint8_t colour = (flags & MN_ITEM_DISABLED) ? MN_COL_DISABLED : ((hilite && active) ? MN_COL_BAR : MN_COL_TEXT);
    if (flags & MN_ITEM_CHECKED) { static const uint8_t tick[] = { '*' };QDTextRaw(tick,1,r.left+2,r.top+1,colour); }
    QDTextRaw(cpuMemory + a + 2,cpuMemory[a+1],r.left+12,r.top+1,colour);
}

static void _MNOpen(uint8_t id) {
    struct Menu *m = &menus[id-1];
    openMenu = id;openItem = 0;
    int n = _MNItemCount(m);
    int w = _MNWidest(m) * 6 + 16;
    openRect.left = m->x - 4;openRect.top = MN_BAR_HEIGHT;
    openRect.right = openRect.left + w;openRect.bottom = openRect.top + n * MN_ITEM_HEIGHT + 2;
    if (openRect.right > gMode.xGSize) { openRect.right = gMode.xGSize;openRect.left = openRect.right - w; }
    struct QDRect save;QDGetClipRaw(&save);
    struct QDRect screen = { 0,0,(int16_t)gMode.xGSize,(int16_t)gMode.yGSize };
    QDSetClipRaw(&screen);
    _MNDrawTitle(m,true);
    QDFillRaw(&openRect,MN_COL_BAR);
    QDFrameRaw(&openRect,MN_COL_TEXT);
    for (int i = 1;i <= n;i++) _MNDrawItem(m,i,false);
    QDSetClipRaw(&save);
}

static void _MNClose(void) {
    if (openMenu == 0) return;
    struct QDRect save;QDGetClipRaw(&save);
    struct QDRect screen = { 0,0,(int16_t)gMode.xGSize,(int16_t)gMode.yGSize };
    QDSetClipRaw(&screen);
    QDFillRaw(&openRect,WM_COL_CONTENT);                                        // Desktop colour
    struct QDRect r = openRect;
    openMenu = 0;openItem = 0;
    MNDrawBar();
    QDSetClipRaw(&save);
    WMInvalidate(&r);                                                           // Windows underneath redraw (update events)
}

// ***************************************************************************************
//
//      API
//
// ***************************************************************************************

void MNReset(void) {
    for (int i = 0;i < MN_MAX_MENUS;i++) menus[i].used = false;
    openMenu = 0;openItem = 0;
}

uint8_t MNNewMenu(uint16_t descAddr,uint8_t *id) {
    if (descAddr > 0xFF00 - 4) return 1;
    int slot = -1;
    for (int i = 0;i < MN_MAX_MENUS;i++) if (!menus[i].used) { slot = i;break; }
    if (slot < 0) return 2;
    menus[slot].used = true;menus[slot].desc = descAddr;
    _MNLayout();
    *id = slot + 1;
    MNDrawBar();
    return 0;
}

uint8_t MNDisposeMenu(uint8_t id) {
    if (id < 1 || id > MN_MAX_MENUS || !menus[id-1].used) return 1;
    if (openMenu == id) _MNClose();
    menus[id-1].used = false;
    _MNLayout();
    MNDrawBar();
    return 0;
}

static uint8_t _MNTitleAt(int16_t x,int16_t y) {
    if (y < 0 || y >= MN_BAR_HEIGHT) return 0;
    for (int i = 0;i < MN_MAX_MENUS;i++) {
        if (menus[i].used && x >= menus[i].x - 4 && x < menus[i].x + menus[i].width + 4) return i + 1;
    }
    return 0;
}

uint8_t MNSelect(int16_t x,int16_t y) {
    uint8_t id = _MNTitleAt(x,y);
    if (openMenu != 0) _MNClose();
    if (id != 0) _MNOpen(id);
    return id;
}

void MNTrack(int16_t x,int16_t y) {
    if (openMenu == 0) return;
    uint8_t title = _MNTitleAt(x,y);
    if (title != 0 && title != openMenu) { _MNClose();_MNOpen(title);return; }
    uint8_t item = 0;
    if (x >= openRect.left && x < openRect.right && y >= openRect.top + 1 && y < openRect.bottom - 1) {
        item = (y - openRect.top - 1) / MN_ITEM_HEIGHT + 1;
        if (_MNItem(&menus[openMenu-1],item) == 0) item = 0;
    }
    if (item == openItem) return;
    struct QDRect save;QDGetClipRaw(&save);
    struct QDRect screen = { 0,0,(int16_t)gMode.xGSize,(int16_t)gMode.yGSize };
    QDSetClipRaw(&screen);
    if (openItem != 0) _MNDrawItem(&menus[openMenu-1],openItem,false);
    if (item != 0) _MNDrawItem(&menus[openMenu-1],item,true);
    QDSetClipRaw(&save);
    openItem = item;
}

void MNTrackEnd(uint8_t *menu,uint8_t *item) {
    *menu = 0;*item = 0;
    if (openMenu == 0) return;
    if (openItem != 0) {
        uint16_t a = _MNItem(&menus[openMenu-1],openItem);
        if (a != 0 && !(cpuMemory[a] & (MN_ITEM_DISABLED | MN_ITEM_SEP))) { *menu = openMenu;*item = openItem; }
    }
    _MNClose();
}

uint8_t MNSetItemFlags(uint8_t menu,uint8_t item,uint8_t flags) {
    if (menu < 1 || menu > MN_MAX_MENUS || !menus[menu-1].used) return 1;
    uint16_t a = _MNItem(&menus[menu-1],item);
    if (a == 0) return 1;
    cpuMemory[a] = (cpuMemory[a] & MN_ITEM_SEP) | (flags & ~MN_ITEM_SEP);
    if (openMenu == menu) { struct QDRect save;QDGetClipRaw(&save);struct QDRect screen = { 0,0,(int16_t)gMode.xGSize,(int16_t)gMode.yGSize };QDSetClipRaw(&screen);_MNDrawItem(&menus[menu-1],item,item == openItem);QDSetClipRaw(&save); }
    return 0;
}

uint8_t MNGetItemFlags(uint8_t menu,uint8_t item,uint8_t *flags) {
    if (menu < 1 || menu > MN_MAX_MENUS || !menus[menu-1].used) return 1;
    uint16_t a = _MNItem(&menus[menu-1],item);
    if (a == 0) return 1;
    *flags = cpuMemory[a];
    return 0;
}
