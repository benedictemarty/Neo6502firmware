// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      tilemap8.cpp
//      Authors :   bmarty
//      Date :      7th October 2026
//      Reviewed :  No
//      Purpose :   T-118 : tilemap of 8 x 8 tiles with an attribute per cell (5,43)
//
// ***************************************************************************************
// ***************************************************************************************
//
//      5,8 only knows 16 x 16 tiles of 4 bits, one byte per cell, no colour per cell. Arcade
//      boards and 8 bit consoles (Bagman, Pac-Man, MSX, SMS...) have 8 x 8 tiles and a colour
//      attribute per cell. Here a cell is 16 bits : tile 0-1023, palette bank 0-15, x and y flip.
//      Everything is described by a block in 6502 RAM, no state is kept between two calls.
//      Drawn pixel by pixel : simple, and it runs as an API command, never on the display core.
//

#include "common.h"

#define TM8_DESC_SIZE   (26)                                                    // Bytes of the descriptor
#define TM8_LOWNIBBLE   (0x01)                                                  // Flag : write the low nibble only (sprite layer kept)
#define TM8_ZEROCLEAR   (0x02)                                                  // Flag : colour index 0 leaves the pixel as it is

static uint16_t _TM8Word(const uint8_t *p) {
    return p[0] | (p[1] << 8);
}

// ***************************************************************************************
//
//      Draw : descriptor at 'desc' in 6502 RAM. 0 if drawn, 1 if the descriptor is refused
//
// ***************************************************************************************

uint8_t TM8Draw(uint16_t desc) {
    if ((uint32_t)desc + TM8_DESC_SIZE > DEFAULT_PORT) return 1;                // Below the API page
    const uint8_t *d = cpuMemory + desc;
    uint8_t mapWidth = d[3],mapHeight = d[4],bpp = d[8],flags = d[9];
    if (mapWidth == 0 || mapHeight == 0) return 1;
    if (bpp != 1 && bpp != 2 && bpp != 4 && bpp != 8) return 1;
    if (flags & ~(TM8_LOWNIBBLE | TM8_ZEROCLEAR)) return 1;

    const uint8_t *map = BLTGetRealAddress(d[2],_TM8Word(d));                   // Map : 2 bytes a cell, row by row
    const uint8_t *mapEnd = BLTGetAreaEnd(d[2]);
    if (map == NULL || mapEnd == NULL || (uint32_t)(mapEnd - map) < 2u * mapWidth * mapHeight) return 1;

    const uint8_t *tiles = BLTGetRealAddress(d[7],_TM8Word(d+5));               // Tiles : 8 rows of bpp bytes each
    const uint8_t *tilesEnd = BLTGetAreaEnd(d[7]);
    if (tiles == NULL || tilesEnd == NULL) return 1;
    uint16_t tileBytes = 8 * bpp;
    uint32_t tileCount = (uint32_t)(tilesEnd - tiles) / tileBytes;              // Tiles past the area are not drawn
    if (tileCount > 1024) tileCount = 1024;

    const uint8_t *palette = NULL;                                              // Page $FF : no palette, colour = bank:index
    if (d[12] != 0xFF) {
        palette = BLTGetRealAddress(d[12],_TM8Word(d+10));
        const uint8_t *palEnd = BLTGetAreaEnd(d[12]);
        uint16_t palBytes = (bpp == 8) ? 256 : (16 << bpp);                     // 16 banks of 2^bpp colours
        if (palette == NULL || palEnd == NULL || palEnd - palette < palBytes) return 1;
    }

    int scrollX = (int16_t)_TM8Word(d+14),scrollY = (int16_t)_TM8Word(d+16);    // Map pixel at the window's top left
    int x0 = (int16_t)_TM8Word(d+18),y0 = (int16_t)_TM8Word(d+20);
    int w = _TM8Word(d+22),h = _TM8Word(d+24);
    if (x0 < 0) { scrollX -= x0;w += x0;x0 = 0; }                               // Clip to the screen
    if (y0 < 0) { scrollY -= y0;h += y0;y0 = 0; }
    if (x0 + w > gMode.xGSize) w = gMode.xGSize - x0;
    if (y0 + h > gMode.yGSize) h = gMode.yGSize - y0;
    if (w <= 0 || h <= 0) return 0;

    bool packed = GFXIsPackedMode() != 0;
    uint8_t mask = (1 << bpp) - 1;
    int mapPixelsX = mapWidth * 8,mapPixelsY = mapHeight * 8;
    for (int yy = 0;yy < h;yy++) {
        int my = scrollY + yy;
        if (my < 0 || my >= mapPixelsY) continue;                               // Outside the map : left as it is
        const uint8_t *cellRow = map + 2 * (my >> 3) * mapWidth;
        uint8_t *line = gMode.graphicsMemory + (y0 + yy) * gMode.stride + x0;
        for (int xx = 0;xx < w;xx++) {
            int mx = scrollX + xx;
            if (mx < 0 || mx >= mapPixelsX) continue;
            uint16_t cell = _TM8Word(cellRow + 2 * (mx >> 3));
            uint16_t tile = cell & 0x3FF;
            if (tile >= tileCount) continue;
            int row = (cell & 0x8000) ? 7 - (my & 7) : (my & 7);
            int col = (cell & 0x4000) ? 7 - (mx & 7) : (mx & 7);
            int bit = col * bpp;                                                // Leftmost pixel in the high bits
            uint8_t index = (tiles[tile * tileBytes + row * bpp + (bit >> 3)] >> (8 - bpp - (bit & 7))) & mask;
            if (index == 0 && (flags & TM8_ZEROCLEAR)) continue;
            uint8_t n = (bpp == 8) ? index : (uint8_t)((((cell >> 10) & 15) << bpp) | index);
            uint8_t c = palette ? palette[n] : n;
            if (packed) {
                if (flags & TM8_LOWNIBBLE) c = (GFXReadPixelRaw(x0 + xx,y0 + yy) & 0xF0) | (c & 0x0F);
                GFXWritePixelRaw(x0 + xx,y0 + yy,c);
            } else {
                line[xx] = (flags & TM8_LOWNIBBLE) ? (line[xx] & 0xF0) | (c & 0x0F) : c;
            }
        }
    }
    return 0;
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//		07-10-26 	T-118 : first version (Neo6502Bagman, ADR 0010)
//
// ***************************************************************************************
