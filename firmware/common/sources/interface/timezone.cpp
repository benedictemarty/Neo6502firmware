// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      timezone.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      21st September 2026
//      Purpose :   Time zones (T-26, see timezone.h). Zone table : name, standard offset in
//                  minutes, daylight saving rule. Not the IANA database : the usual zones,
//                  rules as of 2026, no history.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "interface/timezone.h"

struct Zone { const char *name;int16_t offset;uint8_t rule; };

static const struct Zone zones[] = {
	{ "UTC",0,TZ_RULE_NONE },{ "Etc/UTC",0,TZ_RULE_NONE },{ "GMT",0,TZ_RULE_NONE },
	// Europe
	{ "Europe/London",0,TZ_RULE_EU },{ "Europe/Dublin",0,TZ_RULE_EU },{ "Europe/Lisbon",0,TZ_RULE_EU },
	{ "Atlantic/Canary",0,TZ_RULE_EU },{ "Atlantic/Reykjavik",0,TZ_RULE_NONE },{ "Atlantic/Azores",-60,TZ_RULE_EU },
	{ "Europe/Paris",60,TZ_RULE_EU },{ "Europe/Brussels",60,TZ_RULE_EU },{ "Europe/Amsterdam",60,TZ_RULE_EU },
	{ "Europe/Luxembourg",60,TZ_RULE_EU },{ "Europe/Madrid",60,TZ_RULE_EU },{ "Europe/Berlin",60,TZ_RULE_EU },
	{ "Europe/Rome",60,TZ_RULE_EU },{ "Europe/Zurich",60,TZ_RULE_EU },{ "Europe/Vienna",60,TZ_RULE_EU },
	{ "Europe/Prague",60,TZ_RULE_EU },{ "Europe/Warsaw",60,TZ_RULE_EU },{ "Europe/Budapest",60,TZ_RULE_EU },
	{ "Europe/Belgrade",60,TZ_RULE_EU },{ "Europe/Zagreb",60,TZ_RULE_EU },{ "Europe/Ljubljana",60,TZ_RULE_EU },
	{ "Europe/Sarajevo",60,TZ_RULE_EU },{ "Europe/Skopje",60,TZ_RULE_EU },{ "Europe/Bratislava",60,TZ_RULE_EU },
	{ "Europe/Stockholm",60,TZ_RULE_EU },{ "Europe/Oslo",60,TZ_RULE_EU },{ "Europe/Copenhagen",60,TZ_RULE_EU },
	{ "Europe/Monaco",60,TZ_RULE_EU },{ "Europe/Andorra",60,TZ_RULE_EU },{ "Europe/Malta",60,TZ_RULE_EU },
	{ "Europe/Tirane",60,TZ_RULE_EU },{ "Europe/Podgorica",60,TZ_RULE_EU },
	{ "Europe/Helsinki",120,TZ_RULE_EU },{ "Europe/Athens",120,TZ_RULE_EU },{ "Europe/Bucharest",120,TZ_RULE_EU },
	{ "Europe/Sofia",120,TZ_RULE_EU },{ "Europe/Kiev",120,TZ_RULE_EU },{ "Europe/Kyiv",120,TZ_RULE_EU },
	{ "Europe/Riga",120,TZ_RULE_EU },{ "Europe/Tallinn",120,TZ_RULE_EU },{ "Europe/Vilnius",120,TZ_RULE_EU },
	{ "Europe/Chisinau",120,TZ_RULE_EU },{ "Europe/Nicosia",120,TZ_RULE_EU },
	{ "Europe/Istanbul",180,TZ_RULE_NONE },{ "Europe/Moscow",180,TZ_RULE_NONE },{ "Europe/Minsk",180,TZ_RULE_NONE },
	// Africa
	{ "Africa/Casablanca",60,TZ_RULE_NONE },{ "Africa/Algiers",60,TZ_RULE_NONE },{ "Africa/Tunis",60,TZ_RULE_NONE },
	{ "Africa/Lagos",60,TZ_RULE_NONE },{ "Africa/Kinshasa",60,TZ_RULE_NONE },{ "Africa/Cairo",120,TZ_RULE_NONE },
	{ "Africa/Johannesburg",120,TZ_RULE_NONE },{ "Africa/Nairobi",180,TZ_RULE_NONE },{ "Africa/Dakar",0,TZ_RULE_NONE },
	{ "Africa/Abidjan",0,TZ_RULE_NONE },{ "Indian/Reunion",240,TZ_RULE_NONE },{ "Indian/Mauritius",240,TZ_RULE_NONE },
	// America
	{ "America/St_Johns",-210,TZ_RULE_NA },{ "America/Halifax",-240,TZ_RULE_NA },{ "America/Moncton",-240,TZ_RULE_NA },
	{ "America/Montreal",-300,TZ_RULE_NA },{ "America/Toronto",-300,TZ_RULE_NA },{ "America/New_York",-300,TZ_RULE_NA },
	{ "America/Detroit",-300,TZ_RULE_NA },{ "America/Chicago",-360,TZ_RULE_NA },{ "America/Winnipeg",-360,TZ_RULE_NA },
	{ "America/Regina",-360,TZ_RULE_NONE },{ "America/Denver",-420,TZ_RULE_NA },{ "America/Edmonton",-420,TZ_RULE_NA },
	{ "America/Phoenix",-420,TZ_RULE_NONE },{ "America/Los_Angeles",-480,TZ_RULE_NA },{ "America/Vancouver",-480,TZ_RULE_NA },
	{ "America/Anchorage",-540,TZ_RULE_NA },{ "America/Whitehorse",-420,TZ_RULE_NONE },{ "America/Yellowknife",-420,TZ_RULE_NA },
	{ "America/Iqaluit",-300,TZ_RULE_NA },{ "America/Mexico_City",-360,TZ_RULE_NONE },{ "America/Havana",-300,TZ_RULE_NONE },
	{ "America/Bogota",-300,TZ_RULE_NONE },{ "America/Lima",-300,TZ_RULE_NONE },{ "America/Caracas",-240,TZ_RULE_NONE },
	{ "America/Santiago",-240,TZ_RULE_NONE },{ "America/Sao_Paulo",-180,TZ_RULE_NONE },{ "America/Buenos_Aires",-180,TZ_RULE_NONE },
	{ "America/Argentina/Buenos_Aires",-180,TZ_RULE_NONE },{ "America/Montevideo",-180,TZ_RULE_NONE },
	{ "America/Martinique",-240,TZ_RULE_NONE },{ "America/Guadeloupe",-240,TZ_RULE_NONE },{ "America/Cayenne",-180,TZ_RULE_NONE },
	{ "America/Miquelon",-180,TZ_RULE_NA },{ "Pacific/Honolulu",-600,TZ_RULE_NONE },
	// Asia / Pacific
	{ "Asia/Dubai",240,TZ_RULE_NONE },{ "Asia/Tehran",210,TZ_RULE_NONE },{ "Asia/Karachi",300,TZ_RULE_NONE },
	{ "Asia/Kolkata",330,TZ_RULE_NONE },{ "Asia/Calcutta",330,TZ_RULE_NONE },{ "Asia/Kathmandu",345,TZ_RULE_NONE },
	{ "Asia/Dhaka",360,TZ_RULE_NONE },{ "Asia/Bangkok",420,TZ_RULE_NONE },{ "Asia/Jakarta",420,TZ_RULE_NONE },
	{ "Asia/Ho_Chi_Minh",420,TZ_RULE_NONE },{ "Asia/Shanghai",480,TZ_RULE_NONE },{ "Asia/Hong_Kong",480,TZ_RULE_NONE },
	{ "Asia/Singapore",480,TZ_RULE_NONE },{ "Asia/Manila",480,TZ_RULE_NONE },{ "Asia/Taipei",480,TZ_RULE_NONE },
	{ "Asia/Tokyo",540,TZ_RULE_NONE },{ "Asia/Seoul",540,TZ_RULE_NONE },{ "Asia/Jerusalem",120,TZ_RULE_NONE },
	{ "Asia/Riyadh",180,TZ_RULE_NONE },{ "Asia/Baghdad",180,TZ_RULE_NONE },{ "Asia/Tashkent",300,TZ_RULE_NONE },
	{ "Australia/Perth",480,TZ_RULE_NONE },{ "Australia/Darwin",570,TZ_RULE_NONE },{ "Australia/Adelaide",570,TZ_RULE_AU },
	{ "Australia/Brisbane",600,TZ_RULE_NONE },{ "Australia/Sydney",600,TZ_RULE_AU },{ "Australia/Melbourne",600,TZ_RULE_AU },
	{ "Australia/Hobart",600,TZ_RULE_AU },{ "Pacific/Auckland",720,TZ_RULE_NZ },{ "Pacific/Noumea",660,TZ_RULE_NONE },
	{ "Pacific/Tahiti",-600,TZ_RULE_NONE },{ "Pacific/Guam",600,TZ_RULE_NONE },
	{ NULL,0,0 }
};

static int16_t zoneOffset = 0;                                                  // Standard offset, minutes
static uint8_t zoneRule = TZ_RULE_NONE;
static char zoneName[TZ_NAME_MAX + 1] = "UTC";

// ***************************************************************************************
//
//		Calendar helpers (proleptic Gregorian, days since 1970-01-01)
//
// ***************************************************************************************

static uint32_t _TZDays(int y,int m,int d) {
	y -= m <= 2;
	int era = y / 400,yoe = y - era * 400;
	int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
	int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return (uint32_t)(era * 146097 + doe - 719468);
}

static int _TZYear(uint32_t secs) {
	uint32_t z = secs / 86400 + 719468;
	int era = z / 146097,doe = z - era * 146097;
	int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	int mp = (5 * doy + 2) / 153;
	int m = mp + (mp < 10 ? 3 : -9);
	return yoe + era * 400 + (m <= 2);
}

static uint32_t _TZNthSunday(int y,int m,int n) {                               // n = 1.. from the start, -1 = last ; days since 1970
	static const uint8_t mdays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	int last = mdays[m-1] + ((m == 2 && (y % 4 == 0) && (y % 100 != 0 || y % 400 == 0)) ? 1 : 0);
	if (n < 0) {
		uint32_t d = _TZDays(y,m,last);
		return d - ((d + 4) % 7);                                               // 1970-01-01 = Thursday : (d+4)%7 = weekday, Sunday 0
	}
	uint32_t d = _TZDays(y,m,1);
	int wd = (d + 4) % 7;
	return d + ((7 - wd) % 7) + 7 * (n - 1);
}

// Daylight saving interval [start,end) in UTC seconds for the year of utc, or false if the rule has none.
static bool _TZDstInterval(uint32_t utc,uint32_t *start,uint32_t *end) {
	int y = _TZYear(utc);
	int32_t std = (int32_t)zoneOffset * 60;
	switch (zoneRule) {
		case TZ_RULE_EU:                                                        // 01:00 UTC both
			*start = _TZNthSunday(y,3,-1) * 86400 + 3600;
			*end = _TZNthSunday(y,10,-1) * 86400 + 3600;
			return true;
		case TZ_RULE_NA:                                                        // 02:00 local standard / 02:00 local daylight
			*start = _TZNthSunday(y,3,2) * 86400 + 7200 - std;
			*end = _TZNthSunday(y,11,1) * 86400 + 7200 - std - 3600;
			return true;
		case TZ_RULE_AU:                                                        // Southern : DST from October to April
			*start = _TZNthSunday(y,10,1) * 86400 + 7200 - std;
			*end = _TZNthSunday(y,4,1) * 86400 + 10800 - std - 3600;
			return true;
		case TZ_RULE_NZ:
			*start = _TZNthSunday(y,9,-1) * 86400 + 7200 - std;
			*end = _TZNthSunday(y,4,1) * 86400 + 10800 - std - 3600;
			return true;
	}
	return false;
}

int16_t TZOffsetAt(uint32_t utc,uint8_t *dst) {
	uint32_t start,end;
	*dst = 0;
	if (_TZDstInterval(utc,&start,&end)) {
		bool in = (start < end) ? (utc >= start && utc < end) : (utc >= start || utc < end);   // Southern : wraps the year end
		if (in) *dst = 1;
	}
	return zoneOffset + (*dst ? 60 : 0);
}

uint32_t TZLocalToUTC(uint32_t local) {
	uint8_t dst;
	uint32_t utc = local - (int32_t)zoneOffset * 60;                            // Standard time first
	int16_t off = TZOffsetAt(utc,&dst);
	if (dst) utc = local - (int32_t)off * 60;                                   // Daylight time then (ambiguous hour : daylight)
	return utc;
}

// ***************************************************************************************
//
//		Select a zone : table name (case insensitive) or "UTC+h", "UTC-h:mm", "+0530"
//
// ***************************************************************************************

static bool _TZEqual(const char *a,const char *b) {
	while (*a && *b) { if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;a++;b++; }
	return *a == 0 && *b == 0;
}

uint8_t TZSet(const char *name) {
	for (const struct Zone *z = zones;z->name != NULL;z++) {
		if (_TZEqual(z->name,name)) {
			zoneOffset = z->offset;zoneRule = z->rule;
			strncpy(zoneName,z->name,TZ_NAME_MAX);zoneName[TZ_NAME_MAX] = 0;
			return 0;
		}
	}
	const char *p = name;                                                       // Numeric : [UTC|GMT][+|-]h[:mm] or +hhmm
	if (strncasecmp(p,"UTC",3) == 0 || strncasecmp(p,"GMT",3) == 0) p += 3;
	if (*p != '+' && *p != '-') return 1;
	int sign = (*p == '-') ? -1 : 1;p++;
	if (!isdigit((unsigned char)*p)) return 1;
	int h = 0,m = 0,digits = 0;
	while (isdigit((unsigned char)*p)) { h = h * 10 + (*p - '0');p++;digits++; }
	if (digits == 4) { m = h % 100;h /= 100; }
	else if (*p == ':') { p++;if (!isdigit((unsigned char)p[0]) || !isdigit((unsigned char)p[1])) return 1;m = (p[0]-'0')*10 + (p[1]-'0');p += 2; }
	if (*p != 0 || h > 14 || m > 59) return 1;
	zoneOffset = sign * (h * 60 + m);zoneRule = TZ_RULE_NONE;
	snprintf(zoneName,sizeof(zoneName),"UTC%c%d%s%02d",sign < 0 ? '-' : '+',h,m ? ":" : "",m);
	if (m == 0) snprintf(zoneName,sizeof(zoneName),"UTC%c%d",sign < 0 ? '-' : '+',h);
	return 0;
}

const char *TZGetName(void) { return zoneName; }

// Zone kept in the settings sector (T-26) : applied at start up, written by 1,24.
void TZLoadFromStorage(void) {
	const char *name = SETGetString(SETTINGS_ZONE);
	if (name[0] && TZSet(name) == 0) CONWriteString("Time zone %s\r",zoneName);
}

uint8_t TZSetAndSave(const char *name) {
	if (TZSet(name) != 0) return 1;
	return SETSetString(SETTINGS_ZONE,SETTINGS_ZONE_LEN,zoneName) ? 2 : 0;
}
