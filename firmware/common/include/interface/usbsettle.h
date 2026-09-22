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

#define USB_QUIET   30                                                          // 300 ms without an enumeration event
#define USB_FLOOR   50                                                          // At least 500 ms of discovery
#define USB_CEILING 500                                                         // At most 5 s

void USBNoteEvent(void);                                                        // Called by every mount/umount callback
void USBWaitSettled(void);                                                      // P1 : returns when the bus is quiet (or at the ceiling)
bool USBIsSettled(void);                                                        // True once the barrier has been passed
