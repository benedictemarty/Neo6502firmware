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

void     CDCPushBack(const uint8_t *data,uint16_t count);  						// Device 0 : give bytes back to the next reader (T-25)
uint16_t CDCRead(uint8_t dev,uint8_t *buffer,uint16_t max);  					// Pushed back bytes first, then the FIFO
uint16_t CDCReadAvailable(uint8_t dev);
uint8_t CDCStatus(uint8_t *params);  											// Group 14 handlers (common)
uint8_t CDCReadByte(uint8_t *params);
uint8_t CDCWriteByte(uint8_t *params);
uint8_t CDCReadBlock(uint8_t *params, uint8_t *cpuMem);
uint8_t CDCWriteBlock(uint8_t *params, uint8_t *cpuMem);
uint8_t CDCSetLineCoding(uint8_t *params);

// UART <-> CDC routing (F-93) : used by group 10 functions 13-19.
void    UARTRouteSet(uint8_t mode);   // 0 hardware UART, 1 CDC device 0, 2 AUTO
uint8_t UARTRouteGet(void);
void    UARTRSetFormat(uint32_t baud, uint32_t protocol);
int     UARTRWriteBlock(uint8_t *data, size_t size);
int     UARTRReadBlock(uint8_t *data, size_t size);
void    UARTRWriteByte(uint8_t b);
bool    UARTRByteAvailable(void);
uint8_t UARTRReadByte(uint8_t *out);

#endif

