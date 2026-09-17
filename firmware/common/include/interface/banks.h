// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      banks.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      17th September 2026
//      Purpose :   Memory banks for the 65C02 (F-23) : bank storage outside 6502 RAM,
//                  switched by copy during the API call (window in 6502 RAM).
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define BANK_SIZE   0x2000                                                      // 8k window / bank
#define BANK_COUNT  2                                                           // 16k : 4 banks (32k) overflow the RP2040 RAM by 10.5k (R22)
#define BANK_NONE   0xFF                                                        // No bank mapped
#define BANK_PAGE   0xA0                                                        // Blitter page of bank 0 (bank n = $A0+n)

uint8_t BNKSelect(uint8_t bank,uint16_t address);
void BNKGetState(uint8_t *bank,uint16_t *address);
void BNKReset(void);
uint8_t *BNKStorage(uint8_t bank);                                              // Storage of a bank (NULL if none)
