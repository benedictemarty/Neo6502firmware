// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      toolbox_database.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      9th November 2026
//      Purpose :   Toolbox group 40, Database (T-138, see database.h).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

static uint8_t dbChannel = 0xFF;                                                // Channel of the open database, $FF none
static uint16_t dbCount = 0,dbSize = 0;                                         // Record count and record size

static uint8_t _DBSeekRecord(uint16_t rec) {                                    // rec 1..count
    if (dbChannel == 0xFF) return DB_ERR_PARAM;
    if (rec < 1 || rec > dbCount) return DB_ERR_NOTFOUND;
    return (FISSeekFileHandle(dbChannel,DB_HEADER + (uint32_t)(rec - 1) * dbSize) == 0) ? DB_ERR_OK : DB_ERR_IO;
}

static bool _DBCopyName(uint16_t nameAddr,char *name,uint8_t max) {             // Length-prefixed name -> fixed buffer
    if (nameAddr > 0xFF00 - 1) return false;
    uint8_t len = cpuMemory[nameAddr];
    if (len >= max || (uint32_t)nameAddr + len > 0xFF00 - 1) return false;
    memcpy(name,cpuMemory + nameAddr + 1,len);name[len] = 0;
    return true;
}

static uint8_t _DBWriteCount(void) {                                            // Header bytes 6..9
    uint8_t raw[4] = { (uint8_t)dbCount,(uint8_t)(dbCount >> 8),0,0 };
    if (FISSeekFileHandle(dbChannel,6) != 0) return DB_ERR_IO;
    uint16_t n = 4;
    return (FISWriteFileHandleBuffer(dbChannel,raw,&n) == 0) ? DB_ERR_OK : DB_ERR_IO;
}

void DBReset(void) {                                                            // T-142 : a program that ended without 40,3 Close left its file channel open
    if (dbChannel != 0xFF) FIOCloseFileHandle(dbChannel);                       // (the next Create / Open on it failed on the board)
    dbChannel = 0xFF;dbCount = 0;dbSize = 0;
}

uint8_t DBCreate(uint8_t channel,uint16_t nameAddr,uint16_t recSize) {
    if (channel >= FIO_NUM_FILES || recSize < 2 || recSize > DB_MAX_RECORD) return DB_ERR_PARAM;
    if (channel == dbChannel) return DB_ERR_PARAM;
    char name[64];
    if (!_DBCopyName(nameAddr,name,sizeof(name))) return DB_ERR_PARAM;
    if (FIOOpenFileHandle(channel,std::string(name),FIOMODE_RDWR_CREATE) != 0) return DB_ERR_IO;
    uint8_t header[DB_HEADER] = { 'N','D',1,0,(uint8_t)recSize,(uint8_t)(recSize >> 8),0,0,0,0,0,0,0,0,0,0 };
    uint16_t n = DB_HEADER;
    uint8_t err = (FISWriteFileHandleBuffer(channel,header,&n) == 0) ? DB_ERR_OK : DB_ERR_IO;
    FIOCloseFileHandle(channel);
    return err;
}

uint8_t DBOpen(uint8_t channel,uint16_t nameAddr) {
    if (channel >= FIO_NUM_FILES) return DB_ERR_PARAM;
    if (dbChannel != 0xFF) DBClose();
    char name[64];
    if (!_DBCopyName(nameAddr,name,sizeof(name))) return DB_ERR_PARAM;
    if (FIOOpenFileHandle(channel,std::string(name),FIOMODE_RDWR) != 0) return DB_ERR_IO;
    uint8_t h[DB_HEADER];
    uint16_t n = DB_HEADER;
    uint16_t size = 0;
    if (FISSeekFileHandle(channel,0) == 0 && FISReadFileHandleBuffer(channel,h,&n) == 0 && n == DB_HEADER) size = h[4] | (h[5] << 8);
    if (size < 2 || size > DB_MAX_RECORD || h[0] != 'N' || h[1] != 'D' || h[2] != 1 || h[8] != 0 || h[9] != 0) {                  // Count is u32 in the file, u16 here
        FIOCloseFileHandle(channel);
        return DB_ERR_PARAM;
    }
    dbChannel = channel;dbSize = size;dbCount = (uint16_t)(h[6] | (h[7] << 8));
    return DB_ERR_OK;
}

uint8_t DBClose(void) {
    if (dbChannel == 0xFF) return DB_ERR_PARAM;
    DBReset();                                                                  // closes the channel
    return DB_ERR_OK;
}

uint8_t DBInfo(uint16_t *count,uint16_t *recSize) {
    *count = dbCount;*recSize = dbSize;
    return (dbChannel == 0xFF) ? DB_ERR_PARAM : DB_ERR_OK;
}

uint8_t DBRead(uint16_t rec,uint8_t page,uint16_t address) {
    uint8_t err = _DBSeekRecord(rec);
    if (err != 0) return err;
    uint16_t got = dbSize;
    if (FIOReadFileHandlePaged(dbChannel,page,address,&got) != 0) return (page == 0 || BLTGetRealAddress(page,address) != NULL) ? DB_ERR_IO : DB_ERR_PARAM;
    return (got == dbSize) ? DB_ERR_OK : DB_ERR_IO;
}

uint8_t DBUpdate(uint16_t rec,uint16_t address) {
    if ((uint32_t)address + dbSize > 0xFF00) return DB_ERR_PARAM;
    uint8_t err = _DBSeekRecord(rec);
    if (err != 0) return err;
    uint16_t n = dbSize;
    return (FISWriteFileHandle(dbChannel,address,&n) == 0) ? DB_ERR_OK : DB_ERR_IO;
}

uint8_t DBAppend(uint16_t address,uint16_t *rec) {
    *rec = 0;
    if (dbChannel == 0xFF || (uint32_t)address + dbSize > 0xFF00 || dbCount >= DB_MAX_RECORDS) return DB_ERR_PARAM;
    if (FISSeekFileHandle(dbChannel,DB_HEADER + (uint32_t)dbCount * dbSize) != 0) return DB_ERR_IO;
    uint16_t n = dbSize;
    if (FISWriteFileHandle(dbChannel,address,&n) != 0) return DB_ERR_IO;
    dbCount++;
    uint8_t err = _DBWriteCount();
    if (err != 0) { dbCount--;return err; }
    *rec = dbCount;
    return DB_ERR_OK;
}

uint8_t DBDelete(uint16_t rec) {                                                // Status byte of the record -> 0
    uint8_t err = _DBSeekRecord(rec);
    if (err != 0) return err;
    uint8_t zero = 0;
    uint16_t n = 1;
    return (FISWriteFileHandleBuffer(dbChannel,&zero,&n) == 0) ? DB_ERR_OK : DB_ERR_IO;
}

// First active record from `start` whose bytes offset..offset+length-1 equal the bytes at `pattern` (6502 RAM).
uint8_t DBSearch(uint16_t start,uint8_t offset,uint8_t length,uint16_t pattern,uint16_t *rec) {
    *rec = 0;
    if (dbChannel == 0xFF || length == 0 || (uint16_t)offset + length > DB_SEARCH_SPAN || (uint16_t)offset + length > dbSize) return DB_ERR_PARAM;
    if ((uint32_t)pattern + length > 0xFF00) return DB_ERR_PARAM;
    uint8_t buf[DB_SEARCH_SPAN];
    for (uint32_t r = (start < 1) ? 1 : start;r <= dbCount;r++) {
        uint8_t err = _DBSeekRecord((uint16_t)r);
        if (err != 0) return err;
        uint16_t n = offset + length;
        if (FISReadFileHandleBuffer(dbChannel,buf,&n) != 0 || n != offset + length) return DB_ERR_IO;
        if (buf[0] == 1 && memcmp(buf + offset,cpuMemory + pattern,length) == 0) { *rec = (uint16_t)r;return DB_ERR_OK; }
    }
    return DB_ERR_NOTFOUND;
}
