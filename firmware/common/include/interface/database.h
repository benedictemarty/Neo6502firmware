// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      database.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      9th November 2026
//      Purpose :   Toolbox group 40, Database (T-138) : fixed length records in a file on the storage.
//                  Nothing is cached in the RP2040 but the channel, the record size and the count (R22 : no
//                  SRAM to spare) ; records are read straight into any memory page the blitter can address
//                  (like 3,27 and 38,7), so the data stays on the USB key and not in memory.
//
//      ND1 file : 'N','D',1,0, record size u16, record count u32, 6 reserved bytes (16 bytes), then the
//      records 1..count of `record size` bytes. Byte 0 of a record is its status : 1 active, 0 deleted.
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define DB_ERR_OK        0
#define DB_ERR_PARAM     1                                                      // Bad parameter / not a database / none open / range
#define DB_ERR_IO        2                                                      // Storage error
#define DB_ERR_NOTFOUND  3                                                      // Record number out of range / no match

#define DB_HEADER        16
#define DB_MAX_RECORD    4096
#define DB_MAX_RECORDS   65535                                                  // Record numbers are 16 bit
#define DB_SEARCH_SPAN   96                                                     // Offset + length of a searched field

void DBReset(void);
uint8_t DBCreate(uint8_t channel,uint16_t nameAddr,uint16_t recSize);           // 40,1
uint8_t DBOpen(uint8_t channel,uint16_t nameAddr);                              // 40,2
uint8_t DBClose(void);                                                          // 40,3
uint8_t DBInfo(uint16_t *count,uint16_t *recSize);                              // 40,4
uint8_t DBRead(uint16_t rec,uint8_t page,uint16_t address);                     // 40,5
uint8_t DBUpdate(uint16_t rec,uint16_t address);                                // 40,6
uint8_t DBAppend(uint16_t address,uint16_t *rec);                               // 40,7
uint8_t DBDelete(uint16_t rec);                                                 // 40,8
uint8_t DBSearch(uint16_t start,uint8_t offset,uint8_t length,uint16_t pattern,uint16_t *rec);  // 40,9
