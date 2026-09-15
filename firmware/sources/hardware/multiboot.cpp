// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      multiboot.cpp
//      Author :    bmarty <bmarty@mailo.com>
//      Purpose :   API 1,14 / 1,15 : reboot on another flash image, slot names (F-81).
//                  The layout comes from multiboot/neoboot.h ; when the selector is not
//                  installed (firmware flashed at 0 as usual) the directory magic is
//                  absent and both calls fail cleanly.
//      NOT YET RUN ON A BOARD.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"
#include "../../multiboot/neoboot.h"

static const struct neoboot_directory *MBDirectory(void) {
	const struct neoboot_directory *d = (const struct neoboot_directory *)(XIP_BASE + NEOBOOT_DIR_OFFSET);
	return (d->magic == NEOBOOT_MAGIC) ? d : NULL;
}

uint8_t HWGetImageName(uint8_t slot, char *name, int maxLen) {
	const struct neoboot_directory *d = MBDirectory();
	if (d == NULL || slot >= NEOBOOT_SLOTS || d->name[slot][0] == 0) return 1;
	strncpy(name, d->name[slot], maxLen - 1);name[maxLen - 1] = 0;
	return 0;
}

uint8_t HWRebootImage(uint8_t slot) {
	char name[NEOBOOT_NAME_LEN];
	if (HWGetImageName(slot, name, sizeof(name)) != 0) return 1;
	watchdog_hw->scratch[0] = NEOBOOT_MAGIC | slot;  								// Read by the selector after the reset.
	watchdog_reboot(0, 0, 10);  													// Standard boot (bootrom -> selector's boot2 -> selector).
	while (1) tight_loop_contents();
}
