// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      clock.cpp (hardware)
//      Authors :   bmarty <bmarty@mailo.com>
//      Date :      17th September 2026
//      Purpose :   F-14 : program the RP2040 RTC from the firmware clock so that FatFs
//                  (SD card library, get_fattime() in its rtc.c) timestamps files. The USB
//                  storage FatFs is built with FF_FS_NORTC = 1 : no timestamps there.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "interface/clock.h"
#include "hardware/rtc.h"

void HWClockSet(const CLOCK_TIME *t) {
	static bool rtcStarted = false;
	if (!rtcStarted) { rtc_init();rtcStarted = true; }
	datetime_t dt;
	dt.year = t->year;dt.month = t->month;dt.day = t->day;
	dt.dotw = 0;  																// Not used by get_fattime ; 0 = Sunday placeholder.
	dt.hour = t->hour;dt.min = t->minute;dt.sec = t->second;
	rtc_set_datetime(&dt);
}
