// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      resources.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      18th September 2026
//      Purpose :   Toolbox group 38, Resource Manager (ADR-01, F-46) : named resources
//                  (fonts, texts, icons, menu or dialog descriptors...) kept on storage in
//                  one NR1 file (docs-bmarty/tools/mkres.py), read on demand into any page
//                  the blitter addresses (like 3,27). Nothing is cached in the RP2040 but
//                  the channel and the count (R22 : no SRAM to spare).
//
//      NR1 file : 'N','R',1,count,0,0,0,0 then count entries of 16 bytes : type[4], id u16,
//      offset u32, size u32, 2 reserved ; then the data.
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define RS_ERR_OK        0
#define RS_ERR_PARAM     1                                                      // Bad parameter / not a resource file / no file open
#define RS_ERR_IO        2                                                      // Storage error (file missing, read failed)
#define RS_ERR_NOTFOUND  3                                                      // Index out of range / type-id not found

struct RSEntry { uint8_t type[4];uint16_t id;uint32_t offset,size; };

void RSReset(void);
uint8_t RSOpen(uint8_t channel,uint16_t nameAddr);                              // 38,1
uint8_t RSClose(void);                                                          // 38,2
uint8_t RSCount(uint8_t *count);                                                // 38,3
uint8_t RSFind(const uint8_t *type,uint16_t id,uint8_t *index);                 // 38,4
uint8_t RSInfo(uint8_t index,struct RSEntry *entry);                            // 38,5 / 38,6
uint8_t RSLoad(uint8_t index,uint8_t page,uint16_t address,uint16_t *size);     // 38,7
uint8_t RSUseFont(uint8_t index,uint8_t page,uint16_t address,uint16_t *size);  // 38,8
