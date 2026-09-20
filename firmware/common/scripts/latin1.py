# ***************************************************************************************
# ***************************************************************************************
#
#      Name :      latin1.py
#      Authors :   bmarty (bmarty@mailo.com)
#      Date :      18th September 2026
#      Purpose :   Latin-1 glyphs (F-17) : $A0-$BF symbols (built in, flash) and $C0-$FF
#                  letters (defaults of the user defined font $C0-$FF, restored at reset,
#                  replaced by 2,5). 5x7 pixels in 6x8 cells like font_5x7.h : bit 7 =
#                  left pixel, row 7 empty. Letters are composed from font_5x7 : lower
#                  case (rows 2-6) under a 2 row accent, capitals squeezed to 6 rows.
#                  Usage : python3 scripts/latin1.py include/interface/font_5x7.h > include/data/latin1font.h
#
# ***************************************************************************************
# ***************************************************************************************

import sys,re

def rows(s):                                                                   # "..X.. .XXX. ..." -> 7 bytes
    r = [int(x.replace(".","0").replace("X","1"),2) << 3 for x in s.split()]
    assert len(r) == 7,s
    return r

# Built in font $20-$7F, from font_5x7.h
font = {}
data = re.findall(r"^\s*((?:0x[0-9a-fA-F]{2},){8})\s*//\s*\$([0-9a-fA-F]{2})",open(sys.argv[1]).read(),re.M)
for bytesText,code in data:
    font[int(code,16)] = [int(x,16) for x in bytesText.split(",") if x != ""]
assert len(font) == 96,"font_5x7.h : {0} glyphs".format(len(font))

ACCENT = {                                                                     # Rows 0-1
    "grave":".X... ..X..", "acute":"...X. ..X..", "circ":"..X.. .X.X.",
    "tilde":".X.X. X.X..", "diaer":".X.X. .....", "ring":".X.X. ..X..",
}

def lower(base,accent):                                                        # Lower case : accent rows 0-1, letter rows 2-6 as is
    g = font[ord(base)][:]
    a = rows(ACCENT[accent] + " ..... ..... ..... ..... .....")
    return [a[0],a[1]] + g[2:7]

def capital(base,accent):                                                      # Capital : squeezed to rows 1-6 (a duplicate row dropped)
    g = font[ord(base)][:7]
    drop = 1
    for i in range(1,7):
        if g[i] == g[i-1]: drop = i;break
    del g[drop]
    a = rows(ACCENT[accent] + " ..... ..... ..... ..... .....")
    return [a[0]] + g[0:6]

def cedilla(base,isCapital):                                                   # Ç / ç : letter in rows 0-5, cedilla in row 6
    g = font[ord(base)][:7]
    if isCapital:
        drop = 1
        for i in range(1,7):
            if g[i] == g[i-1]: drop = i;break
        del g[drop]
        body = g[0:6]
    else:
        body = [0] + g[2:7]
    return body + [0x20]

SYMBOLS = [                                                                    # $A0-$BF
    "..... ..... ..... ..... ..... ..... .....",   # $A0 no break space
    "..X.. ..... ..X.. ..X.. ..X.. ..X.. ..X..",   # $A1 ¡
    "..X.. .XXX. X.X.. X.X.. .XXX. ..X.. .....",   # $A2 ¢
    "..XX. .X..X .X... XXX.. .X... .X... XXXXX",   # $A3 £
    "..... X...X .XXX. .X.X. .XXX. X...X .....",   # $A4 ¤
    "X...X .X.X. ..X.. XXXXX ..X.. XXXXX ..X..",   # $A5 ¥
    "..X.. ..X.. ..X.. ..... ..X.. ..X.. ..X..",   # $A6 ¦
    ".XXX. X.... .XXX. X...X .XXX. ....X .XXX.",   # $A7 §
    ".X.X. ..... ..... ..... ..... ..... .....",   # $A8 ¨
    ".XXX. X.XXX XX..X XX..X X.XXX .XXX. .....",   # $A9 ©
    ".XXX. ....X .XXXX X...X .XXXX ..... XXXXX",   # $AA ª
    "..... ..X.X .X.X. X.X.. .X.X. ..X.X .....",   # $AB «
    "..... ..... XXXXX ....X ....X ..... .....",   # $AC ¬
    "..... ..... ..... .XXX. ..... ..... .....",   # $AD soft hyphen
    ".XXX. XXX.X X.X.X XXX.X X.X.X .XXX. .....",   # $AE ®
    "XXXXX ..... ..... ..... ..... ..... .....",   # $AF ¯
    "..X.. .X.X. ..X.. ..... ..... ..... .....",   # $B0 °
    "..X.. ..X.. XXXXX ..X.. ..X.. ..... XXXXX",   # $B1 ±
    ".XX.. ...X. ..X.. .X... .XXX. ..... .....",   # $B2 ²
    "XXX.. ...X. .XX.. ...X. XXX.. ..... .....",   # $B3 ³
    "...X. ..X.. ..... ..... ..... ..... .....",   # $B4 ´
    "..... ..... X...X X...X X...X XXXX. X....",   # $B5 µ
    ".XXXX X.X.X X.X.X .XX.X ..X.X ..X.X ..X.X",   # $B6 ¶
    "..... ..... ..... ..X.. ..... ..... .....",   # $B7 ·
    "..... ..... ..... ..... ..... ..X.. .XX..",   # $B8 ¸
    "..X.. .XX.. ..X.. ..X.. .XXX. ..... .....",   # $B9 ¹
    ".XX.. X..X. X..X. .XX.. ..... XXXX. .....",   # $BA º
    "..... X.X.. .X.X. ..X.X .X.X. X.X.. .....",   # $BB »
    "X...X X..X. X.X.. .X.X. ..XXX ....X ....X",   # $BC ¼ (approximation)
    "X...X X..X. X.X.. .XXX. ....X ...X. ..XXX",   # $BD ½ (approximation)
    "XX..X ..XX. XX.X. .X.X. ..XXX ....X ....X",   # $BE ¾ (approximation)
    "..X.. ..... ..X.. .X... X.... X...X .XXX.",   # $BF ¿
]

SPECIAL = {                                                                    # Letters with no accent composition
    0xC6:".XXXX X.X.. X.X.. XXXX. X.X.. X.X.. X.XXX",   # Æ
    0xD0:"XXX.. X..X. X...X XX..X X...X X..X. XXX..",   # Ð
    0xD7:"..... X...X .X.X. ..X.. .X.X. X...X .....",   # ×
    0xD8:".XXX. X..XX X.X.X X.X.X X.X.X XX..X .XXX.",   # Ø
    0xDE:"X.... XXXX. X...X X...X XXXX. X.... X....",   # Þ
    0xDF:".XX.. X..X. X.X.. X.XX. X...X X...X X.XX.",   # ß
    0xE6:"..... ..... XX.X. ..X.X .XXXX X.X.. .XXXX",   # æ
    0xF0:".X.X. ..X.. .X.X. ....X .XXXX X...X .XXX.",   # ð
    0xF7:"..... ..X.. ..... XXXXX ..... ..X.. .....",   # ÷
    0xF8:"..... ..... .XXX. X..XX X.X.X XX..X .XXX.",   # ø
    0xFE:"X.... X.... XXXX. X...X X...X XXXX. X....",   # þ
}

ORDER = "grave acute circ tilde diaer".split()
LETTERS = {}
for base,codes in (("A",0xC0),("E",0xC8),("I",0xCC),("O",0xD2),("U",0xD9),("a",0xE0),("e",0xE8),("i",0xEC),("o",0xF2),("u",0xF9)):
    for i,acc in enumerate(ORDER):
        if base in "EIei" and acc == "tilde": continue
        if base in "Uu" and acc == "tilde": continue
        if base in "Oo" and acc == "tilde" or base in "Aa" and acc == "tilde": pass
        code = codes + i
        if base in "IiEe" and i >= 3: code = codes + i - 1                    # No tilde for E and I
        if base in "Uu" and i >= 3: code = codes + i - 1
        LETTERS[code] = (capital if base.isupper() else lower)(base,acc)
LETTERS[0xC5] = capital("A","ring");LETTERS[0xE5] = lower("a","ring")
LETTERS[0xC7] = cedilla("C",True);LETTERS[0xE7] = cedilla("c",False)
LETTERS[0xD1] = capital("N","tilde");LETTERS[0xF1] = lower("n","tilde")
LETTERS[0xDD] = capital("Y","acute");LETTERS[0xFD] = lower("y","acute");LETTERS[0xFF] = lower("y","diaer")
for code,s in SPECIAL.items(): LETTERS[code] = rows(s)
assert sorted(LETTERS.keys()) == list(range(0xC0,0x100)),[hex(c) for c in range(0xC0,0x100) if c not in LETTERS]

def emit(name,glyphs,first):
    print("static const uint8_t {0}[] = {{".format(name))
    for i,g in enumerate(glyphs):
        g = list(g) + [0] * (8 - len(g))
        print("\t{0}, // ${1:02X}".format(",".join("0x{0:02x}".format(b) for b in g),first + i))
    print("};")

print("//\n//\tGenerated by scripts/latin1.py (F-17) : Latin-1 glyphs $A0-$BF and $C0-$FF defaults.\n//")
emit("font_latin1_symbols",[rows(s) for s in SYMBOLS],0xA0)
emit("font_latin1_letters",[LETTERS[c] for c in range(0xC0,0x100)],0xC0)
