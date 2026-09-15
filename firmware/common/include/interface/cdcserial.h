// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      cdcserial.h
//      Author :    bmarty <bmarty@mailo.com>
//      Purpose :   USB CDC-ACM serial devices plugged into the Neo6502 host port (modems,
//                  USB-serial adapters) — API group 14 (F-90). Implementation specific
//                  part : HWCDC* (board : TinyUSB cdc_host ; emulators : host tty/pty).
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _CDCSERIAL_H
#define _CDCSERIAL_H

#define CDC_MAX_DEVICES 	(2)

int  HWCDCConnected(uint8_t dev);  												// 1 if a CDC device dev is mounted
uint16_t HWCDCReadAvailable(uint8_t dev);
uint16_t HWCDCRead(uint8_t dev, uint8_t *buffer, uint16_t max);
uint16_t HWCDCWrite(uint8_t dev, const uint8_t *buffer, uint16_t count);
uint8_t HWCDCSetLineCoding(uint8_t dev, uint32_t baud, uint8_t dataBits, uint8_t parity, uint8_t stopBits);

uint8_t CDCStatus(uint8_t *params);  											// Group 14 handlers (common)
uint8_t CDCReadByte(uint8_t *params);
uint8_t CDCWriteByte(uint8_t *params);
uint8_t CDCReadBlock(uint8_t *params, uint8_t *cpuMem);
uint8_t CDCWriteBlock(uint8_t *params, uint8_t *cpuMem);
uint8_t CDCSetLineCoding(uint8_t *params);

#endif
