// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      settings.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      21st September 2026
//      Purpose :   Persistent settings (see settings.h).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "interface/settings.h"

static uint8_t record[SETTINGS_SIZE];
static bool loaded = false;

bool SETLoad(void) {
	const uint8_t *s = HWSettingsStorage();
	loaded = true;
	if (s != NULL && memcmp(s,SETTINGS_MAGIC,4) == 0) { memcpy(record,s,SETTINGS_SIZE);return true; }
	memset(record,0,SETTINGS_SIZE);
	memcpy(record,SETTINGS_MAGIC,4);
	return false;
}

const uint8_t *SETRecord(void) { if (!loaded) SETLoad();return record; }

uint8_t SETWrite(void) { if (!loaded) SETLoad();return HWSettingsWrite(record); }

const char *SETGetString(uint16_t offset) {
	if (!loaded) SETLoad();
	return (const char *)record + offset;
}

uint8_t SETSetString(uint16_t offset,uint16_t maxLen,const char *value) {
	if (!loaded) SETLoad();
	if (strlen(value) >= maxLen || offset + maxLen > SETTINGS_SIZE) return 1;
	if (strcmp((const char *)record + offset,value) == 0) return 0;              // Unchanged : no flash write
	memset(record + offset,0,maxLen);
	strcpy((char *)record + offset,value);
	return SETWrite();
}
