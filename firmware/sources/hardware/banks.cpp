// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      banks.cpp (board)
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      21st September 2026
//      Purpose :   Bank storage in the RP2040 flash (T-17) : the top BANK_COUNT * BANK_SIZE bytes
//                  of the 2 MB flash, read in place through XIP. Writing a bank : the DVI is
//                  suspended (core 1 parked in RAM, DMA stopped : the screen goes black for the
//                  erase + program, about 150 ms), interrupts are off on core 0 (the 65C02 is
//                  stalled on the API call anyway), then the DVI restarts in the current mode.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "interface/settings.h"

#define BANK_FLASH_OFFSET   (PICO_FLASH_SIZE_BYTES - BANK_COUNT * BANK_SIZE)      // 0x1C0000 : 256k at the top
#define SETTINGS_FLASH_OFFSET (BANK_FLASH_OFFSET - FLASH_SECTOR_SIZE)           // 0x1BF000 : one 4k sector of settings (T-26)

const uint8_t *HWSettingsStorage(void) { return (const uint8_t *)(XIP_BASE + SETTINGS_FLASH_OFFSET); }

uint8_t HWSettingsWrite(const uint8_t *data) {
	static uint8_t page[FLASH_PAGE_SIZE];                                          // 256 bytes : one flash page
	memcpy(page,data,SETTINGS_SIZE);
	RNDSuspend();
	uint32_t ints = save_and_disable_interrupts();
	flash_range_erase(SETTINGS_FLASH_OFFSET,FLASH_SECTOR_SIZE);
	flash_range_program(SETTINGS_FLASH_OFFSET,page,FLASH_PAGE_SIZE);
	restore_interrupts(ints);
	RNDResume();
	return memcmp(page,HWSettingsStorage(),SETTINGS_SIZE) == 0 ? 0 : 1;
}

const uint8_t *HWBankStorage(uint8_t bank) {
	if (bank >= BANK_COUNT) return NULL;
	return (const uint8_t *)(XIP_BASE + BANK_FLASH_OFFSET + bank * BANK_SIZE);
}

uint8_t HWBankWrite(uint8_t bank,const uint8_t *data) {
	if (bank >= BANK_COUNT) return 1;
	RNDSuspend();                                                                  // Core 1 parked (in RAM), DMA and PIO stopped ; data is in cpuMemory (RAM), no copy                                                                  // Core 1 parked (in RAM), DMA and PIO stopped
	uint32_t ints = save_and_disable_interrupts();
	flash_range_erase(BANK_FLASH_OFFSET + bank * BANK_SIZE,BANK_SIZE);            // 2 sectors of 4k
	flash_range_program(BANK_FLASH_OFFSET + bank * BANK_SIZE,data,BANK_SIZE);
	restore_interrupts(ints);
	RNDResume();
	return memcmp(data,HWBankStorage(bank),BANK_SIZE) == 0 ? 0 : 1;              // Verify
}
