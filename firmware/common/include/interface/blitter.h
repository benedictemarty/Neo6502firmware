// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      blitter.h
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      24th March 2024
//      Reviewed :  No
//      Purpose :   Blitter header
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _BLITTER_H
#define _BLITTER_H

struct BlitterArea {
	uint16_t	address;
	uint8_t		page;
    uint8_t     padding;     // unused. Should be zero.
	int16_t		stride;		 // Number of bytes between start of each line in memory (in bytes).
	uint8_t		format;      // one of BLTFMT_*
    // Everything below here is ignored for target.
	uint8_t		transparent; // transparent value in src, for BLTACT_MASK and BLTACT_SOLID.
	uint8_t		solid;	 	 // constant value for BLTACT_SOLID.
	uint8_t  	height;		 // Number of lines.
	uint16_t  	width;		 // Size of line, in src units.
};

// Format types for BlitterArea.format field
#define BLTFMT_BYTE 0		// whole byte
#define BLTFMT_PAIR 1		// 2 4-bit values, aka nibbles (src only)
#define BLTFMT_BITS 2		// 8 1-bit values (src only)
#define BLTFMT_HIGH 3		// High nibble (target only)
#define BLTFMT_LOW  4		// Low nibble (target only)
#define BLTFMT_PACKED     5	// Trinity T-90 : 2 pixels per byte, left pixel in the high nibble, first pixel in the HIGH
							// nibble of the first byte (even x) — the 16 colour pages of mode 2 (target only)
#define BLTFMT_PACKED_ODD 6	// Same, first pixel in the LOW nibble of the first byte (odd x) (target only)

const uint8_t *BLTGetAreaEnd(uint8_t page);  									// T-118 : end of a page's area, NULL if illegal
uint8_t *BLTGetRealAddress(uint8_t page,uint16_t address);							// page:address -> pointer, NULL if illegal (T-12 : QuickDraw fonts)
void BLTLoadArea(uint16_t addr,struct BlitterArea *b);													// 12,3 area structure in 6502 RAM (T-12)
uint8_t BLTCopyArea(uint8_t action,const struct BlitterArea *source,const struct BlitterArea *target);	// Areas in firmware memory (T-12)
uint8_t BLTSimpleCopy(uint8_t pageFrom,uint16_t addressFrom, uint8_t pageTo, uint16_t addressTo, uint16_t transferSize);

// Values for BLTComplexCopy and BLTImage action params
#define BLTACT_COPY 0		// Straight rectangle copy
#define BLTACT_MASK 1		// Copy, but only where src != srcarea.transparent
#define BLTACT_SOLID 2		// Fill with constant srcarea.solid value, but only where src != srcarea.transparent
#define BLTACT_TRANSLATE 3	// Trinity T-115 : target = table[src] (12,3 only, table of 256 bytes)
#define BLTACT_TRANSLATE_MASK 4	// Same, but only where src != srcarea.transparent

uint8_t BLTComplexCopy(uint8_t action,uint16_t aSource,uint16_t aTarget,uint16_t aTable,uint8_t tablePage);

uint8_t BLTImage(uint8_t action, uint16_t sourceArea, int16_t x, int16_t y, uint8_t destFmt);

#endif
// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
