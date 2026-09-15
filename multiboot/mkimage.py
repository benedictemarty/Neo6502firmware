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
#                 --slot 0 firmware.uf2 "Neo6502" --slot 1 bbc.uf2 "BBC Micro"
#
#      A UF2 block carries its own target address : the tool only checks that every
#      block of an image falls inside its slot, and re-numbers the blocks.
#
# ***************************************************************************************
import argparse, struct, sys

UF2_MAGIC0, UF2_MAGIC1, UF2_MAGIC_END = 0x0A324655, 0x9E5D5157, 0x0AB16F30
RP2040_FAMILY = 0xE48BFF56
XIP_BASE, SLOT0, SLOT_SIZE, SLOTS, DIR_OFFSET, NAME_LEN, MAGIC = 0x10000000, 0x10000, 0x78000, 4, 0x0F000, 32, 0x4E454F00

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
ap.add_argument("--slot", nargs=3, action="append", metavar=("N", "UF2", "NAME"), default=[])
a = ap.parse_args()

out = []  # (addr, payload)
for addr, size, payload in read_uf2(a.selector):
    if not (XIP_BASE <= addr < XIP_BASE + DIR_OFFSET): sys.exit("sélecteur : bloc hors zone 0x%08X" % addr)
    out.append((addr, payload))
names = [""] * SLOTS
for n, path, name in a.slot:
    n = int(n)
    if not 0 <= n < SLOTS: sys.exit("slot %d invalide" % n)
    lo, hi = XIP_BASE + SLOT0 + n * SLOT_SIZE, XIP_BASE + SLOT0 + (n + 1) * SLOT_SIZE
    blocks = read_uf2(path)
    for addr, size, payload in blocks:
        if not (lo <= addr < hi): sys.exit("%s : bloc 0x%08X hors du slot %d (0x%08X-0x%08X) — image liée avec memmap_slot_%d.ld ?" % (path, addr, n, lo, hi - 1, n))
        out.append((addr, payload))
    names[n] = name[:NAME_LEN - 1]
    print("slot %d : %-20s %6d Ko  %s" % (n, name, len(blocks) * 256 // 1024, path))
directory = struct.pack("<I", MAGIC) + b"".join(nm.encode("latin-1").ljust(NAME_LEN, b"\0") for nm in names)
out.append((XIP_BASE + DIR_OFFSET, directory))
with open(a.o, "wb") as f:
    for i, (addr, payload) in enumerate(out):
        f.write(make_block(addr, payload, i, len(out)))
print("%s : %d blocs, %d Ko" % (a.o, len(out), len(out) * 256 // 1024))
