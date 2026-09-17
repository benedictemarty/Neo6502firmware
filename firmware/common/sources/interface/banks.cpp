// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      banks.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      17th September 2026
//      Purpose :   Memory banks for the 65C02 (F-23, ADR-03).
//
//      The bus loop serves every 65C02 read from cpuMemory[] on a time-critical path, so
//      there is no per-access bank test. Instead a bank is *copied* into a window of 6502
//      RAM while the 65C02 is stalled on the API call (as 3,2/3,8 already write cpuMemory
//      during a call) ; the bank leaving the window is written back to its storage first.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

static uint8_t bankStorage[BANK_COUNT][BANK_SIZE];                               // Bank contents (outside 6502 RAM)
static uint8_t currentBank = BANK_NONE;                                          // Bank mapped in the window
static uint16_t currentAddress = 0;                                              // Window address

// ***************************************************************************************
//
//      Forget the mapping (reset) : the 6502 program is gone, nothing to write back.
//
// ***************************************************************************************

void BNKReset(void) {
    currentBank = BANK_NONE;
    currentAddress = 0;
}

// ***************************************************************************************
//
//      Map a bank at a window address (BANK_NONE : just write the current bank back).
//      The window is page aligned and must not reach the API/vector page $FF00.
//
// ***************************************************************************************

uint8_t BNKSelect(uint8_t bank,uint16_t address) {
    if (bank != BANK_NONE && bank >= BANK_COUNT) return 1;                       // Unknown bank
    if (bank != BANK_NONE) {
        if ((address & 0xFF) != 0) return 1;                                     // Page aligned
        if ((uint32_t)address + BANK_SIZE > 0xFF00) return 1;                    // Below the API page
    }
    if (currentBank != BANK_NONE) {                                              // Write the outgoing bank back
        memcpy(bankStorage[currentBank],cpuMemory + currentAddress,BANK_SIZE);
    }
    if (bank != BANK_NONE) {                                                     // Bring the new one in
        memcpy(cpuMemory + address,bankStorage[bank],BANK_SIZE);
        currentAddress = address;
    }
    currentBank = bank;
    return 0;
}

// ***************************************************************************************
//
//      Current mapping
//
// ***************************************************************************************

void BNKGetState(uint8_t *bank,uint16_t *address) {
    *bank = currentBank;
    *address = (currentBank == BANK_NONE) ? 0 : currentAddress;
}
