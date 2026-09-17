// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      events.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      17th September 2026
//      Purpose :   Toolbox group 33, Event Manager (ADR-01, F-42) : one queue for keyboard,
//                  mouse and timer events, polled by the 6502 (no blocking call : the hosts
//                  pump their input outside API calls).
//
//      EventRecord (8 bytes in 6502 RAM) :
//        0 what        0 null, 1 keyDown, 2 keyUp, 3 autoKey, 4 mouseDown, 5 mouseUp,
//                      6 mouseMove, 7 wheel, 8 timer
//        1 message     key : ASCII (0 if none) ; mouse down/up : buttons after the change ;
//                      wheel : delta (signed) ; timer : timer id
//        2 message2    key : key code ; mouse down/up : the button that changed, bit 7 = double click
//        3 modifiers   key : modifiers ; mouse : buttons held
//        4-5 x, 6-7 y  mouse position when the event was posted
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define EVT_NULL        0
#define EVT_KEYDOWN     1
#define EVT_KEYUP       2
#define EVT_AUTOKEY     3
#define EVT_MOUSEDOWN   4
#define EVT_MOUSEUP     5
#define EVT_MOUSEMOVE   6
#define EVT_WHEEL       7
#define EVT_TIMER       8
#define EVT_COUNT       9

#define EVT_QUEUE_SIZE  32
#define EVT_TIMERS      4
#define EVT_DOUBLE_TICKS 50                                                     // 500 ms
#define EVT_DOUBLE_DIST 4                                                       // pixels

struct EventRecord { uint8_t what,message,message2,modifiers;uint16_t x,y; };

void EVTReset(void);                                                            // DSP reset : disabled, queue empty
void EVTInit(uint16_t mask);                                                    // 33,1
uint8_t EVTGetNext(uint16_t recAddr,uint16_t mask);                             // 33,2 : 1 if an event was returned
uint8_t EVTAvailable(uint16_t mask);                                            // 33,3
void EVTFlush(uint16_t mask);                                                   // 33,4
uint8_t EVTSetTimer(uint8_t id,uint16_t period);                                // 33,5
void EVTStatus(uint8_t *queued,uint8_t *overflow);                              // 33,6
void EVTPostKey(uint8_t what,uint8_t ascii,uint8_t keyCode,uint8_t modifiers);  // from keyboard.cpp
void EVTPostMouseButtons(uint8_t oldButtons,uint8_t newButtons);                // from mouse.cpp
void EVTPostMouseMove(void);
void EVTPostWheel(int8_t delta);
