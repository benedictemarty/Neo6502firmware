// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      windows.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      17th September 2026
//      Purpose :   Toolbox group 34, Window Manager (ADR-01, F-43) : rectangular windows
//                  with a title bar, Z order, frames drawn by the firmware, content drawn
//                  by the 6502 on update events (group 33, type 9).
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define WM_MAX_WINDOWS   8
#define WM_TITLE_HEIGHT  10                                                     // Title bar (inside the frame)
#define WM_TITLE_MAX     31
#define WM_GROW_SIZE     8                                                      // Grow box, bottom right of the content

#define WM_FLAG_TITLE    0x01                                                   // Has a title bar (drag, close box)
#define WM_FLAG_CLOSE    0x02                                                   // Close box in the title bar
#define WM_FLAG_GROW     0x04                                                   // Grow box

#define WM_PART_NONE     0
#define WM_PART_CONTENT  1
#define WM_PART_DRAG     2
#define WM_PART_CLOSE    3
#define WM_PART_GROW     4

#define WM_COL_FRAME     15
#define WM_COL_TITLE_ON  15                                                     // Front window title bar
#define WM_COL_TITLE_OFF 8
#define WM_COL_TEXT      0
#define WM_COL_CONTENT   0

void WMReset(void);
uint8_t WMNewWindow(uint16_t rectAddr,uint16_t titleAddr,uint8_t flags,uint8_t *id);   // 34,1
uint8_t WMDisposeWindow(uint8_t id);                                            // 34,2
uint8_t WMShowWindow(uint8_t id,uint8_t visible);                               // 34,3
uint8_t WMSelectWindow(uint8_t id);                                             // 34,4
uint8_t WMMoveWindow(uint8_t id,int16_t x,int16_t y);                           // 34,5
uint8_t WMSizeWindow(uint8_t id,int16_t w,int16_t h);                           // 34,6
void WMFindWindow(int16_t x,int16_t y,uint8_t *id,uint8_t *part);               // 34,7
uint8_t WMGetContentRect(uint8_t id,uint16_t rectAddr);                         // 34,8
uint8_t WMSetTitle(uint8_t id,uint16_t titleAddr);                              // 34,9
uint8_t WMFrontWindow(void);                                                    // 34,10
uint8_t WMEndUpdate(uint8_t id);                                                // 34,11
uint8_t WMSetPort(uint8_t id);                                                  // 34,12
void WMInvalidate(const struct QDRect *r);                                      // Menus : windows under r redraw
