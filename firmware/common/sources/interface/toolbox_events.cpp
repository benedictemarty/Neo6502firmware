// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      toolbox_events.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      17th September 2026
//      Purpose :   Toolbox group 33, Event Manager (see events.h)
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

static struct EventRecord queue[EVT_QUEUE_SIZE];
static uint8_t qHead = 0,qTail = 0;                                             // Circular queue
static bool enabled = false;
static uint16_t enabledMask = 0;                                                // bit (what-1) : event type posted
static uint8_t overflowed = 0;
static uint16_t timerPeriod[EVT_TIMERS];
static uint32_t timerNext[EVT_TIMERS];
static uint32_t lastDownTime[8];                                                // Double click detection per button
static uint16_t lastDownX[8],lastDownY[8];

static inline bool _EVTWanted(uint8_t what,uint16_t mask) { return (mask & (1 << (what-1))) != 0; }

static void _EVTPost(uint8_t what,uint8_t message,uint8_t message2,uint8_t modifiers) {
    if (!enabled || !_EVTWanted(what,enabledMask)) return;
    uint8_t next = (qTail + 1) % EVT_QUEUE_SIZE;
    if (next == qHead) { overflowed = 1;return; }                               // Full : drop, remember
    uint16_t x,y;uint8_t b,w;
    MSEGetState(&x,&y,&b,&w);
    struct EventRecord *e = &queue[qTail];
    e->what = what;e->message = message;e->message2 = message2;e->modifiers = modifiers;e->x = x;e->y = y;
    qTail = next;
}

// ***************************************************************************************
//
//      Reset / init
//
// ***************************************************************************************

void EVTReset(void) {
    enabled = false;enabledMask = 0;qHead = qTail = 0;overflowed = 0;
    for (int i = 0;i < EVT_TIMERS;i++) timerPeriod[i] = 0;
    for (int i = 0;i < 8;i++) lastDownTime[i] = 0;
}

void EVTInit(uint16_t mask) {
    EVTReset();
    enabled = true;
    enabledMask = (mask == 0) ? 0xFFFF : mask;
}

// ***************************************************************************************
//
//      Timers : due events are generated when the 6502 polls (at most one per timer)
//
// ***************************************************************************************

static void _EVTRunTimers(void) {
    uint32_t now = TMRRead();
    for (int i = 0;i < EVT_TIMERS;i++) {
        if (timerPeriod[i] != 0 && (int32_t)(now - timerNext[i]) >= 0) {
            timerNext[i] = now + timerPeriod[i];
            _EVTPost(EVT_TIMER,i,0,0);
        }
    }
}

uint8_t EVTSetTimer(uint8_t id,uint16_t period) {
    if (id >= EVT_TIMERS) return 1;
    timerPeriod[id] = period;
    timerNext[id] = TMRRead() + period;
    return 0;
}

// ***************************************************************************************
//
//      Polling
//
// ***************************************************************************************

static void _EVTWrite(uint16_t addr,const struct EventRecord *e) {
    uint8_t *p = cpuMemory + addr;
    p[0] = e->what;p[1] = e->message;p[2] = e->message2;p[3] = e->modifiers;
    p[4] = e->x & 0xFF;p[5] = e->x >> 8;p[6] = e->y & 0xFF;p[7] = e->y >> 8;
}

uint8_t EVTGetNext(uint16_t recAddr,uint16_t mask) {
    if (recAddr > 0xFF00 - 8) return 0xFF;                                      // Bad address (caller flags the error)
    _EVTRunTimers();
    if (mask == 0) mask = 0xFFFF;
    for (uint8_t i = qHead;i != qTail;i = (i + 1) % EVT_QUEUE_SIZE) {           // First wanted event, keep the others
        if (_EVTWanted(queue[i].what,mask)) {
            struct EventRecord e = queue[i];
            for (uint8_t j = i;j != qHead;j = (j + EVT_QUEUE_SIZE - 1) % EVT_QUEUE_SIZE)  // Close the gap
                queue[j] = queue[(j + EVT_QUEUE_SIZE - 1) % EVT_QUEUE_SIZE];
            qHead = (qHead + 1) % EVT_QUEUE_SIZE;
            _EVTWrite(recAddr,&e);
            return 1;
        }
    }
    struct EventRecord null = { EVT_NULL,0,0,0,0,0 };
    MSEGetState(&null.x,&null.y,&null.modifiers,&null.message2);null.message2 = 0;
    _EVTWrite(recAddr,&null);
    return 0;
}

uint8_t EVTAvailable(uint16_t mask) {
    _EVTRunTimers();
    if (mask == 0) mask = 0xFFFF;
    uint8_t n = 0;
    for (uint8_t i = qHead;i != qTail;i = (i + 1) % EVT_QUEUE_SIZE) if (_EVTWanted(queue[i].what,mask)) n++;
    return n;
}

void EVTFlush(uint16_t mask) {
    if (mask == 0) { qHead = qTail = 0;overflowed = 0;return; }
    uint8_t out = qHead;
    for (uint8_t i = qHead;i != qTail;i = (i + 1) % EVT_QUEUE_SIZE) {
        if (!_EVTWanted(queue[i].what,mask)) { queue[out] = queue[i];out = (out + 1) % EVT_QUEUE_SIZE; }
    }
    qTail = out;
}

void EVTStatus(uint8_t *queued,uint8_t *overflow) {
    *queued = (qTail + EVT_QUEUE_SIZE - qHead) % EVT_QUEUE_SIZE;
    *overflow = overflowed;overflowed = 0;
}

// ***************************************************************************************
//
//      Sources
//
// ***************************************************************************************

void EVTPostKey(uint8_t what,uint8_t ascii,uint8_t keyCode,uint8_t modifiers) {
    _EVTPost(what,ascii,keyCode,modifiers);
}

void EVTPostMouseButtons(uint8_t oldButtons,uint8_t newButtons) {
    uint8_t changed = oldButtons ^ newButtons;
    for (int b = 0;b < 8;b++) {
        if (!(changed & (1 << b))) continue;
        if (newButtons & (1 << b)) {                                            // Down : double click ?
            uint16_t x,y;uint8_t bs,w;
            MSEGetState(&x,&y,&bs,&w);
            uint32_t now = TMRRead();
            uint8_t which = 1 << b;
            if (lastDownTime[b] != 0 && now - lastDownTime[b] <= EVT_DOUBLE_TICKS &&
                abs((int)x - (int)lastDownX[b]) <= EVT_DOUBLE_DIST && abs((int)y - (int)lastDownY[b]) <= EVT_DOUBLE_DIST) {
                which |= 0x80;lastDownTime[b] = 0;
            } else {
                lastDownTime[b] = now ? now : 1;lastDownX[b] = x;lastDownY[b] = y;
            }
            _EVTPost(EVT_MOUSEDOWN,newButtons,which,newButtons);
        } else {
            _EVTPost(EVT_MOUSEUP,newButtons,1 << b,newButtons);
        }
    }
}

void EVTPostMouseMove(void) {
    if (!enabled) return;
    if (qTail != qHead) {                                                       // Coalesce with a trailing move
        uint8_t last = (qTail + EVT_QUEUE_SIZE - 1) % EVT_QUEUE_SIZE;
        if (queue[last].what == EVT_MOUSEMOVE) {
            uint8_t b,w;MSEGetState(&queue[last].x,&queue[last].y,&b,&w);queue[last].modifiers = b;
            return;
        }
    }
    uint16_t x,y;uint8_t b,w;
    MSEGetState(&x,&y,&b,&w);
    _EVTPost(EVT_MOUSEMOVE,0,0,b);
}

void EVTPostWheel(int8_t delta) {
    uint16_t x,y;uint8_t b,w;
    MSEGetState(&x,&y,&b,&w);
    _EVTPost(EVT_WHEEL,(uint8_t)delta,0,b);
}

void EVTPostWindow(uint8_t what,uint8_t window,uint8_t message2) {
    _EVTPost(what,window,message2,0);
}
