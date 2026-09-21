// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      clock.cpp
//      Authors :   bmarty <bmarty@mailo.com>
//      Date :      17th September 2026
//      Purpose :   Date and time (F-14). Two sources :
//                  - a PCF8563 real time clock at I2C address $51 on the UEXT bus, used when
//                    a probe at initialisation reads plausible BCD registers ;
//                  - otherwise a software clock : seconds since 1970 set by 1,21, advanced
//                    by the 100 Hz system timer (TMRRead). Unset until 1,21 is called.
//                  Setting the time writes the RTC too when it is present, and the RP2040
//                  RTC (HWClockSet) so FatFs timestamps files on the SD card (get_fattime).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "interface/clock.h"

#define PCF8563_ADDRESS   (0x51)
#define PCF8563_REG_TIME  (0x02)  													// seconds, minutes, hours, days, weekdays, months, years

static bool clkInitialised = false;
static bool rtcPresent = false;
static uint8_t swSource = CLK_SOURCE_UNSET;
static uint32_t swEpoch = 0;  														// Seconds since 1970 when the software clock was set.
static uint32_t swBaseTick = 0;  													// TMRRead() at that moment.
static uint32_t swLastTick = 0;  													// For the 32 bit wrap of the timer.
static uint32_t swWraps = 0;

// ***************************************************************************************
//
//		Civil date <-> seconds since 1970 (Howard Hinnant's algorithms, proleptic Gregorian).
//
// ***************************************************************************************

static uint32_t CLKDaysFromCivil(int y,int m,int d) {
	y -= m <= 2;
	int era = y / 400;
	int yoe = y - era * 400;
	int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
	int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return (uint32_t)(era * 146097 + doe - 719468);
}

static void CLKCivilFromDays(uint32_t z,int *y,int *m,int *d) {
	z += 719468;
	int era = z / 146097;
	int doe = z - era * 146097;
	int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	int yy = yoe + era * 400;
	int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	int mp = (5 * doy + 2) / 153;
	*d = doy - (153 * mp + 2) / 5 + 1;
	*m = mp + (mp < 10 ? 3 : -9);
	*y = yy + (*m <= 2);
}

static bool CLKValid(const CLOCK_TIME *t) {
	static const uint8_t mdays[] = { 31,29,31,30,31,30,31,31,30,31,30,31 };
	if (t->year < 1970 || t->year > 2099 || t->month < 1 || t->month > 12) return false;
	if (t->day < 1 || t->day > mdays[t->month-1]) return false;
	if (t->month == 2 && t->day == 29 && (t->year % 4) != 0) return false;  		// 1970-2099 : no century rule needed.
	return t->hour < 24 && t->minute < 60 && t->second < 60;
}

// ***************************************************************************************
//
//								  PCF8563 : BCD registers $02-$08
//
// ***************************************************************************************

static uint8_t CLKFromBCD(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
static uint8_t CLKToBCD(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

static bool CLKReadRTC(CLOCK_TIME *t) {
	uint8_t r[7];
	uint8_t reg = PCF8563_REG_TIME;
	if (IOI2CWriteBlock(PCF8563_ADDRESS,&reg,1) != 0) return false;
	if (IOI2CReadBlock(PCF8563_ADDRESS,r,7) != 0) return false;
	if (r[0] & 0x80) return false;  												// VL : voltage low, time not guaranteed.
	t->second = CLKFromBCD(r[0] & 0x7F);
	t->minute = CLKFromBCD(r[1] & 0x7F);
	t->hour = CLKFromBCD(r[2] & 0x3F);
	t->day = CLKFromBCD(r[3] & 0x3F);
	t->month = CLKFromBCD(r[5] & 0x1F);
	t->year = ((r[5] & 0x80) ? 1900 : 2000) + CLKFromBCD(r[6]);  					// $02 sec $03 min $04 hour $05 day $06 weekday $07 month (bit 7 = century) $08 year
	t->source = CLK_SOURCE_RTC;
	return CLKValid(t);
}

static bool CLKWriteRTC(const CLOCK_TIME *t) {
	uint8_t w[8];
	w[0] = PCF8563_REG_TIME;
	w[1] = CLKToBCD(t->second);
	w[2] = CLKToBCD(t->minute);
	w[3] = CLKToBCD(t->hour);
	w[4] = CLKToBCD(t->day);
	w[5] = (CLKDaysFromCivil(t->year,t->month,t->day) + 4) % 7;  					// 1970-01-01 was a Thursday (4).
	w[6] = CLKToBCD(t->month) | ((t->year < 2000) ? 0x80 : 0);
	w[7] = CLKToBCD(t->year % 100);
	return IOI2CWriteBlock(PCF8563_ADDRESS,w,8) == 0;
}

// ***************************************************************************************
//
//		Initialise : probe the RTC once. Called lazily by CLKGet/CLKSet (and at reset).
//
// ***************************************************************************************

void CLKInitialise(void) {
	CLOCK_TIME t;
	clkInitialised = true;
	swSource = CLK_SOURCE_UNSET;
	swWraps = 0;swLastTick = TMRRead();
	rtcPresent = CLKReadRTC(&t);
	if (rtcPresent) HWClockSet(&t);  											// RP2040 RTC follows the PCF8563 (FAT timestamps on SD).
}

static uint32_t CLKSeconds(void) {  												// Elapsed seconds since the timer started, 64 bit safe.
	uint32_t now = TMRRead();
	if (now < swLastTick) swWraps++;
	swLastTick = now;
	uint64_t ticks = ((uint64_t)swWraps << 32) | now;
	return (uint32_t)(ticks / 100);
}

static uint32_t modemNextTry = 0;  												// T-25 : next automatic attempt (100 Hz ticks)

static void CLKFromSeconds(uint32_t secs,CLOCK_TIME *t) {  						// UTC seconds -> local civil time (T-26)
	uint8_t dst;
	secs += (int32_t)TZOffsetAt(secs,&dst) * 60;
	int y,m,d;
	CLKCivilFromDays(secs / 86400,&y,&m,&d);
	t->year = y;t->month = m;t->day = d;
	t->hour = (secs / 3600) % 24;t->minute = (secs / 60) % 60;t->second = secs % 60;
}

static uint32_t CLKToSeconds(const CLOCK_TIME *t) {  								// Civil time -> seconds (no zone)
	return CLKDaysFromCivil(t->year,t->month,t->day) * 86400 + t->hour * 3600 + t->minute * 60 + t->second;
}

// Set the clock from UTC seconds (source given) : software clock, RTC (kept in UTC) and host.
static void CLKSetUTC(uint32_t utc,uint8_t source) {
	if (!clkInitialised) CLKInitialise();  											// Before any state is written (it resets it)
	swEpoch = utc;
	swBaseTick = CLKSeconds();
	swSource = source;
	CLOCK_TIME u;
	int y,m,d;
	CLKCivilFromDays(utc / 86400,&y,&m,&d);
	u.year = y;u.month = m;u.day = d;u.hour = (utc / 3600) % 24;u.minute = (utc / 60) % 60;u.second = utc % 60;u.source = source;
	if (rtcPresent) CLKWriteRTC(&u);
	HWClockSet(&u);
}

uint32_t CLKUTCNow(void) {  														// Current UTC seconds (T-26)
	if (!clkInitialised) CLKInitialise();
	CLOCK_TIME u;
	if (rtcPresent && CLKReadRTC(&u)) return CLKToSeconds(&u);
	uint32_t secs = CLKSeconds();
	return (swSource != CLK_SOURCE_UNSET) ? swEpoch + (secs - swBaseTick) : secs;
}

void CLKGet(CLOCK_TIME *t) {
	if (!clkInitialised) CLKInitialise();
	if (rtcPresent) {
		CLOCK_TIME u;
		if (CLKReadRTC(&u)) { CLKFromSeconds(CLKToSeconds(&u),t);t->source = CLK_SOURCE_RTC;return; }   // RTC in UTC, shown local
	}
	if (swSource == CLK_SOURCE_UNSET && HWCDCConnected(0) && (int32_t)(TMRRead() - modemNextTry) >= 0) {   // T-25 : unset and a
		modemNextTry = TMRRead() + 3000;  											// modem is there : ask it (30 s between tries)
		if (HWCDCReadAvailable(0) == 0) CLKSyncFromModem();  						// Only when the link is idle (a program may use it)
	}
	uint32_t secs = CLKSeconds();
	if (swSource != CLK_SOURCE_UNSET) secs = swEpoch + (secs - swBaseTick);  		// Unset : 1970-01-01 plus the uptime (UTC, no zone)
	if (swSource == CLK_SOURCE_UNSET) {
		int y,m,d;
		CLKCivilFromDays(secs / 86400,&y,&m,&d);
		t->year = y;t->month = m;t->day = d;t->hour = (secs / 3600) % 24;t->minute = (secs / 60) % 60;t->second = secs % 60;
	} else CLKFromSeconds(secs,t);
	t->source = swSource;
}

uint8_t CLKSet(const CLOCK_TIME *t) {  											// 1,21 : local time of the zone (T-26)
	if (!clkInitialised) CLKInitialise();
	if (!CLKValid(t)) return 1;
	CLKSetUTC(TZLocalToUTC(CLKToSeconds(t)),CLK_SOURCE_SOFTWARE);
	return 0;
}

// ***************************************************************************************
//
//		API helpers (1,20 / 1,21) working on the parameter block P0..P7 :
//		P0-1 year, P2 month, P3 day, P4 hour, P5 minute, P6 second, P7 source.
//
// ***************************************************************************************

void CLKGetParams(uint8_t *p) {
	CLOCK_TIME t;
	CLKGet(&t);
	p[0] = t.year & 0xFF;p[1] = t.year >> 8;
	p[2] = t.month;p[3] = t.day;p[4] = t.hour;p[5] = t.minute;p[6] = t.second;p[7] = t.source;
}

uint8_t CLKSetParams(const uint8_t *p) {
	CLOCK_TIME t;
	t.year = p[0] | (p[1] << 8);
	t.month = p[2];t.day = p[3];t.hour = p[4];t.minute = p[5];t.second = p[6];t.source = 0;
	return CLKSet(&t);
}

// ***************************************************************************************
//
//		T-25 : time from the USB modem (Pico W, Neo6502picowifi) : AT+CIPSNTPTIME? answers
//		"+CIPSNTPTIME:Tue Sep 15 12:00:00 2026" (local time : the modem adds its tz offset,
//		whole hours, AT+CIPSNTPCFG) then OK ; "Thu Jan 01 00:00:00 1970" while SNTP has not
//		synchronised. The host is served (KBDSync) while waiting, 1 s at most.
//
// ***************************************************************************************

uint8_t CLKParseModemTime(const char *line,CLOCK_TIME *t) {
	static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
	const char *p = strstr(line,"+CIPSNTPTIME:");
	if (p == NULL) return 2;
	p += 13;
	while (*p == ' ') p++;
	if (strlen(p) < 24) return 2;  													// "Www Mmm dd hh:mm:ss yyyy"
	const char *m = strstr(months,std::string(p + 4,3).c_str());
	if (m == NULL || (m - months) % 3 != 0) return 2;
	int month = (m - months) / 3 + 1;
	int day = atoi(p + 8),hour = atoi(p + 11),minute = atoi(p + 14),second = atoi(p + 17),year = atoi(p + 20);
	if (year < 1980) return 2;  														// 1970 : SNTP not synchronised yet
	t->year = year;t->month = month;t->day = day;t->hour = hour;t->minute = minute;t->second = second;t->source = CLK_SOURCE_MODEM;
	return 0;
}

uint8_t CLKSyncFromModem(void) {
	if (!HWCDCConnected(0)) return 1;
	uint8_t junk[64];
	while (HWCDCRead(0,junk,sizeof(junk)) > 0) ;  									// Drop pending input
	static const char cmd[] = "AT+CIPSNTPTIME?\r\n";
	if (HWCDCWrite(0,(const uint8_t *)cmd,sizeof(cmd) - 1) != sizeof(cmd) - 1) return 2;
	char line[96];int n = 0;uint8_t result = 2;CLOCK_TIME t;
	uint32_t timeOut = TMRRead() + 100;  											// 1 s
	while ((int32_t)(TMRRead() - timeOut) < 0) {
		uint8_t c;
		if (HWCDCRead(0,&c,1) == 0) { KBDSync();continue; }  							// Serve the USB host meanwhile
		if (c == '\n' || c == '\r') {
			line[n] = 0;
			if (n > 0 && strcmp(line,"OK") == 0) break;
			if (n > 0 && strcmp(line,"ERROR") == 0) break;
			if (n > 0 && CLKParseModemTime(line,&t) == 0) result = 0;
			n = 0;
		} else if (n < (int)sizeof(line) - 1) line[n++] = (char)c;
	}
	if (result != 0) return 2;
	if (!CLKValid(&t)) return 2;
	int modemTz = 0;                                                                // AT+CIPSNTPCFG? -> +CIPSNTPCFG:en,tz,"server"
	static const char cfg[] = "AT+CIPSNTPCFG?\r\n";                                 // (the modem adds tz hours : take it out, T-26)
	if (HWCDCWrite(0,(const uint8_t *)cfg,sizeof(cfg) - 1) == sizeof(cfg) - 1) {
		n = 0;timeOut = TMRRead() + 100;
		while ((int32_t)(TMRRead() - timeOut) < 0) {
			uint8_t c;
			if (HWCDCRead(0,&c,1) == 0) { KBDSync();continue; }
			if (c == '\n' || c == '\r') {
				line[n] = 0;
				if (n > 0 && (strcmp(line,"OK") == 0 || strcmp(line,"ERROR") == 0)) break;
				const char *p = strstr(line,"+CIPSNTPCFG:");
				if (p != NULL) { p = strchr(p,',');if (p != NULL) modemTz = atoi(p + 1); }
				n = 0;
			} else if (n < (int)sizeof(line) - 1) line[n++] = (char)c;
		}
	}
	CLKSetUTC(CLKToSeconds(&t) - (int32_t)modemTz * 3600,CLK_SOURCE_MODEM);
	return 0;
}
