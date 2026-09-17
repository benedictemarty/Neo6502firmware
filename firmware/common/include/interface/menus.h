// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      menus.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      18th September 2026
//      Purpose :   Toolbox group 35, Menu Manager (ADR-01, F-44) : menu bar and pull-down
//                  menus. Menu descriptors stay in 6502 RAM and are read in place :
//                    title (length-prefixed), then items : flags byte + length-prefixed
//                    text, ended by a $FF flags byte. Flags : bit 0 disabled, bit 1
//                    checked, bit 7 separator (text ignored).
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define MN_MAX_MENUS     6
#define MN_MAX_ITEMS     16
#define MN_BAR_HEIGHT    12
#define MN_ITEM_HEIGHT   10
#define MN_TITLE_GAP     12                                                     // Pixels between titles
#define MN_COL_BAR       15
#define MN_COL_TEXT      0
#define MN_COL_DISABLED  8

#define MN_ITEM_DISABLED 0x01
#define MN_ITEM_CHECKED  0x02
#define MN_ITEM_SEP      0x80
#define MN_END           0xFF

void MNReset(void);
uint8_t MNNewMenu(uint16_t descAddr,uint8_t *id);                               // 35,1
void MNDrawBar(void);                                                           // 35,2
uint8_t MNSelect(int16_t x,int16_t y);                                          // 35,3 : menu opened (0 none)
void MNTrack(int16_t x,int16_t y);                                              // 35,4
void MNTrackEnd(uint8_t *menu,uint8_t *item);                                   // 35,5
uint8_t MNSetItemFlags(uint8_t menu,uint8_t item,uint8_t flags);                // 35,6
uint8_t MNDisposeMenu(uint8_t id);                                              // 35,7
uint8_t MNGetItemFlags(uint8_t menu,uint8_t item,uint8_t *flags);               // 35,8
