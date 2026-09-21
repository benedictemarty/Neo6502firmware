// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      timezone.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      21st September 2026
//      Purpose :   Time zones (T-26) : the clock keeps UTC ; 1,20 / 1,21 / FAT timestamps are
//                  local time of the zone selected by 1,24 (IANA style name, "Europe/Paris",
//                  "America/Montreal", or "UTC+2" / "UTC-3:30"), with the daylight saving rule
//                  of the zone (none, European Union, North America, Australia, New Zealand).
//                  The zone name is kept in the settings sector of the flash (settings.h).
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define TZ_RULE_NONE  0
#define TZ_RULE_EU    1                                                         // Last Sunday of March 01:00 UTC -> last Sunday of October 01:00 UTC
#define TZ_RULE_NA    2                                                         // 2nd Sunday of March 02:00 local -> 1st Sunday of November 02:00 local
#define TZ_RULE_AU    3                                                         // 1st Sunday of October 02:00 std -> 1st Sunday of April 03:00 dst (southern)
#define TZ_RULE_NZ    4                                                         // Last Sunday of September 02:00 std -> 1st Sunday of April 03:00 dst

#define TZ_NAME_MAX   31

uint8_t TZSet(const char *name);                                                // 0 ok, 1 unknown zone
const char *TZGetName(void);                                                    // "UTC" by default
int16_t TZOffsetAt(uint32_t utc,uint8_t *dst);                                  // Minutes to add to UTC at that instant (dst : 1 if daylight time)
uint32_t TZLocalToUTC(uint32_t local);                                          // Local wall clock seconds -> UTC seconds
void TZLoadFromStorage(void);                                                   // Settings sector, at start up
uint8_t TZSetAndSave(const char *name);                                         // 1,24 : 0 ok, 1 unknown zone, 2 flash write failed
