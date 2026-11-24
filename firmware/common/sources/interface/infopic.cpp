// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      infopic.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      24th November 2026
//      Purpose :   Group 41, Infocom v6 pictures (T-164, see infopic.h).
//
//      Format (from the description given by the Neo6502Ozmoo project, reference pix2gif.c
//      of ztools) : 16 byte header (words low byte first : entry count at 4, entry size at
//      8, 12 or 14), directory of entries (number, width, height, flags : 16 bit LE ; data
//      address 24 bit big endian ; palette address 24 bit big endian if 14 byte entries).
//      Pixels : LZW, codes of 9 to 12 bits read low bit first, 256 clear, 257 end, the code
//      width grows when the next free code is 2^width-1, before the entry is added ; no
//      entry is added for the first code after a clear. A decoded value is a colour 0-15 :
//      a table entry is a 12 bit prefix and a 4 bit colour, so the whole table is 7 676 bytes.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

#define IP_FIRST    258                                                         // First code that is a table entry
#define IP_TABLE    4096
#define IP_ENTRIES  (IP_TABLE - IP_FIRST)

struct IPEntry { uint16_t width,height,flags;uint32_t data,palette; };

static uint8_t _IPFind(uint8_t channel,uint16_t number,struct IPEntry *e) {
    if (channel >= FIO_NUM_FILES) return IP_ERR_PARAM;
    uint8_t h[16];
    uint16_t n = 16;
    if (FISSeekFileHandle(channel,0) != 0 || FISReadFileHandleBuffer(channel,h,&n) != 0 || n != 16) return IP_ERR_IO;
    uint16_t count = h[4] | (h[5] << 8);
    uint8_t size = h[8];
    if ((size != 12 && size != 14) || count == 0) return IP_ERR_PARAM;
    uint8_t buf[14 * 16];
    uint16_t done = 0;
    while (done < count) {                                                      // Sequential reads, 16 entries at a time
        uint16_t chunk = count - done;
        if (chunk > 16) chunk = 16;
        n = chunk * size;
        if (FISReadFileHandleBuffer(channel,buf,&n) != 0 || n != chunk * size) return IP_ERR_IO;
        for (uint16_t i = 0;i < chunk;i++) {
            const uint8_t *p = buf + i * size;
            if ((uint16_t)(p[0] | (p[1] << 8)) != number) continue;
            e->width = p[2] | (p[3] << 8);
            e->height = p[4] | (p[5] << 8);
            e->flags = p[6] | (p[7] << 8);
            e->data = ((uint32_t)p[8] << 16) | (p[9] << 8) | p[10];
            e->palette = (size == 14) ? (((uint32_t)p[11] << 16) | (p[12] << 8) | p[13]) : 0;
            return IP_ERR_OK;
        }
        done += chunk;
    }
    return IP_ERR_NOTFOUND;
}

uint8_t IPInfo(uint8_t channel,uint16_t number,uint16_t *width,uint16_t *height,uint16_t *flags,uint8_t *what) {
    struct IPEntry e;
    uint8_t err = _IPFind(channel,number,&e);
    if (err != IP_ERR_OK) return err;
    *width = e.width;*height = e.height;*flags = e.flags;
    *what = (e.data != 0 ? 1 : 0) | (e.palette != 0 ? 2 : 0);
    return IP_ERR_OK;
}

// ***************************************************************************************
//
//		Bit source : the file read sequentially in 256 byte pieces.
//
// ***************************************************************************************

struct IPBits { uint8_t channel;uint8_t buf[256];uint16_t len,pos;uint32_t acc;uint8_t nbits; };

static int _IPCode(struct IPBits *s,int width) {                                // -1 at the end of the file or on a storage error
    while (s->nbits < width) {
        if (s->pos >= s->len) {
            uint16_t n = sizeof(s->buf);
            if (FISReadFileHandleBuffer(s->channel,s->buf,&n) != 0 || n == 0) return -1;
            s->len = n;s->pos = 0;
        }
        s->acc |= (uint32_t)s->buf[s->pos++] << s->nbits;
        s->nbits += 8;
    }
    int c = s->acc & ((1u << width) - 1);
    s->acc >>= width;s->nbits -= width;
    return c;
}

// ***************************************************************************************
//
//		Palettes
//
// ***************************************************************************************

static const uint8_t egaColours[16][3] = {
    {0x00,0x00,0x00},{0x00,0x00,0xAA},{0x00,0xAA,0x00},{0x00,0xAA,0xAA},{0xAA,0x00,0x00},{0xAA,0x00,0xAA},{0xAA,0x55,0x00},{0xAA,0xAA,0xAA},
    {0x55,0x55,0x55},{0x55,0x55,0xFF},{0x55,0xFF,0x55},{0x55,0xFF,0xFF},{0xFF,0x55,0x55},{0xFF,0x55,0xFF},{0xFF,0xFF,0x55},{0xFF,0xFF,0xFF}
};

static uint8_t _IPLoadPalette(uint8_t channel,uint32_t address,uint8_t base) {
    uint8_t n = 0,rgb[14 * 3];
    uint16_t cnt = 1;
    if (FISSeekFileHandle(channel,address) != 0 || FISReadFileHandleBuffer(channel,&n,&cnt) != 0 || cnt != 1) return IP_ERR_IO;
    if (n > 14) n = 14;                                                         // « buggy Arthur pictures »
    cnt = n * 3;
    if (cnt != 0 && (FISReadFileHandleBuffer(channel,rgb,&cnt) != 0 || cnt != n * 3)) return IP_ERR_IO;
    for (uint8_t i = 0;i < n;i++) if ((unsigned)base + i + 2 < 256) GFXSetPalette(base + i + 2,rgb[i * 3],rgb[i * 3 + 1],rgb[i * 3 + 2]);
    return IP_ERR_OK;
}

// ***************************************************************************************
//
//		Drawing : the table lives in the heap for the call only.
//
// ***************************************************************************************

struct IPDrawing { int x,y;uint16_t width;uint8_t options,transparent,base;bool useTransparent; };

static void _IPPut(const struct IPDrawing *d,int px,int py,uint8_t c) {
    if (d->useTransparent && c == d->transparent) return;
    if (d->options & IP_OPT_HALF) { if (px & 1) return;px >>= 1; }
    int sx = d->x + px,sy = d->y + py;
    if (sx < 0 || sy < 0 || sx >= (int)gMode.xGSize || sy >= (int)gMode.yGSize) return;
    GFXWritePixelRaw(sx,sy,(uint8_t)(c + d->base));
}

uint8_t IPDraw(uint8_t channel,uint16_t number,int16_t x,int16_t y,uint8_t options,uint8_t base) {
    if (!(options & IP_OPT_BASE)) base = 0;
    if (gMode.bitsPerPixel != 4 && gMode.bitsPerPixel != 8) return IP_ERR_PARAM;
    struct IPEntry e;
    uint8_t err = _IPFind(channel,number,&e);
    if (err != IP_ERR_OK) return err;
    if (e.data == 0 || e.width == 0 || e.height == 0) return IP_ERR_NODATA;
    if (options & IP_OPT_EGA) for (uint8_t i = 0;i < 16;i++) if ((unsigned)base + i < 256) GFXSetPalette(base + i,egaColours[i][0],egaColours[i][1],egaColours[i][2]);
    if ((options & IP_OPT_PALETTE) && e.palette != 0) {
        err = _IPLoadPalette(channel,e.palette,base);
        if (err != IP_ERR_OK) return err;
    }
    uint16_t *table = (uint16_t *)malloc(IP_ENTRIES * sizeof(uint16_t));
    if (table == NULL) return IP_ERR_MEMORY;
    struct IPBits *src = (struct IPBits *)malloc(sizeof(struct IPBits));
    if (src == NULL) { free(table);return IP_ERR_MEMORY; }
    err = IP_ERR_OK;
    if (FISSeekFileHandle(channel,e.data) != 0) err = IP_ERR_IO;
    src->channel = channel;src->len = src->pos = 0;src->acc = 0;src->nbits = 0;

    struct IPDrawing d = { x,y,e.width,options,(uint8_t)(e.flags >> 12),base,(options & IP_OPT_TRANSP) && (e.flags & 1) };
    uint32_t total = (uint32_t)e.width * e.height,index = 0;
    int width = 9;
    unsigned next = IP_FIRST;
    int old = 0;
    uint8_t firstOld = 0;
    while (err == IP_ERR_OK && index < total) {
        int code = _IPCode(src,width);
        if (code < 0) { err = IP_ERR_DATA;break; }
        if (next == (1u << width) - 1 && width < 12) width++;                   // Before the entry is added
        if (code == 257) break;
        unsigned length;
        uint8_t root;
        if (code == 256) {                                                      // Clear : the next code is a single colour
            width = 9;next = IP_FIRST;
            code = _IPCode(src,9);
            if (code < 0) { err = IP_ERR_DATA;break; }
            if (code == 257) break;
            if (code >= 256) { err = IP_ERR_DATA;break; }
            length = 1;root = code & 15;
        } else {
            if ((unsigned)code > next) { err = IP_ERR_DATA;break; }
            bool repeat = ((unsigned)code == next);                             // KwKwK : the string of the entry being added
            if (repeat) {
                if (next >= IP_TABLE) { err = IP_ERR_DATA;break; }
                table[next - IP_FIRST] = (uint16_t)((old << 4) | firstOld);next++;
            }
            length = 1;
            int c = code;
            while (c >= IP_FIRST) { c = table[c - IP_FIRST] >> 4;length++; }
            root = c & 15;
            if (!repeat && next < IP_TABLE) { table[next - IP_FIRST] = (uint16_t)((old << 4) | root);next++; }
        }
        firstOld = root;old = code;
        uint32_t row = index / e.width;
        unsigned col = index - row * e.width;                                   // The string is written backwards, last colour first
        int c = code;
        for (unsigned k = length;k > 0;k--) {
            uint8_t colour;
            if (c >= IP_FIRST) { uint16_t t = table[c - IP_FIRST];colour = t & 15;c = t >> 4; }
            else colour = c & 15;
            if (index + k - 1 >= total) continue;
            unsigned pos = col + k - 1;
            if (pos < e.width) _IPPut(&d,pos,row,colour);
            else _IPPut(&d,pos % e.width,row + pos / e.width,colour);
        }
        index += length;
    }
    free(src);free(table);
    return err;
}
