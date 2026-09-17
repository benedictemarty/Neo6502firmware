#!/usr/bin/env python3
# mkfont.py — génère une police proportionnelle NF1 pour QuickDraw (32,15 Set Font, fork Neo6502firmware)
# à partir d'une police TrueType (Pillow).
#
#   mkfont.py police.ttf taille sortie.nf1 [--first 32] [--count 96] [--spacing 1] [--max-width 16] [--threshold 96]
#
# Format NF1 : "NF", version 1, hauteur, premier caractère, nombre, espacement, octets par ligne (1 ou 2),
# puis par caractère : largeur (octet) et hauteur × octets-par-ligne octets de bitmap (bit 7 = pixel gauche).
# SPDX-License-Identifier: EUPL-1.2 — bmarty <bmarty@mailo.com>
import argparse, sys
from PIL import Image, ImageDraw, ImageFont

ap = argparse.ArgumentParser()
ap.add_argument("ttf"); ap.add_argument("size", type=int); ap.add_argument("out")
ap.add_argument("--first", type=int, default=32); ap.add_argument("--count", type=int, default=96)
ap.add_argument("--spacing", type=int, default=1); ap.add_argument("--max-width", type=int, default=16)
ap.add_argument("--threshold", type=int, default=96, help="seuil de gris (0-255) pour garder un pixel (traits fins)")
a = ap.parse_args()
font = ImageFont.truetype(a.ttf, a.size)
ascent, descent = font.getmetrics()
height = ascent + descent
if height > 255: sys.exit("hauteur > 255")
row_bytes = 1 if a.max_width <= 8 else 2
glyphs = []
clamped = 0
for code in range(a.first, a.first + a.count):
    ch = chr(code)
    left, top, right, bottom = font.getbbox(ch)
    shift = -left if left < 0 else 0                     # débord à gauche : décale le tracé
    width = max(1, round(font.getlength(ch)), right + shift)
    if width > a.max_width: width = a.max_width; clamped += 1
    im = Image.new("L", (a.max_width, height), 0)             # niveaux de gris puis seuil : garde les traits fins
    ImageDraw.Draw(im).text((shift, 0), ch, font=font, fill=255)
    rows = bytearray([width])
    for y in range(height):
        bits = 0                                          # colonne x -> octet x>>3, bit 7-(x&7)
        for x in range(width):
            if im.getpixel((x, y)) >= a.threshold: bits |= 1 << (8 * row_bytes - 1 - x)
        rows += bits.to_bytes(row_bytes, "big")
    glyphs.append(bytes(rows))
data = bytes([ord('N'), ord('F'), 1, height, a.first, a.count, a.spacing, row_bytes]) + b"".join(glyphs)
open(a.out, "wb").write(data)
print(f"{a.out} : {a.count} glyphes de {height} lignes, {row_bytes} octet(s)/ligne, {len(data)} octets"
      + (f", {clamped} glyphe(s) tronqué(s) à {a.max_width} px" if clamped else ""))
