# ***************************************************************************************
#
#      Name :      mda14_latin1.py
#      Author :    bmarty <bmarty@mailo.com>
#      Purpose :   T-85 : build font_latin1_8x14.h, the 14 line glyphs of Latin-1 $A0-$FF for
#                  the 9x14 cells of mode 1 (Hercules), in the style of the IBM MDA character
#                  generator (same source as mda14.py : Bm437_IBM_MDA.otb, VileR, CC BY-SA 4.0).
#
#                  Three origins, recorded per glyph in the generated file :
#                    cp437   the character exists in CP437 : the MDA glyph itself.
#                    compose a letter CP437 lacks (À, Ã, Õ, Ý...) : built as the MDA builds its
#                            own accented letters — capitals : accent on rows 0-1, body squeezed
#                            from 9 rows (2-10) to 8 (3-10) by dropping a repeated row, as Ä and
#                            Ñ are ; small letters : accent on rows 2-3 over the unchanged body.
#                    8line   no MDA model (§, ©, ®, ¤...) : the 8 line Latin-1 glyph, centred as
#                            the console drew it before.
#
#      Usage :     python3 mda14_latin1.py [source.otb] [dest.h] [preview.png]
#
# ***************************************************************************************
import sys, unicodedata, re
from PIL import Image, ImageDraw, ImageFont

src = sys.argv[1] if len(sys.argv) > 1 else "assets/Bm437_IBM_MDA.otb"
dst = sys.argv[2] if len(sys.argv) > 2 else "../common/include/interface/font_latin1_8x14.h"
png = sys.argv[3] if len(sys.argv) > 3 else None

PAD, CELL_X, CELL_Y, W, H = 2, 9, 14, 8, 14
font = ImageFont.truetype(src, 16)


def mda(ch):
    """14 rows (8 bits, MSB left) of the MDA glyph of a CP437 character."""
    code = ch.encode("cp437")[0]
    image = Image.new("1", (CELL_X + 2 * PAD, CELL_Y + 2 * PAD + 6), 0)
    ImageDraw.Draw(image).text((PAD, PAD), bytes([code]).decode("cp437"), font=font, fill=1)
    p = image.load()
    return [sum(1 << (7 - x) for x in range(W) if p[PAD + x, PAD + 2 + y]) for y in range(CELL_Y)]


def mda_direct(ch):
    """The MDA glyph of a character the font maps directly (¶ § : CP437 $14 $15, control codes to Python)."""
    image = Image.new("1", (CELL_X + 2 * PAD, CELL_Y + 2 * PAD + 6), 0)
    ImageDraw.Draw(image).text((PAD, PAD), ch, font=font, fill=1)
    p = image.load()
    return [sum(1 << (7 - x) for x in range(W) if p[PAD + x, PAD + 2 + y]) for y in range(CELL_Y)]


def rows(*patterns):
    return [int(r.replace(".", "0").replace("#", "1"), 2) for r in patterns]


# Accents drawn by the MDA itself, 2 rows each (capitals : rows 0-1 ; small letters : rows 2-3).
ACCENT = {
    "́": rows("....##..", "...##..."),                                     # acute, É
    "̀": rows("..##....", "...##..."),                                     # grave, mirror of the acute
    "̂": rows("...###..", "..##.##."),                                     # circumflex, â
    "̃": rows("..###.##", ".##.###."),                                     # tilde, Ñ ñ
    "̈": rows("........", ".##...##"),                                     # diaeresis, Ä
}


def squeeze(body):
    """9 rows -> 8 : drop the last row equal to the one above it (the MDA does the same for Ä)."""
    for i in range(len(body) - 1, 0, -1):
        if body[i] == body[i - 1]:
            return body[:i] + body[i + 1:]
    return body[:-1]


def compose(ch):
    base, *marks = unicodedata.normalize("NFD", ch)
    if len(marks) != 1 or marks[0] not in ACCENT:
        return None
    g = mda(base)
    if base.isupper():
        return ACCENT[marks[0]] + [0] + squeeze(g[2:11]) + g[11:]
    out = list(g)
    out[2:4] = ACCENT[marks[0]]
    return out


def slashed(base):                                                              # Ø ø : the MDA has no slashed O
    g = mda(base)
    top, bottom = (2, 10) if base.isupper() else (5, 10)
    for i, y in enumerate(range(bottom, top - 1, -1)):
        x = 1 + (i * 6) // (bottom - top)
        g[y] |= 0x80 >> x
    return g


def eth(base):                                                                  # Ð : D with a bar ; ð drawn by hand
    if base == "D":
        g = mda("D")
        g[6] |= rows("####....")[0]
        return g
    return [0, 0] + rows("..##.#..", "...##...", "..#.##..", ".....##.", "..######", ".##...##",
                         ".##...##", ".##...##", "..#####.") + [0, 0, 0]


def thorn(upper):                                                               # Þ þ, drawn in the MDA style
    if upper:
        return [0, 0] + rows(".####...", "..##....", "..#####.", "..##..##", "..##..##", "..##..##",
                             "..#####.", "..##....", ".####...") + [0, 0, 0]
    return [0, 0] + rows(".###....", "..##....", "..##....", "..#####.", "..##..##", "..##..##",
                         "..##..##", "..#####.", "..##....", "..##....", ".####...", "........")


def eight_line(code):
    """Fallback : the 8 line glyph, rows 3-10, taken from the firmware's own tables."""
    table = FONT8["symbols"] if code < 0xC0 else FONT8["letters"]
    i = (code - 0xA0) if code < 0xC0 else (code - 0xC0)
    return [0, 0, 0] + table[i * 8:i * 8 + 8] + [0, 0, 0]


def load8():
    """Byte tables of the 8 line Latin-1 glyphs, read from the generator's output latin1font.h."""
    text = open("../common/include/data/latin1font.h").read()
    out = {}
    for name, key in (("font_latin1_symbols", "symbols"), ("font_latin1_letters", "letters")):
        m = re.search(name + r"\[[^\]]*\]\s*=\s*\{(.*?)\};", text, re.S)
        out[key] = [int(v, 0) for v in re.findall(r"0x[0-9A-Fa-f]+|\b\d+\b", m.group(1))]
    return out


FONT8 = load8()
glyphs, origin = [], []
for code in range(0xA0, 0x100):
    ch = chr(code)
    try:
        if code == 0xA0:
            raise UnicodeEncodeError("cp437", ch, 0, 1, "")                    # NBSP : blank, below
        g, o = mda(ch), "cp437"
    except UnicodeEncodeError:
        g, o = None, None
    if g is None and code == 0xA0:
        g, o = [0] * 14, "blank"
    if g is None and ch in "Øø":
        g, o = slashed("O" if ch == "Ø" else "o"), "compose"
    if g is None and ch in "Ðð":
        g, o = eth("D" if ch == "Ð" else "d"), "compose"
    if g is None and ch in "Þþ":
        g, o = thorn(ch == "Þ"), "compose"
    if g is None and ch in "¶§":
        g, o = mda_direct(ch), "cp437"
    if g is None and ch in "´¨":                                                 # spacing accents : the MDA marks, rows 2-3
        g, o = [0, 0] + ACCENT["\u0301" if ch == "´" else "\u0308"] + [0] * 10, "compose"
    if g is None and ch == "¯":
        g, o = [0, 0] + rows(".#######") + [0] * 11, "compose"
    if g is None and ch == "¸":                                                  # the cedilla of ç, alone
        g, o = [0] * 11 + mda("ç")[11:14], "compose"
    if g is None and ch == "×":
        g, o = [0] * 4 + rows(".##...##", "..##.##.", "...###..", "..##.##.", ".##...##") + [0] * 5, "compose"
    if g is None and ch == "­":
        g, o = mda("-"), "compose"                                              # soft hyphen = hyphen
    if g is None:
        c = compose(ch)
        if c is not None:
            g, o = c, "compose"
    if g is None:
        g, o = eight_line(code), "8line"
    glyphs.append(g)
    origin.append(o)

out = ["// Generated by firmware/scripts/mda14_latin1.py (T-85) from Bm437_IBM_MDA.otb (IBM MDA 8x14),",
       "// Ultimate Oldschool PC Font Pack v2.2 by VileR (int10h.org), CC BY-SA 4.0. Latin-1 $A0-$FF,",
       "// 14 rows per glyph, MSB left. Origin : cp437 = MDA glyph, compose = MDA letter + MDA accent,",
       "// 8line = the console's 8 line glyph centred (no MDA model). Do not edit.",
       "#pragma once", "#include <cstdint>", "", "const uint8_t font_latin1_8x14[] = {"]
for i, (g, o) in enumerate(zip(glyphs, origin)):
    code = 0xA0 + i
    name = unicodedata.name(chr(code), "NO-BREAK SPACE").lower()
    out.append("\t" + ",".join("0x%02x" % b for b in g) + ",  // $%02X %s (%s)" % (code, name, o))
out.append("};")
open(dst, "w").write("\n".join(out) + "\n")

counts = {k: origin.count(k) for k in ("cp437", "compose", "8line", "blank")}
print("font_latin1_8x14 : %s" % counts)
print("8line : " + " ".join(chr(0xA0 + i) for i, o in enumerate(origin) if o == "8line"))

if png:                                                                         # Preview : 16 x 6 cells of 9 x 14, x3
    S = 3
    im = Image.new("RGB", (16 * 9 * S + 16, 6 * 14 * S + 12), (40, 40, 40))
    px = im.load()
    for i, g in enumerate(glyphs):
        ox, oy = (i % 16) * (9 * S + 1), (i // 16) * (14 * S + 2)
        colour = {"cp437": (255, 255, 255), "compose": (120, 220, 255), "8line": (255, 170, 80), "blank": (255, 255, 255)}[origin[i]]
        for y in range(14):
            for x in range(9):
                on = x < 8 and (g[y] >> (7 - x)) & 1
                for dy in range(S):
                    for dx in range(S):
                        px[ox + x * S + dx, oy + y * S + dy] = colour if on else (0, 0, 0)
    im.save(png)
