// neoboot.h — flash layout of the Neo6502 multi-boot (F-80). Shared by the selector,
// the Neo firmware (API 1,14 / 1,15) and mkimage.py (keep the numbers in sync).
//
// What is in a slot is discovered from the image itself (no directory to maintain) :
//   - present  : plausible vector table at base+0x100 (SP in SRAM, PC in the slot, thumb)
//   - name     : the Pico SDK binary_info "program name" (bi_program_name / target name),
//                found through the binary_info header that crt0 puts right after the
//                vector table (marker 0x7188ebf2 ... 0xe71aa390, first 256 bytes).
#ifndef NEOBOOT_H
#define NEOBOOT_H
#include <stdint.h>

#define NEOBOOT_MAGIC 			(0x4E454F00u)  			// 'NEO' | slot in watchdog scratch[0]
#define NEOBOOT_SLOTS 			(4)
#define NEOBOOT_DEFAULT_SLOT 	(0)  					// Slot 0 = Neo6502 firmware
#define NEOBOOT_SLOT_SIZE 		(0x78000u)  			// 480 KB per slot
#define NEOBOOT_SLOT0_OFFSET 	(0x10000u)  			// Selector : 0x00000-0x0FFFF

#ifndef XIP_BASE
#define XIP_BASE 				(0x10000000u)
#endif

#define NEOBOOT_BI_MARKER_START	(0x7188ebf2u)
#define NEOBOOT_BI_MARKER_END 	(0xe71aa390u)
#define NEOBOOT_BI_TYPE_ID_STR 	(6)
#define NEOBOOT_BI_TAG_RP 		(0x5052u)  				// 'R','P'
#define NEOBOOT_BI_ID_PROGRAM 	(0x02031c86u)

static inline uint32_t neoboot_slot_base(int slot) {
	return XIP_BASE + NEOBOOT_SLOT0_OFFSET + (uint32_t)slot * NEOBOOT_SLOT_SIZE;
}

static inline int neoboot_slot_present(int slot) {
	if (slot < 0 || slot >= NEOBOOT_SLOTS) return 0;
	uint32_t base = neoboot_slot_base(slot);
	const uint32_t *vt = (const uint32_t *)(base + 0x100);
	uint32_t sp = vt[0], pc = vt[1];
	return (sp >= 0x20000000u && sp <= 0x20042000u) && (pc >= base && pc < base + NEOBOOT_SLOT_SIZE) && (pc & 1);
}

// Program name of the image in the slot, or NULL (image without binary_info).
static inline const char *neoboot_slot_name(int slot) {
	if (!neoboot_slot_present(slot)) return 0;
	uint32_t base = neoboot_slot_base(slot);
	const uint32_t *w = (const uint32_t *)(base + 0x100);
	for (int i = 0; i < 64 - 5; i++) {  											// Header within the first 256 bytes after the vectors.
		if (w[i] != NEOBOOT_BI_MARKER_START || w[i + 4] != NEOBOOT_BI_MARKER_END) continue;
		uint32_t start = w[i + 1], end = w[i + 2];  								// Table of pointers to entries (addresses in the slot).
		if (start < base || end > base + NEOBOOT_SLOT_SIZE || end < start) return 0;
		for (uint32_t p = start; p + 4 <= end; p += 4) {
			uint32_t e = *(const uint32_t *)p;
			if (e < base || e + 12 > base + NEOBOOT_SLOT_SIZE) continue;
			const uint16_t *core = (const uint16_t *)e;  							// {type, tag}, then id, then value
			if (core[0] == NEOBOOT_BI_TYPE_ID_STR && core[1] == NEOBOOT_BI_TAG_RP &&
					*(const uint32_t *)(e + 4) == NEOBOOT_BI_ID_PROGRAM) {
				uint32_t s = *(const uint32_t *)(e + 8);
				if (s >= base && s < base + NEOBOOT_SLOT_SIZE) return (const char *)s;
			}
		}
		return 0;
	}
	return 0;
}
#endif
