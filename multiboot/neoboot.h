// neoboot.h — flash layout of the Neo6502 multi-boot (F-80). Shared by the selector,
// the Neo firmware (API 1,14 / 1,15) and mkimage.py (keep the numbers in sync).
#ifndef NEOBOOT_H
#define NEOBOOT_H
#include <stdint.h>

#define NEOBOOT_MAGIC 			(0x4E454F00u)  			// 'NEO' | slot in scratch[0]
#define NEOBOOT_SLOTS 			(4)
#define NEOBOOT_DEFAULT_SLOT 	(0)  					// Slot 0 = Neo6502 firmware
#define NEOBOOT_SLOT_SIZE 		(0x78000u)  			// 480 KB per slot
#define NEOBOOT_SLOT0_OFFSET 	(0x10000u)  			// Selector : 0x00000-0x0FFFF (code + directory)
#define NEOBOOT_DIR_OFFSET 		(0x0F000u)  			// Last sector of the selector : slot directory
#define NEOBOOT_NAME_LEN 		(32)

#ifndef XIP_BASE
#define XIP_BASE 				(0x10000000u)
#endif

struct neoboot_directory {
	uint32_t magic;  										// NEOBOOT_MAGIC when written by mkimage.py
	char name[NEOBOOT_SLOTS][NEOBOOT_NAME_LEN];  			// ASCIIZ, "" = empty slot
};

static inline uint32_t neoboot_slot_base(int slot) {
	return XIP_BASE + NEOBOOT_SLOT0_OFFSET + (uint32_t)slot * NEOBOOT_SLOT_SIZE;
}
#endif
