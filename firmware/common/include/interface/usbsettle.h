// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      usbsettle.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      22nd September 2026
//      Purpose :   USB enumeration barrier (T-32, ADR-0001) : every mount/umount callback
//                  timestamps an event ; phase P1 of the boot ends when the bus has been
//                  quiet for USB_QUIET ticks, with a floor and a ceiling, so the boot no
//                  longer depends on how many devices are plugged or in which order they
//                  enumerate. 100 Hz ticks (TMRRead).
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define USB_QUIET   30                                                          // 300 ms without an enumeration event, after the first one
#define USB_CEILING 400                                                         // At most 4 s (nothing plugged, or a device that never comes up)
#define USB_FLOOR   300                                                         // T-66 : at least 3 s, so the logos can be seen — enumeration
                                                                                // happens during this pause anyway, so it costs nothing but the wait

void USBNoteEvent(void);                                                        // Called by every mount/umount callback
void USBWaitSettled(void);
void USBReport(void);  															// T-66 : printed in P2, after the logo pause                                                      // P1 : returns when the bus is quiet (or at the ceiling)
bool USBIsSettled(void);                                                        // True once the barrier has been passed
uint16_t USBEventCount(void);                                                   // T-74 : enumeration events seen
