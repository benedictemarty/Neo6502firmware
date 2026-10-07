// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      blitter.cpp
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      24th March 2024
//      Reviewed :  No
//      Purpose :   Blitter code
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

// ***************************************************************************************
//
//						Convert page/address to a physical address
//
// ***************************************************************************************

uint8_t *BLTGetRealAddress(uint8_t page,uint16_t address) {  					// Public : QuickDraw (T-12)
	int pos;
	uint8_t *ptr = NULL;
	switch(page) {
		case 0x00: 																	// CPU Memory
			ptr = cpuMemory + address;break;
		case 0x80:  																// Video RAM
		case 0x81:
			pos = ((page - 0x80) << 16) | address;  								// Offset in video RAM
			if (pos < 320*240) ptr = graphicsMemory + pos; 							// Position in bitmap RAM
			break;
		case 0x90: 																	// Graphic storage RAM
			if (address < GFX_MEMORY_SIZE) ptr = gfxObjectMemory + address;
			break;
		default:  																	// T-17 : bank n = page $A0+n, read only (flash)
			if (page >= BANK_PAGE && page < BANK_PAGE + BANK_COUNT && address < BANK_SIZE) ptr = (uint8_t *)BNKStorage(page - BANK_PAGE) + address;
			break;
	}
	return ptr;
}

// ***************************************************************************************
//
//								Copy Blitter, very simple
//
// ***************************************************************************************

uint8_t BLTSimpleCopy(uint8_t pageFrom,uint16_t addressFrom, uint8_t pageTo, uint16_t addressTo, uint16_t transferSize) {	
	TRACEF("Blit: %02x:%04x to %02x:%04x bytes %04x\n",pageFrom,addressFrom,pageTo,addressTo,transferSize);
	if (transferSize == 0) return 0;
	uint8_t *src = BLTGetRealAddress(pageFrom,addressFrom);  						// Copy from here
	uint8_t *dst = BLTGetRealAddress(pageTo,addressTo);  							// To here.
	if (src == NULL || dst == NULL) return 1;  										// Start both legitimate addresses
	if (pageTo >= BANK_PAGE) return 1;  											// T-17 : banks are read only (flash)
	if (BLTGetRealAddress(pageFrom,addressFrom+transferSize-1) == NULL) return 1; 	// Check end both legitimate addresses
	if (BLTGetRealAddress(pageTo,addressTo+transferSize-1) == NULL) return 1;
	memmove(dst,src,transferSize); 													// Copy it.

	return 0;
}

// ***************************************************************************************
//
//						Add constant to a physical address
//
// ***************************************************************************************

void _BLTAddAddress(struct BlitterArea *ba,uint32_t add) {
	uint32_t a = ba->address + add;  												// Work out the result
	ba->address = a & 0xFFFF; 														// Put address part back
	if (a & 0x10000) ba->page++;  													// If carry out, go to next page.
}

// ***************************************************************************************
//
//							 Load a blitter object structure
//
// ***************************************************************************************

void _BLTLoadBlitterAreaObject(uint16_t addr,struct BlitterArea *b) {
	b->address = cpuMemory[addr] + (cpuMemory[addr+1] << 8);
	b->page = cpuMemory[addr+2];
	// 1 byte of padding
	b->stride = cpuMemory[addr+4] + (cpuMemory[addr+5] << 8);
	b->format = cpuMemory[addr+6];
	// The rest are source-only.
	b->transparent = cpuMemory[addr+7];
	b->solid = cpuMemory[addr+8];
	b->height = cpuMemory[addr+9];
	b->width = cpuMemory[addr+10] + (cpuMemory[addr+11] << 8);
}

// ***************************************************************************************
//
//					BLTComplexCopy() line helpers: straight copy
//
// ***************************************************************************************

// Definition for our line-copy routines.
typedef void (*copyFn)(uint8_t *, const uint8_t *, size_t );

// Copy src to target, whole bytes.
static void copy_ByteToByte(uint8_t *tgt, const uint8_t *src, size_t n) {
	memmove(tgt, src, n);
}

//  Copy src to target low nibble (target high nibble is unchanged).
static void copy_ByteToLow(uint8_t *tgt, const uint8_t *src, size_t n) {
	while(n > 0) {
		*tgt = (*tgt & 0xF0) | (*src & 0x0F);
		++tgt;
		++src;
		--n;
	}
}

//  Copy src to target high nibble (target low nibble is unchanged).
static void copy_ByteToHigh(uint8_t *tgt, const uint8_t *src, size_t n) {
	while(n > 0) {
		*tgt = (*tgt & 0x0F) | (*src << 4);
		++tgt;
		++src;
		--n;
	}
}

// Expand src nibbles into consecutive bytes in target (high nibbles of target will be zeroed).
static void copy_PairToByte(uint8_t *tgt, const uint8_t *src, size_t n) {
	n = n / 2;	// 2 nibbles per byte.
	while(n > 0) {
		*tgt++ = *src >> 4;
		*tgt++ = *src & 0x0F;
		++src;
		--n;
	}
}


// Expand src nibbles into consecutive low nibbles in target (leaving target high nibbles unchanged).
static void copy_PairToLow(uint8_t *tgt, const uint8_t *src, size_t n) {
	n = n / 2;	// 2 nibbles per byte.
	while(n > 0) {
		*tgt = (*tgt & 0xF0) | (*src >> 4);
		++tgt;
		*tgt = (*tgt & 0xF0) | (*src & 0x0F);
		++tgt;
		++src;
		--n;
	}
}

// Expand src nibbles into consecutive high nibbles in target (leaving target low nibbles unchanged).
static void copy_PairToHigh(uint8_t *tgt, const uint8_t *src, size_t n) {
	n = n / 2;	// 2 nibbles per byte.
	while(n > 0) {
		*tgt = (*tgt & 0x0F) | (*src & 0xF0);
		++tgt;
		*tgt = (*tgt & 0x0F) | (*src << 4);
		++tgt;
		++src;
		--n;
	}
}


// Expand src bits into consecutive bytes in target.
static void copy_BitsToByte(uint8_t *tgt, const uint8_t *src, size_t n) {
	n = n / 8;	// 8 bits per byte.
	while(n > 0) {
		for (int i = 7; i >= 0; --i) {
			*tgt++ = (*src >> i) & 0x01;
		}
		++src;
		--n;
	}
}


// Expand src bits into consecutive low nibbles in target (leaving target high nibbles unchanged).
static void copy_BitsToLow(uint8_t *tgt, const uint8_t *src, size_t n) {
	n = n / 8;	// 8 bits per byte.
	while(n > 0) {
		for (int i = 7; i >= 0; --i) {
			*tgt = (*tgt & 0xF0) | ((*src >> i) & 0x01);
			++tgt;
		}
		++src;
		--n;
	}
}

// Expand src bits into consecutive high nibbles in target (leaving target low nibbles unchanged).
static void copy_BitsToHigh(uint8_t *tgt, const uint8_t *src, size_t n) {
	n = n / 8;	// 8 bits per byte.
	while(n > 0) {
		for (int i = 7; i >= 0; --i) {
			*tgt = (*tgt & 0x0F) | (((*src >> i) & 0x01) << 4);
			++tgt;
		}
		++src;
		--n;
	}
}


static copyFn pickCopyFn(uint8_t srcFormat, uint8_t tgtFormat)
{
	switch (srcFormat) {
		case BLTFMT_BYTE:
			switch (tgtFormat) {
				case BLTFMT_BYTE: return copy_ByteToByte;
				case BLTFMT_HIGH: return copy_ByteToHigh;
				case BLTFMT_LOW: return copy_ByteToLow;
				default: return nullptr;
			}
		case BLTFMT_PAIR:
			switch (tgtFormat) {
				case BLTFMT_BYTE: return copy_PairToByte;
				case BLTFMT_HIGH: return copy_PairToHigh;
				case BLTFMT_LOW: return copy_PairToLow;
				default: return nullptr;
			}
		case BLTFMT_BITS:
			switch (tgtFormat) {
				case BLTFMT_BYTE: return copy_BitsToByte;
				case BLTFMT_HIGH: return copy_BitsToHigh;
				case BLTFMT_LOW: return copy_BitsToLow;
				default: return nullptr;
			}
		default: return nullptr;
	}
}


// ***************************************************************************************
//
//			BLTComplexCopy() line helpers: copy with src masking (transparency)
//
// ***************************************************************************************

// Definition for our masked-copy routines.
typedef void (*copyMaskedFn)(uint8_t *, const uint8_t *, size_t, uint8_t);

// Copy src to target, whole bytes.
static void copy_masked_ByteToByte(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent) {
	while (n>0) {
		if (*src != transparent) {
			*tgt = *src;
		}
		++tgt;
		++src;
		--n;
	}
}

//  Copy src to target low nibble (target high nibble is unchanged).
static void copy_masked_ByteToLow(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent) {
	while(n > 0) {
		if (*src != transparent) {
			*tgt = (*tgt & 0xF0) | (*src & 0x0F);
		}
		++tgt;
		++src;
		--n;
	}
}

//  Copy src to target high nibble (target low nibble is unchanged).
static void copy_masked_ByteToHigh(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent) {
	while(n > 0) {
		if (*src != transparent) {
			*tgt = (*tgt & 0x0F) | (*src << 4);
		}
		++tgt;
		++src;
		--n;
	}
}

// Expand src nibbles into consecutive bytes in target (high nibbles of target will be zeroed).
static void copy_masked_PairToByte(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent) {
	n = n / 2;	// 2 nibbles per byte.
	while(n > 0) {
		if ((*src >> 4) != transparent) {
			*tgt = *src >> 4;
		}
		++tgt;
		if ((*src & 0x0F) != transparent) {
			*tgt = *src & 0x0F;
		}
		++tgt;
		++src;
		--n;
	}
}

// Expand src nibbles into consecutive low nibbles in target (leaving target high nibbles unchanged).
static void copy_masked_PairToLow(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent) {
	n = n / 2;	// 2 nibbles per byte.
	while(n > 0) {
		if ((*src >> 4) != transparent) {
			*tgt = (*tgt & 0xF0) | (*src >> 4);
		}
		++tgt;
		if ((*src & 0x0F) != transparent) {
			*tgt = (*tgt & 0xF0) | (*src & 0x0F);
		}
		++tgt;
		++src;
		--n;
	}
}

// Expand src nibbles into consecutive high nibbles in target (leaving target low nibbles unchanged).
static void copy_masked_PairToHigh(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent) {
	n = n / 2;	// 2 nibbles per byte.
	while(n > 0) {
		uint8_t v = *src >> 4;
		if (v != transparent) {
			*tgt = (*tgt & 0x0F) | (v << 4);
		}
		++tgt;
		v = *src & 0x0F;
		if (v != transparent) {
			*tgt = (*tgt & 0x0F) | (v << 4);
		}
		++tgt;
		++src;
		--n;
	}
}


// Expand src bits into consecutive bytes in target.
static void copy_masked_BitsToByte(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent) {
	n = n / 8;	// 8 bits per byte.
	while(n > 0) {
		for (int i = 7; i >= 0; --i) {
			uint8_t v = (*src >> i) & 0x01;
			if (v != transparent) {
				*tgt = v;
			}
			++tgt;
		}
		++src;
		--n;
	}
}


// Expand src bits into consecutive low nibbles in target (leaving target high nibbles unchanged).
static void copy_masked_BitsToLow(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent) {
	n = n / 8;	// 8 bits per byte.
	while(n > 0) {
		for (int i = 7; i >= 0; --i) {
			uint8_t v = (*src >> i) & 0x01;
			if (v != transparent) {
				*tgt = (*tgt & 0xF0) | v;
			}
			++tgt;
		}
		++src;
		--n;
	}
}

// Expand src bits into consecutive high nibbles in target (leaving target low nibbles unchanged).
static void copy_masked_BitsToHigh(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent) {
	n = n / 8;	// 8 bits per byte.
	while(n > 0) {
		for (int i = 7; i >= 0; --i) {
			uint8_t v = (*src >> i) & 0x01;
			if (v != transparent) {
				*tgt = (*tgt & 0x0F) | (v << 4);
			}
			++tgt;
		}
		++src;
		--n;
	}
}

static copyMaskedFn pickCopyMaskedFn(uint8_t srcFormat, uint8_t tgtFormat)
{
	switch (srcFormat) {
		case BLTFMT_BYTE:
			switch (tgtFormat) {
				case BLTFMT_BYTE: return copy_masked_ByteToByte;
				case BLTFMT_HIGH: return copy_masked_ByteToHigh;
				case BLTFMT_LOW: return copy_masked_ByteToLow;
				default: return nullptr;
			}
		case BLTFMT_PAIR:
			switch (tgtFormat) {
				case BLTFMT_BYTE: return copy_masked_PairToByte;
				case BLTFMT_HIGH: return copy_masked_PairToHigh;
				case BLTFMT_LOW: return copy_masked_PairToLow;
				default: return nullptr;
			}
		case BLTFMT_BITS:
			switch (tgtFormat) {
				case BLTFMT_BYTE: return copy_masked_BitsToByte;
				case BLTFMT_HIGH: return copy_masked_BitsToHigh;
				case BLTFMT_LOW: return copy_masked_BitsToLow;
				default: return nullptr;
			}
		default: return nullptr;
	}
}



// ***************************************************************************************
//
//					BLTComplexCopy() line helpers: solid fill with src masking
//
// ***************************************************************************************

// Definition for our masked-solid routines.
typedef void (*solidMaskedFn)(uint8_t *, const uint8_t *, size_t, uint8_t, uint8_t);

// Copy src to target, whole bytes.
static void solid_masked_ByteToByte(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent, uint8_t solid) {
	while (n>0) {
		if (*src != transparent) {
			*tgt = solid;
		}
		++tgt;
		++src;
		--n;
	}
}

//  Copy src to target low nibble (target high nibble is unchanged).
static void solid_masked_ByteToLow(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent, uint8_t solid) {
	solid &= 0x0F;
	while(n > 0) {
		if (*src != transparent) {
			*tgt = (*tgt & 0xF0) | solid;
		}
		++tgt;
		++src;
		--n;
	}
}

//  Copy src to target high nibble (target low nibble is unchanged).
static void solid_masked_ByteToHigh(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent, uint8_t solid) {
	solid <<= 4;
	while(n > 0) {
		if (*src != transparent) {
			*tgt = (*tgt & 0x0F) | solid;
		}
		++tgt;
		++src;
		--n;
	}
}

// Expand src nibbles into consecutive bytes in target (high nibbles of target will be zeroed).
static void solid_masked_PairToByte(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent, uint8_t solid) {
	solid &= 0x0F;
	n = n / 2;	// 2 nibbles per byte.
	while(n > 0) {
		if ((*src >> 4) != transparent) {
			*tgt = solid;
		}
		++tgt;
		if ((*src & 0x0F) != transparent) {
			*tgt = solid;
		}
		++tgt;
		++src;
		--n;
	}
}

// Expand src nibbles into consecutive low nibbles in target (leaving target high nibbles unchanged).
static void solid_masked_PairToLow(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent, uint8_t solid) {
	solid &= 0x0F;
	n = n / 2;	// 2 nibbles per byte.
	while(n > 0) {
		if ((*src >> 4) != transparent) {
			*tgt = (*tgt & 0xF0) | solid;
		}
		++tgt;
		if ((*src & 0x0F) != transparent) {
			*tgt = (*tgt & 0xF0) | solid;
		}
		++tgt;
		++src;
		--n;
	}
}

// Expand src nibbles into consecutive high nibbles in target (leaving target low nibbles unchanged).
static void solid_masked_PairToHigh(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent, uint8_t solid) {
	solid <<= 4;
	n = n / 2;	// 2 nibbles per byte.
	while(n > 0) {
		uint8_t v = *src >> 4;
		if (v != transparent) {
			*tgt = (*tgt & 0x0F) | solid;
		}
		++tgt;
		v = *src & 0x0F;
		if (v != transparent) {
			*tgt = (*tgt & 0x0F) | solid;
		}
		++tgt;
		++src;
		--n;
	}
}


// Expand src bits into consecutive bytes in target.
static void solid_masked_BitsToByte(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent, uint8_t solid) {
	n = n / 8;	// 8 bits per byte.
	while(n > 0) {
		for (int i = 7; i >= 0; --i) {
			uint8_t v = (*src >> i) & 0x01;
			if (v != transparent) {
				*tgt = solid;
			}
			++tgt;
		}
		++src;
		--n;
	}
}


// Expand src bits into consecutive low nibbles in target (leaving target high nibbles unchanged).
static void solid_masked_BitsToLow(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent, uint8_t solid) {
	solid &= 0x0F;
	n = n / 8;	// 8 bits per byte.
	while(n > 0) {
		for (int i = 7; i >= 0; --i) {
			uint8_t v = (*src >> i) & 0x01;
			if (v != transparent) {
				*tgt = (*tgt & 0xF0) | solid;
			}
			++tgt;
		}
		++src;
		--n;
	}
}

// Expand src bits into consecutive high nibbles in target (leaving target low nibbles unchanged).
static void solid_masked_BitsToHigh(uint8_t *tgt, const uint8_t *src, size_t n, uint8_t transparent, uint8_t solid) {
	solid <<= 4;
	n = n / 8;	// 8 bits per byte.
	while(n > 0) {
		for (int i = 7; i >= 0; --i) {
			uint8_t v = (*src >> i) & 0x01;
			if (v != transparent) {
				*tgt = (*tgt & 0x0F) | solid;
			}
			++tgt;
		}
		++src;
		--n;
	}
}

static solidMaskedFn pickSolidMaskedFn(uint8_t srcFormat, uint8_t tgtFormat)
{
	switch (srcFormat) {
		case BLTFMT_BYTE:
			switch (tgtFormat) {
				case BLTFMT_BYTE: return solid_masked_ByteToByte;
				case BLTFMT_HIGH: return solid_masked_ByteToHigh;
				case BLTFMT_LOW: return solid_masked_ByteToLow;
				default: return nullptr;
			}
		case BLTFMT_PAIR:
			switch (tgtFormat) {
				case BLTFMT_BYTE: return solid_masked_PairToByte;
				case BLTFMT_HIGH: return solid_masked_PairToHigh;
				case BLTFMT_LOW: return solid_masked_PairToLow;
				default: return nullptr;
			}
		case BLTFMT_BITS:
			switch (tgtFormat) {
				case BLTFMT_BYTE: return solid_masked_BitsToByte;
				case BLTFMT_HIGH: return solid_masked_BitsToHigh;
				case BLTFMT_LOW: return solid_masked_BitsToLow;
				default: return nullptr;
			}
		default: return nullptr;
	}
}



// ***************************************************************************************
//
//								More Complex Blitter Copy
//
// ***************************************************************************************

//		T-61 : only the FIRST address of each area was checked. width is 16 bit, so one line
//		could copy 64 Ko ; and height (up to 255) times stride (up to 65535) walked as far as
//		16 Mo from the start. Every one of those fields is read from a structure the 6502
//		program writes itself, so this was an unbounded write into whatever follows the area.
//		BLTSimpleCopy checks both ends — this one never did.
//		The end of each area, so a line can be rejected before it is copied.
static const uint8_t *_BLTAreaEnd(uint8_t page) {
	switch(page) {
		case 0x00: return cpuMemory + MEMORY_SIZE;
		case 0x80:
		case 0x81: return graphicsMemory + MAXGRAPHICSMEMORY;
		case 0x90: return gfxObjectMemory + GFX_MEMORY_SIZE;
	}
	if (page >= BANK_PAGE && page < BANK_PAGE + BANK_COUNT) return (const uint8_t *)BNKStorage(page - BANK_PAGE) + BANK_SIZE;
	return NULL;
}

//		A line fits when it starts inside the area and ends inside it too.
static bool _BLTLineFits(const uint8_t *p,const uint8_t *end,uint16_t width) {
	return p != NULL && end != NULL && p < end && (size_t)(end - p) >= width;
}

//		T-97 (reported by Neo6502POP) : width counts VALUES (pixels), not bytes, and the line
//		copies round it down to whole source bytes. The check used width for both areas, so a
//		1 bit (or 4 bit) image close to the end of an area — a bank, say — was refused while it
//		fitted. These are the bytes the copies actually read and write.
static uint16_t _BLTSourceBytes(uint8_t format,uint16_t width) {
	if (format == BLTFMT_BITS) return width / 8;
	if (format == BLTFMT_PAIR) return width / 2;
	return width;
}

static uint16_t _BLTTargetBytes(uint8_t srcFormat,uint16_t width) {  			// Targets are one byte per value
	if (srcFormat == BLTFMT_BITS) return (width / 8) * 8;
	if (srcFormat == BLTFMT_PAIR) return (width / 2) * 2;
	return width;
}

//		T-90 (need n° 1 of Neo6502POP) : targets of 2 pixels per byte (BLTFMT_PACKED, _ODD), e.g. the 16 colour
//		pages of mode 2. One generic line for every source format and action rather than nine more helpers : it
//		reads the same values as the other copies (whole source bytes only), and writes value n into nibble
//		first + n of the line, where first is 0 (high nibble of the first byte) or 1 (its low nibble), so an
//		odd x needs no shifting by the program. Values keep their low 4 bits ; a nibble not written is kept.
static uint16_t _BLTPackedBytes(uint8_t tgtFormat,uint16_t values) {
	uint16_t first = (tgtFormat == BLTFMT_PACKED_ODD) ? 1 : 0;
	return (uint16_t)((first + values + 1) / 2);
}

static void _BLTPackedLine(uint8_t action,const struct BlitterArea *source,uint8_t tgtFormat,uint8_t *tgt,const uint8_t *src,uint16_t values) {
	uint32_t nib = (tgtFormat == BLTFMT_PACKED_ODD) ? 1 : 0;
	for (uint16_t i = 0;i < values;i++,nib++) {
		uint8_t v;
		switch (source->format) {
			case BLTFMT_BITS: v = (src[i >> 3] >> (7 - (i & 7))) & 1;break;
			case BLTFMT_PAIR: v = (i & 1) ? (src[i >> 1] & 0x0F) : (src[i >> 1] >> 4);break;
			default:          v = src[i];break;
		}
		if (action != BLTACT_COPY && v == source->transparent) continue;  		// copymasked / solidmasked : skipped
		if (action == BLTACT_SOLID) v = source->solid;
		v &= 0x0F;
		uint8_t *t = tgt + (nib >> 1);
		*t = (nib & 1) ? ((*t & 0xF0) | v) : ((*t & 0x0F) | (uint8_t)(v << 4));
	}
}

static uint8_t _BLTPackedCopy(uint8_t action,const struct BlitterArea *source,const struct BlitterArea *target,
							  const uint8_t *srcEnd,const uint8_t *tgtEnd) {
	if (source->format != BLTFMT_BYTE && source->format != BLTFMT_PAIR && source->format != BLTFMT_BITS) return 1;
	if (action != BLTACT_COPY && action != BLTACT_MASK && action != BLTACT_SOLID) return 1;
	uint16_t values = _BLTTargetBytes(source->format,source->width);  			// Whole source bytes, as the other copies
	uint8_t *src = BLTGetRealAddress(source->page, source->address);
	uint8_t *tgt = BLTGetRealAddress(target->page, target->address);
	if (src == NULL || tgt == NULL) return 1;
	for (uint8_t l = source->height; l > 0; --l) {
		if (!_BLTLineFits(src,srcEnd,_BLTSourceBytes(source->format,source->width)) ||   // T-61 / T-97
			!_BLTLineFits(tgt,tgtEnd,_BLTPackedBytes(target->format,values))) return 1;
		_BLTPackedLine(action,source,target->format,tgt,src,values);
		src += source->stride;
		tgt += target->stride;
	}
	return 0;
}

//		T-115 (Neo6502AigleDor) : translated copy. Each source value v (byte, nibble or bit, as the other
//		copies, whole source bytes only) is written as table[v] (256 bytes, any readable page) in the
//		target format : whole byte, high or low nibble (the other one kept), or packed (T-90). With
//		BLTACT_TRANSLATE_MASK a value equal to source->transparent leaves its target untouched.
//		One generic line, like _BLTPackedLine. Done on the 6502, a background of 240 bytes a line
//		cost ~4 300 cycles a line (Neo6502AigleDor).
static void _BLTTranslateLine(bool masked,const struct BlitterArea *source,uint8_t tgtFormat,uint8_t *tgt,
							  const uint8_t *src,uint16_t values,const uint8_t *table) {
	uint32_t nib = (tgtFormat == BLTFMT_PACKED_ODD) ? 1 : 0;
	for (uint16_t i = 0;i < values;i++,nib++) {
		uint8_t v;
		switch (source->format) {
			case BLTFMT_BITS: v = (src[i >> 3] >> (7 - (i & 7))) & 1;break;
			case BLTFMT_PAIR: v = (i & 1) ? (src[i >> 1] & 0x0F) : (src[i >> 1] >> 4);break;
			default:          v = src[i];break;
		}
		if (masked && v == source->transparent) continue;
		uint8_t w = table[v];
		switch (tgtFormat) {
			case BLTFMT_BYTE: tgt[i] = w;break;
			case BLTFMT_HIGH: tgt[i] = (tgt[i] & 0x0F) | (uint8_t)(w << 4);break;
			case BLTFMT_LOW:  tgt[i] = (tgt[i] & 0xF0) | (w & 0x0F);break;
			default: {
				uint8_t *t = tgt + (nib >> 1);
				*t = (nib & 1) ? ((*t & 0xF0) | (w & 0x0F)) : ((*t & 0x0F) | (uint8_t)(w << 4));
			}
		}
	}
}

static uint8_t _BLTTranslateCopy(uint8_t action,const struct BlitterArea *source,const struct BlitterArea *target,
								 uint16_t aTable,uint8_t tablePage) {
	if (source->format != BLTFMT_BYTE && source->format != BLTFMT_PAIR && source->format != BLTFMT_BITS) return 1;
	uint8_t tf = target->format;
	if (tf != BLTFMT_BYTE && tf != BLTFMT_HIGH && tf != BLTFMT_LOW && tf != BLTFMT_PACKED && tf != BLTFMT_PACKED_ODD) return 1;
	const uint8_t *table = BLTGetRealAddress(tablePage,aTable);
	if (!_BLTLineFits(table,_BLTAreaEnd(tablePage),256)) return 1;  				// The whole table must be readable
	const uint8_t *srcEnd = _BLTAreaEnd(source->page);
	const uint8_t *tgtEnd = _BLTAreaEnd(target->page);
	uint16_t values = _BLTTargetBytes(source->format,source->width);  			// Whole source bytes, as the other copies
	bool packed = (tf == BLTFMT_PACKED || tf == BLTFMT_PACKED_ODD);
	uint8_t *src = BLTGetRealAddress(source->page, source->address);
	uint8_t *tgt = BLTGetRealAddress(target->page, target->address);
	if (src == NULL || tgt == NULL) return 1;
	for (uint8_t l = source->height; l > 0; --l) {
		if (!_BLTLineFits(src,srcEnd,_BLTSourceBytes(source->format,source->width)) ||
			!_BLTLineFits(tgt,tgtEnd,packed ? _BLTPackedBytes(tf,values) : values)) return 1;
		_BLTTranslateLine(action == BLTACT_TRANSLATE_MASK,source,tf,tgt,src,values,table);
		src += source->stride;
		tgt += target->stride;
	}
	return 0;
}

static uint8_t internalBLTComplexCopy(uint8_t action, const struct BlitterArea *source, const struct BlitterArea *target) {
	if (target->format == BLTFMT_PACKED || target->format == BLTFMT_PACKED_ODD)  	// T-90
		return _BLTPackedCopy(action,source,target,_BLTAreaEnd(source->page),_BLTAreaEnd(target->page));
	const uint8_t *srcEnd = _BLTAreaEnd(source->page);  							// T-61
	const uint8_t *tgtEnd = _BLTAreaEnd(target->page);

	switch (action) {
		case BLTACT_COPY:
			{
				copyFn copy = pickCopyFn(source->format, target->format);
				if (!copy) {
					return 1;	// Unsupported combination
				}

				uint8_t *src = BLTGetRealAddress(source->page, source->address);
				uint8_t *tgt = BLTGetRealAddress(target->page, target->address);
				if (src == NULL || tgt == NULL) return 1;
				for (uint8_t l = source->height; l > 0; --l) {
					if (!_BLTLineFits(src,srcEnd,_BLTSourceBytes(source->format,source->width)) ||  	// T-61 / T-97
						!_BLTLineFits(tgt,tgtEnd,_BLTTargetBytes(source->format,source->width))) return 1;
					(*copy)(tgt, src, source->width);
					src += source->stride;
					tgt += target->stride;
				}
			}
			break;
		case BLTACT_MASK:
			{
				copyMaskedFn copyMasked = pickCopyMaskedFn(source->format, target->format);
				if (!copyMasked) {
					return 1;	// Unsupported combination
				}

				uint8_t *src = BLTGetRealAddress(source->page, source->address);
				uint8_t *tgt = BLTGetRealAddress(target->page, target->address);
				if (src == NULL || tgt == NULL) return 1;
				for (uint8_t l = source->height; l > 0; --l) {
					if (!_BLTLineFits(src,srcEnd,_BLTSourceBytes(source->format,source->width)) ||  	// T-61 / T-97
						!_BLTLineFits(tgt,tgtEnd,_BLTTargetBytes(source->format,source->width))) return 1;
					(*copyMasked)(tgt, src, source->width, source->transparent);
					src += source->stride;
					tgt += target->stride;
				}
			}
			break;
		case BLTACT_SOLID:
			{
				solidMaskedFn solidMasked = pickSolidMaskedFn(source->format, target->format);
				if (!solidMasked) {
					return 1;	// Unsupported combination
				}

				uint8_t *src = BLTGetRealAddress(source->page, source->address);
				uint8_t *tgt = BLTGetRealAddress(target->page, target->address);
				if (src == NULL || tgt == NULL) return 1;
				for (uint8_t l = source->height; l > 0; --l) {
					if (!_BLTLineFits(src,srcEnd,_BLTSourceBytes(source->format,source->width)) ||  	// T-61 / T-97
						!_BLTLineFits(tgt,tgtEnd,_BLTTargetBytes(source->format,source->width))) return 1;
					(*solidMasked)(tgt, src, source->width, source->transparent, source->solid);
					src += source->stride;
					tgt += target->stride;
				}
			}
			break;
	}
	return 0;
}

void BLTLoadArea(uint16_t addr,struct BlitterArea *b) { _BLTLoadBlitterAreaObject(addr,b); }  		// T-12
uint8_t BLTCopyArea(uint8_t action,const struct BlitterArea *source,const struct BlitterArea *target) {
	return internalBLTComplexCopy(action,source,target);
}

uint8_t BLTComplexCopy(uint8_t action,uint16_t aSource,uint16_t aTarget,uint16_t aTable,uint8_t tablePage) {
	struct BlitterArea source, target;
	_BLTLoadBlitterAreaObject(aSource,&source);
	_BLTLoadBlitterAreaObject(aTarget,&target);
	if (target.page >= BANK_PAGE) return 1;  										// T-17 : banks are read only (flash)
	if (action == BLTACT_TRANSLATE || action == BLTACT_TRANSLATE_MASK)  			// T-115
		return _BLTTranslateCopy(action,&source,&target,aTable,tablePage);
	return internalBLTComplexCopy(action, &source, &target);
}

// ***************************************************************************************
//
//						Image-oriented blit function with clipping.
//
// ***************************************************************************************

// Note: no handy width and height defines available, so we'll define them here.
// But they should probably be somewhere else.
#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240

// Clip rectangle. Maybe it'd make sense to let user set custom clipping area, but for now
// we'll just hardcode them here.
#define LCLIP 0
#define RCLIP FRAME_WIDTH
#define TCLIP 0
#define BCLIP FRAME_HEIGHT

uint8_t BLTImage(uint8_t action, uint16_t sourceArea, int16_t x, int16_t y, uint8_t destFmt)
{
	//		T-90 : the target address below assumes mode 0 (FRAME_WIDTH bytes a line, one byte a
	//		pixel). In the packed modes (1 and 2) it would write at the wrong place : refused.
	if (GFXIsPackedMode()) return 1;
	struct BlitterArea src;
	_BLTLoadBlitterAreaObject(sourceArea, &src);

	// Clip against left.
	int16_t w = src.width;
	if (x < LCLIP) {
		// Left-clipping is a little fiddly for sub-byte source formats.
		// We need to round up to next byte boundary (maybe leaving
		// up to 7 pixels missing on the left).
		int16_t adj = (LCLIP - x);
		switch (src.format) {
			case BLTFMT_BYTE:
				src.address += adj;
				w -= adj;
				x += adj;
				break;
			case BLTFMT_PAIR:
				src.address += (adj + 1) >> 1;
				w -= (adj + 1) & ~1;
				x += (adj + 1) & ~1;
				break;
			case BLTFMT_BITS:
				src.address += (adj + 7) >> 3;
				w -= (adj + 7) & ~7;
				x += (adj + 7) & ~7;
				break;
			default: return 1;  // bad format.
		}
		if (w <= 0) {
			return 0;   // Totally off left.
		}
	}

	// Clip against right.
	if ((x + w) > RCLIP) {
		// Don't worry about byte boundaries here - the blit
		// will just skip any sub-byte data at the end of each line.
		w -= ((x+w) - RCLIP);
		if (w <= 0) {
			return 0;   // Totally off right.
		}
	}
	src.width = w;

	int16_t h = src.height;
	// Clip against top.
	if (y < TCLIP) {
		int16_t adj = TCLIP-y;
		h -= adj;
		if (h <= 0) {
			return 0;   // Totally off top.
		}
		y += adj;
		// Stride is in bytes.
		src.address += adj * src.stride;
	}

	// Clip against bottom.
	if ((y + h) > BCLIP) {
		h -= ((y + h) - BCLIP);
		if (h <= 0) {
			return 0;   // Totally off bottom.
		}
	}
	src.height = h;

	uint32_t offset = (y * FRAME_WIDTH) + x;
	struct BlitterArea target = {
		.address = (uint16_t)(offset & 0xFFFF),
		.page = (uint8_t)(0x80 + (offset >> 16)),
		.padding = 0,
		.stride = FRAME_WIDTH,
		.format = destFmt,
	};

	return internalBLTComplexCopy(action, &src, &target);
}


// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
