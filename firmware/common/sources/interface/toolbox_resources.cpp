// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      toolbox_resources.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      18th September 2026
//      Purpose :   Toolbox group 38, Resource Manager (see resources.h).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

// Trinity (T-12, 2026-09-20) : taken from the fork ; the file name goes through a fixed buffer (T-13).

static uint8_t rsChannel = 0xFF;                                                // Channel of the open resource file, $FF none
static uint8_t rsCount = 0;

static uint8_t _RSReadAt(uint32_t offset,uint8_t *dest,uint16_t size) {
    if (FISSeekFileHandle(rsChannel,offset) != 0) return RS_ERR_IO;
    uint16_t n = size;
    if (FISReadFileHandleBuffer(rsChannel,dest,&n) != 0 || n != size) return RS_ERR_IO;
    return RS_ERR_OK;
}

static uint8_t _RSEntry(uint8_t index,struct RSEntry *e) {                      // index 1..count
    if (rsChannel == 0xFF) return RS_ERR_PARAM;
    if (index < 1 || index > rsCount) return RS_ERR_NOTFOUND;
    uint8_t raw[16];
    uint8_t err = _RSReadAt(8 + 16 * (uint32_t)(index - 1),raw,16);
    if (err != 0) return err;
    memcpy(e->type,raw,4);
    e->id = raw[4] | (raw[5] << 8);
    e->offset = raw[6] | (raw[7] << 8) | ((uint32_t)raw[8] << 16) | ((uint32_t)raw[9] << 24);
    e->size = raw[10] | (raw[11] << 8) | ((uint32_t)raw[12] << 16) | ((uint32_t)raw[13] << 24);
    return RS_ERR_OK;
}

void RSReset(void) { rsChannel = 0xFF;rsCount = 0; }

uint8_t RSOpen(uint8_t channel,uint16_t nameAddr) {
    if (channel >= FIO_NUM_FILES || nameAddr > 0xFF00 - 1) return RS_ERR_PARAM;
    if (rsChannel != 0xFF) RSClose();
    char name[64];                                                              // Fixed buffer (T-13) ; FIOOpenFileHandle wants a std::string
    uint8_t len = cpuMemory[nameAddr];
    if (len >= sizeof(name) || (uint32_t)nameAddr + len > 0xFF00 - 1) return RS_ERR_PARAM;
    memcpy(name,cpuMemory + nameAddr + 1,len);name[len] = 0;
    if (FIOOpenFileHandle(channel,std::string(name),FIOMODE_RDONLY) != 0) return RS_ERR_IO;
    rsChannel = channel;
    uint8_t header[8];
    if (_RSReadAt(0,header,8) != 0 || header[0] != 'N' || header[1] != 'R' || header[2] != 1) {
        FIOCloseFileHandle(channel);rsChannel = 0xFF;
        return RS_ERR_PARAM;
    }
    rsCount = header[3];
    return RS_ERR_OK;
}

uint8_t RSClose(void) {
    if (rsChannel == 0xFF) return RS_ERR_PARAM;
    FIOCloseFileHandle(rsChannel);
    rsChannel = 0xFF;rsCount = 0;
    return RS_ERR_OK;
}

uint8_t RSCount(uint8_t *count) {
    *count = rsCount;
    return (rsChannel == 0xFF) ? RS_ERR_PARAM : RS_ERR_OK;
}

uint8_t RSFind(const uint8_t *type,uint16_t id,uint8_t *index) {
    *index = 0;
    if (rsChannel == 0xFF) return RS_ERR_PARAM;
    for (uint8_t i = 1;i <= rsCount;i++) {
        struct RSEntry e;
        uint8_t err = _RSEntry(i,&e);
        if (err != 0) return err;
        if (memcmp(e.type,type,4) == 0 && e.id == id) { *index = i;return RS_ERR_OK; }
    }
    return RS_ERR_NOTFOUND;
}

uint8_t RSInfo(uint8_t index,struct RSEntry *entry) { return _RSEntry(index,entry); }

// Load resource index into page:address (pages of 12,2 : $00 6502 RAM, $90 graphics RAM), at most *size bytes ; *size = bytes read.
uint8_t RSLoad(uint8_t index,uint8_t page,uint16_t address,uint16_t *size) {
    struct RSEntry e;
    uint8_t err = _RSEntry(index,&e);
    if (err != 0) { *size = 0;return err; }
    uint16_t n = (e.size < *size) ? (uint16_t)e.size : *size;
    *size = 0;
    if (n == 0) return RS_ERR_OK;
    if (FISSeekFileHandle(rsChannel,e.offset) != 0) return RS_ERR_IO;
    uint16_t got = n;
    if (FIOReadFileHandlePaged(rsChannel,page,address,&got) != 0) return (page == 0 || BLTGetRealAddress(page,address) != NULL) ? RS_ERR_IO : RS_ERR_PARAM;
    *size = got;
    return (got == n) ? RS_ERR_OK : RS_ERR_IO;
}

// Load an NF1 font resource into page:address and select it (32,15).
uint8_t RSUseFont(uint8_t index,uint8_t page,uint16_t address,uint16_t *size) {
    uint8_t err = RSLoad(index,page,address,size);
    if (err != 0) return err;
    return (QDSetFont(page,address) == QD_ERR_OK) ? RS_ERR_OK : RS_ERR_PARAM;
}
