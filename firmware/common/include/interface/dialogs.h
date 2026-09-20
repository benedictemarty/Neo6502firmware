// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      dialogs.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      18th September 2026
//      Purpose :   Toolbox group 37, Dialog Manager (ADR-01, F-45) : modal dialogs and
//                  alerts built from a descriptor in 6502 RAM, on top of the Window (34)
//                  and Control (36) managers. No blocking call : the program feeds the
//                  events it gets from group 33 to Dialog Event and gets the item hit.
//
//      Descriptor (read in place, must stay intact while the dialog is up) :
//        Rect (8)        screen rectangle of the window
//        flags (1)       bit 0 title bar
//        title           length-prefixed
//        items           kind (1) : 0 end, 1 button, 2 check box, 3 radio, 4 scroll bar,
//                                   5 text field, 6 static text
//                        flags (1) : bit 0 disabled, bit 1 default (Return), bit 2 cancel (Escape)
//                        Rect (8)  : relative to the content of the window
//                        max (2)   : scroll bar maximum / text field maximum length
//                        text      : length-prefixed title / text / buffer (a text field
//                                    reserves 1 + max bytes ; static text : lines split on 13)
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define DL_MAX_DIALOGS   2                                                      // An alert over a dialog
#define DL_MAX_ITEMS     12

#define DL_KIND_STATIC   6                                                      // 1..5 = CT_KIND_*
#define DL_ITEM_DISABLED 0x01
#define DL_ITEM_DEFAULT  0x02
#define DL_ITEM_CANCEL   0x04

#define DL_ALERT_OK      0
#define DL_ALERT_OKCANCEL 1
#define DL_ALERT_YESNO   2

#define DL_BUTTON_W      48
#define DL_BUTTON_H      16
#define DL_MARGIN        8

void DLReset(void);
uint8_t DLNew(uint16_t descAddr,uint8_t *id,uint8_t *window);                  // 37,1
uint8_t DLDispose(uint8_t id);                                                  // 37,2
uint8_t DLEvent(uint8_t id,uint16_t recAddr,uint8_t *item,uint8_t *consumed);  // 37,3
uint8_t DLGetItem(uint8_t id,uint8_t item,uint8_t *control,uint8_t *kind);      // 37,4
uint8_t DLDraw(uint8_t id);                                                     // 37,5
uint8_t DLAlert(uint16_t msgAddr,uint8_t buttons,uint8_t *id,uint8_t *window);  // 37,6
