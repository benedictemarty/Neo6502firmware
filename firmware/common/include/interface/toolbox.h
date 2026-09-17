// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      toolbox.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      17th September 2026
//      Purpose :   Toolbox (ADR-01) : groups 32.. ; group 32 = QuickDraw.
//
//      Conventions (ADR-01) : structures live in 6502 RAM and are passed by 16 bit
//      address ; Rect = left, top, right, bottom as int16 (right/bottom exclusive) ;
//      Point = x, y as int16 ; results in the parameters ; errors in $FF02 (QD_ERR_*).
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define QD_ERR_OK        0
#define QD_ERR_PARAM     1                                                      // Bad address or degenerate rectangle
#define QD_ERR_NOGFX     2                                                      // No graphics screen

struct QDRect { int16_t left,top,right,bottom; };

void QDInitGraf(void);                                                          // 32,1
uint8_t QDSetClip(uint16_t rectAddr);                                           // 32,2
uint8_t QDGetClip(uint16_t rectAddr);                                           // 32,3
void QDSetPenColour(uint8_t colour);                                            // 32,4
void QDMoveTo(int16_t x,int16_t y);                                             // 32,5
void QDGetPen(int16_t *x,int16_t *y,uint8_t *colour);                           // 32,6
uint8_t QDFrameRect(uint16_t rectAddr);                                         // 32,7
uint8_t QDPaintRect(uint16_t rectAddr);                                         // 32,8
uint8_t QDEraseRect(uint16_t rectAddr,uint8_t colour);                          // 32,9
uint8_t QDInvertRect(uint16_t rectAddr);                                        // 32,10
uint8_t QDLineTo(int16_t x,int16_t y);                                          // 32,11
uint8_t QDSetPattern(uint16_t patAddr);                                         // 32,12
uint8_t QDFillRect(uint16_t rectAddr,uint8_t backColour);                       // 32,13
uint8_t QDCopyBits(uint8_t action,uint16_t areaAddr,int16_t x,int16_t y);       // 32,14
