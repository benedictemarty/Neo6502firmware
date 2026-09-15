// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      multiboot.cpp
//      Author :    bmarty <bmarty@mailo.com>
//      Purpose :   API 1,14 / 1,15 : reboot on another flash image, slot names (F-81).
//                  The layout comes from multiboot/neoboot.h : a slot is "there" when it
//                  holds a plausible image, and its name is read from the image's own
//                  binary_info — the menu follows whatever is actually in flash. When
//                  this firmware runs at flash 0 (no selector) slot 0 overlaps it and
//                  the other slots are erased flash : both calls fail cleanly.
//      NOT YET RUN ON A BOARD.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"
#include "../../multiboot/neoboot.h"

static bool MBSelectorInstalled(void) {  											// This firmware was linked for a slot and runs from it.
	extern char __flash_binary_start[];
	return (uint32_t)__flash_binary_start >= XIP_BASE + NEOBOOT_SLOT0_OFFSET;
}

uint8_t HWGetImageName(uint8_t slot, char *name, int maxLen) {
	if (!MBSelectorInstalled() || !neoboot_slot_present(slot)) return 1;
	const char *n = neoboot_slot_name(slot);
	if (n == NULL) snprintf(name, maxLen, "image %d", slot);  						// No binary_info : still selectable.
	else { strncpy(name, n, maxLen - 1);name[maxLen - 1] = 0; }
	return 0;
}

uint8_t HWRebootImage(uint8_t slot) {
	if (!MBSelectorInstalled() || !neoboot_slot_present(slot)) return 1;
	watchdog_hw->scratch[0] = NEOBOOT_MAGIC | slot;  								// Read by the selector after the reset.
	watchdog_reboot(0, 0, 10);  													// Standard boot (bootrom -> selector's boot2 -> selector).
	while (1) tight_loop_contents();
}
