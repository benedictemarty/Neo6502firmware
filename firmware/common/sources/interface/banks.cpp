// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      banks.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      21st September 2026
//      Purpose :   Memory banks in flash (see banks.h). The bus loop serves every 65C02 read
//                  from cpuMemory[] on a time critical path, so there is no per access bank
//                  test : a bank is copied into the window while the 65C02 is stalled on the
//                  API call (as 3,2/3,8 write cpuMemory during a call).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

static uint8_t currentBank = BANK_NONE;                                          // Bank mapped in the window
static uint16_t currentAddress = 0;                                              // Window address

void BNKReset(void) {
    currentBank = BANK_NONE;
    currentAddress = 0;
}

static bool _BNKWindowOk(uint16_t address) {
    return (address & 0xFF) == 0 && (uint32_t)address + BANK_SIZE <= 0xFF00;    // Page aligned, below the API page
}

// 1,18 : copy bank into the window (BANK_NONE : forget the mapping, the window keeps its content)
uint8_t BNKSelect(uint8_t bank,uint16_t address) {
    if (bank == BANK_NONE) { currentBank = BANK_NONE;return 0; }
    if (bank >= BANK_COUNT || !_BNKWindowOk(address)) return 1;
    const uint8_t *src = HWBankStorage(bank);
    if (src == NULL) return 1;
    memcpy(cpuMemory + address,src,BANK_SIZE);
    currentBank = bank;currentAddress = address;
    return 0;
}

void BNKGetState(uint8_t *bank,uint16_t *address) {
    *bank = currentBank;
    *address = (currentBank == BANK_NONE) ? 0 : currentAddress;
}

// 1,22 : program bank from the 8k at address (flash erase + program on the board)
uint8_t BNKWrite(uint8_t bank,uint16_t address) {
    if (bank >= BANK_COUNT || !_BNKWindowOk(address)) return 1;
    return HWBankWrite(bank,cpuMemory + address) ? 1 : 0;
}

// 1,26 : Fletcher 16 of the bank as in flash (T-81) : s1 = sum of the bytes, s2 = sum of the successive s1, mod 65536
uint8_t BNKChecksum(uint8_t bank,uint16_t *s1,uint16_t *s2) {
    const uint8_t *src = BNKStorage(bank);
    if (src == NULL) return 1;
    uint16_t a = 0,b = 0;
    for (int i = 0;i < BANK_SIZE;i++) { a += src[i];b += a; }
    *s1 = a;*s2 = b;
    return 0;
}

const uint8_t *BNKStorage(uint8_t bank) {
    return (bank < BANK_COUNT) ? HWBankStorage(bank) : NULL;
}
