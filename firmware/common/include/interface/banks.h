// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      banks.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      21st September 2026
//      Purpose :   Memory banks in flash (T-17, XIP ; from F-23 of the fork, which kept them in
//                  SRAM). BANK_COUNT banks of BANK_SIZE bytes live in a reserved area of the
//                  RP2040 flash and are read in place (XIP) : 1,18 Select Bank copies one into
//                  an 8k window of 6502 RAM while the 65C02 waits for the call ; there is no
//                  write back (the window is a copy). 1,22 Write Bank programs a bank from a
//                  window (erase + program, the display pauses meanwhile) ; a written bank
//                  survives resets. The emulators keep the "flash" in a host file.
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define BANK_SIZE   0x2000                                                      // 8k window / bank
#define BANK_COUNT  32                                                          // 256k of flash (top of the 2 MB)
#define BANK_NONE   0xFF                                                        // No bank mapped
#define BANK_PAGE   0xA0                                                        // Blitter page of bank n = $A0+n (read only)

uint8_t BNKSelect(uint8_t bank,uint16_t address);                               // 1,18
void BNKGetState(uint8_t *bank,uint16_t *address);                              // 1,19
uint8_t BNKWrite(uint8_t bank,uint16_t address);                                // 1,22
void BNKReset(void);
const uint8_t *BNKStorage(uint8_t bank);                                        // Read only view of a bank (NULL if none)

const uint8_t *HWBankStorage(uint8_t bank);                                     // Host : XIP address (board) or file image (emulators)
uint8_t HWBankWrite(uint8_t bank,const uint8_t *data);                          // Host : program the bank, 0 if ok
