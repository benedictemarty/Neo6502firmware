// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      clock.h
//      Authors :   bmarty <bmarty@mailo.com>
//      Date :      17th September 2026
//      Purpose :   Date and time (F-14) : software clock on the 100 Hz timer, PCF8563 RTC
//                  on the UEXT I2C bus when present.
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once
#include <stdint.h>

#define CLK_SOURCE_UNSET    (0)  													// No time known.
#define CLK_SOURCE_SOFTWARE (1)  													// Set by 1,21, kept by the 100 Hz timer.
#define CLK_SOURCE_RTC      (2)  													// Read from the PCF8563.
#define CLK_SOURCE_MODEM    (3)  													// Set from the SNTP time of the USB modem (T-25).

typedef struct _clock_time {
	uint16_t year;  																// 1970-2099
	uint8_t  month,day,hour,minute,second;
	uint8_t  source;  																// CLK_SOURCE_*
} CLOCK_TIME;

void CLKInitialise(void);
void CLKGet(CLOCK_TIME *t);
uint8_t CLKSet(const CLOCK_TIME *t);  												// 1 if the fields are out of range.
void CLKGetParams(uint8_t *p);  													// P0-7 layout of 1,20
uint8_t CLKSetParams(const uint8_t *p);  											// P0-6 layout of 1,21
uint8_t CLKSyncFromModem(void);  													// 1,23 : 0 ok, 1 no modem, 2 no SNTP time / no answer (T-25)
uint8_t CLKParseModemTime(const char *line,CLOCK_TIME *t);  						// "+CIPSNTPTIME:Www Mmm dd hh:mm:ss yyyy" -> t (0 ok)
void HWClockSet(const CLOCK_TIME *t);  												// Implementation specific : RP2040 RTC (FAT timestamps), no-op in the emulator
