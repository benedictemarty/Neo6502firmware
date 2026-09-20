// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      clock.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      20th September 2026
//      Purpose :   Board side of the date and time (T-18, F-14 of the fork) : FatFs timestamps.
//                  Trinity builds its own FatFs (firmware/lib/fatfs, FF_FS_NORTC = 0) : get_fattime()
//                  is computed from the firmware clock (PCF8563 or software clock, clock.cpp of
//                  common), so the files NeoDOS or NeoBASIC write on the USB key are dated. No use
//                  of the RP2040 RTC (HWClockSet is a no-op).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "interface/clock.h"
#include "ff.h"

void HWClockSet(const CLOCK_TIME *t) { (void)t; }

extern "C" DWORD get_fattime(void) {
	CLOCK_TIME t;
	CLKGet(&t);
	if (t.year < 1980) return ((DWORD)(1980 - 1980) << 25) | (1 << 21) | (1 << 16);	// FAT epoch : 1980-01-01 00:00:00
	return ((DWORD)(t.year - 1980) << 25) | ((DWORD)t.month << 21) | ((DWORD)t.day << 16)
		 | ((DWORD)t.hour << 11) | ((DWORD)t.minute << 5) | ((DWORD)t.second >> 1);
}
