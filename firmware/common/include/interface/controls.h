// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      controls.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      18th September 2026
//      Purpose :   Toolbox group 36, Control Manager (ADR-01, F-44) : buttons, check
//                  boxes, radio buttons, scroll bars and text fields attached to a window
//                  (rectangles relative to its content), drawn on request, tracked by
//                  phases from mouse events. Titles / text buffers stay in 6502 RAM.
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define CT_MAX_CONTROLS  24

#define CT_KIND_BUTTON   1
#define CT_KIND_CHECK    2
#define CT_KIND_RADIO    3
#define CT_KIND_SCROLL   4                                                      // Vertical if higher than wide
#define CT_KIND_TEXT     5

#define CT_FLAG_DISABLED 0x01
#define CT_FLAG_HILITE   0x80                                                   // Internal : button pressed

#define CT_PART_NONE     0
#define CT_PART_BODY     1
#define CT_PART_UP       2                                                      // Scroll bar : first arrow
#define CT_PART_DOWN     3                                                      // Scroll bar : second arrow
#define CT_PART_PAGEUP   4
#define CT_PART_PAGEDOWN 5
#define CT_PART_THUMB    6

#define CT_PHASE_DOWN    0
#define CT_PHASE_MOVE    1
#define CT_PHASE_UP      2

#define CT_COL_FRAME     15
#define CT_COL_TEXT      15
#define CT_COL_BACK      0
#define CT_COL_DISABLED  8
#define CT_ARROW         8                                                      // Scroll bar arrow length

void CTReset(void);
uint8_t CTNew(uint8_t kind,uint8_t window,uint16_t rectAddr,uint16_t textAddr,uint16_t max,uint8_t *id);  // 36,1
uint8_t CTDispose(uint8_t id);                                                  // 36,2
uint8_t CTDrawAll(uint8_t window);                                              // 36,3
uint8_t CTDraw(uint8_t id);                                                     // 36,4
uint8_t CTSetValue(uint8_t id,int16_t value);                                   // 36,5
uint8_t CTGetValue(uint8_t id,int16_t *value);                                  // 36,6
uint8_t CTSetFlags(uint8_t id,uint8_t flags);                                   // 36,7
void CTFind(int16_t x,int16_t y,uint8_t *id,uint8_t *part);                     // 36,8
uint8_t CTTrack(uint8_t id,int16_t x,int16_t y,uint8_t phase,uint8_t *acted);   // 36,9
uint8_t CTKey(uint8_t id,uint8_t key,uint8_t *changed);                         // 36,10
void CTWindowDisposed(uint8_t window);                                          // from the Window Manager
