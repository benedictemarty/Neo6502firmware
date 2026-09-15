#!/usr/bin/env python3
# ***************************************************************************************
#
#      Name :      mkimage.py
#      Author :    bmarty <bmarty@mailo.com>
#      Purpose :   Assemble one UF2 for the Neo6502 multi-boot (F-80) : the selector at
#                  flash 0, images in their slots (each already linked for its slot with
#                  memmap_slot_N.ld), plus the slot directory (names) read by API 1,15.
#
#      mkimage.py -o neo6502-multi.uf2 --selector build/neoboot.uf2 \
#                 --slot 0 firmware_slot0.uf2 --slot 1 bbc_slot1.uf2
#
#      A UF2 block carries its own target address : the tool only checks that every
#      block of an image falls inside its slot, and re-numbers the blocks. Names are
#      not stored anywhere : the firmware reads them from each image's binary_info.
#
# ***************************************************************************************
import argparse, struct, sys

UF2_MAGIC0, UF2_MAGIC1, UF2_MAGIC_END = 0x0A324655, 0x9E5D5157, 0x0AB16F30
RP2040_FAMILY = 0xE48BFF56
XIP_BASE, SLOT0, SLOT_SIZE, SLOTS, SELECTOR_SIZE = 0x10000000, 0x10000, 0x78000, 4, 0x10000

def read_uf2(path):
    data = open(path, "rb").read()
    blocks = []
    for i in range(0, len(data), 512):
        b = data[i:i + 512]
        m0, m1, flags, addr, size, _, _, fam = struct.unpack_from("<8I", b, 0)
        if m0 != UF2_MAGIC0 or m1 != UF2_MAGIC1: sys.exit("%s : bloc %d invalide" % (path, i // 512))
        blocks.append((addr, size, b[32:32 + size]))
    return blocks

def make_block(addr, payload, idx, total):
    payload = payload + b"\0" * (256 - len(payload))
    return struct.pack("<8I", UF2_MAGIC0, UF2_MAGIC1, 0x2000, addr, 256, idx, total, RP2040_FAMILY) + payload + b"\0" * (476 - 256) + struct.pack("<I", UF2_MAGIC_END)

ap = argparse.ArgumentParser()
ap.add_argument("-o", required=True)
ap.add_argument("--selector", required=True, help="neoboot.uf2 (flash 0x10000000)")
ap.add_argument("--slot", nargs=2, action="append", metavar=("N", "UF2"), default=[])
a = ap.parse_args()

out = []  # (addr, payload)
for addr, size, payload in read_uf2(a.selector):
    if not (XIP_BASE <= addr < XIP_BASE + SELECTOR_SIZE): sys.exit("sélecteur : bloc hors zone 0x%08X" % addr)
    out.append((addr, payload))

def program_name(blocks, base):
    """Nom binary_info de l'image (comme le fera le firmware), pour information."""
    mem = {}
    for addr, size, payload in blocks: mem[addr] = payload
    def rd32(a):
        b = mem.get(a & ~0xFF)
        return struct.unpack_from("<I", b, a & 0xFF)[0] if b and (a & 0xFF) <= 252 else None
    for i in range(0, 59):
        a = base + 0x100 + i * 4
        if rd32(a) == 0x7188EBF2 and rd32(a + 16) == 0xE71AA390:
            start, end = rd32(a + 4), rd32(a + 8)
            p = start
            while p < end:
                e = rd32(p); p += 4
                if e and rd32(e) == (0x5052 << 16 | 6) and rd32(e + 4) == 0x02031C86:
                    s = rd32(e + 8); out = b""
                    while True:
                        c = mem.get(s & ~0xFF)
                        if not c: return "?"
                        ch = c[s & 0xFF]
                        if ch == 0: return out.decode("latin-1")
                        out += bytes([ch]); s += 1
    return "(sans binary_info)"

for n, path in a.slot:
    n = int(n)
    if not 0 <= n < SLOTS: sys.exit("slot %d invalide" % n)
    lo, hi = XIP_BASE + SLOT0 + n * SLOT_SIZE, XIP_BASE + SLOT0 + (n + 1) * SLOT_SIZE
    blocks = read_uf2(path)
    for addr, size, payload in blocks:
        if not (lo <= addr < hi): sys.exit("%s : bloc 0x%08X hors du slot %d (0x%08X-0x%08X) — image liée avec memmap_slot_%d.ld ?" % (path, addr, n, lo, hi - 1, n))
        out.append((addr, payload))
    print("slot %d : %-20s %6d Ko  %s" % (n, program_name(blocks, lo), len(blocks) * 256 // 1024, path))
with open(a.o, "wb") as f:
    for i, (addr, payload) in enumerate(out):
        f.write(make_block(addr, payload, i, len(out)))
print("%s : %d blocs, %d Ko" % (a.o, len(out), len(out) * 256 // 1024))
