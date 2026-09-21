// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      settings.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      21st September 2026
//      Purpose :   Persistent settings of the board (T-26) : one 4k flash sector just below
//                  the memory banks (board) or storage/settings.flash (emulators). Record :
//                  magic "NST1", then fields ; the time zone name first (1,24). Written only
//                  when a setting changes (flash wear).
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define SETTINGS_SIZE       256
#define SETTINGS_MAGIC      "NST1"
#define SETTINGS_ZONE       8                                                   // char[32] : time zone name (T-26)
#define SETTINGS_ZONE_LEN   32

bool SETLoad(void);                                                             // Fills the record from flash ; false if none/invalid
const uint8_t *SETRecord(void);
uint8_t SETWrite(void);                                                         // Programs the record ; 0 if ok
uint8_t SETSetString(uint16_t offset,uint16_t maxLen,const char *value);        // Change a field and write ; 0 if ok
const char *SETGetString(uint16_t offset);

const uint8_t *HWSettingsStorage(void);                                         // Host : SETTINGS_SIZE bytes in place
uint8_t HWSettingsWrite(const uint8_t *data);                                   // Host : program SETTINGS_SIZE bytes
