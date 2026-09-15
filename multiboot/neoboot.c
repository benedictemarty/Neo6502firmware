// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      neoboot.c
//      Author :    bmarty <bmarty@mailo.com>
//      Purpose :   Multi-boot selector for the RP2040 of the Neo6502 (F-80).
//
//      Lives at the start of flash (its own boot2 runs first, as usual). Reads the slot
//      requested by the previous firmware in watchdog scratch[0] (survives a watchdog
//      reboot, cleared by a power cycle), falls back to the default slot, then :
//        1. copies the chosen image's boot2 (first 256 bytes of the slot) to RAM and
//           calls it with LR set, so that the image gets *its* flash configuration
//           (PICO_FLASH_SPI_CLKDIV...) exactly as if it had been flashed at 0 ;
//        2. vectors into the image at slot + 0x100 (VTOR, MSP, reset handler) — the
//           same entry as boot2's vector_into_flash, just at another base.
//      Each image must be linked for its slot (multiboot/memmap_slot_*.ld).
//
//      NOT YET RUN ON A BOARD.
//
// ***************************************************************************************
// ***************************************************************************************

#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/structs/watchdog.h"
#include "hardware/structs/scb.h"
#include "hardware/sync.h"
#include "neoboot.h"

static void __attribute__((noreturn)) launch(uint32_t slotBase) {
	static uint8_t boot2Copy[256] __attribute__((aligned(4)));
	memcpy(boot2Copy, (const void *)slotBase, 256);  								// The image's own boot2 (checksummed by its build).
	__compiler_memory_barrier();
	void (*boot2)(void) = (void (*)(void))((uint32_t)boot2Copy + 1);  			// Thumb call : boot2 returns to LR when LR != 0.
	boot2();  																	// Flash configured for that image.
	uint32_t *vt = (uint32_t *)(slotBase + 0x100);  								// Vector table of the image.
	scb_hw->vtor = (uint32_t)vt;
	__asm volatile ("msr msp, %0\n bx %1" : : "r"(vt[0]), "r"(vt[1]) : "memory");
	__builtin_unreachable();
}

int main(void) {
	int slot = NEOBOOT_DEFAULT_SLOT;
	uint32_t req = watchdog_hw->scratch[0];
	if ((req & 0xFFFFFF00u) == NEOBOOT_MAGIC) {  									// Requested by API 1,14 before the reboot.
		slot = req & 0xFF;
		watchdog_hw->scratch[0] = 0;  												// One shot : a plain reset goes back to the default.
	}
	if (slot >= NEOBOOT_SLOTS || !neoboot_slot_present(slot)) slot = NEOBOOT_DEFAULT_SLOT;
	if (!neoboot_slot_present(slot)) {  													// Nothing to run at all : blink the LED forever.
		gpio_init(PICO_DEFAULT_LED_PIN);gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
		while (1) { gpio_xor_mask(1u << PICO_DEFAULT_LED_PIN);sleep_ms(200); }
	}
	launch(neoboot_slot_base(slot));
}
