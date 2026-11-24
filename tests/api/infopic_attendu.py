#!/usr/bin/env python3
"""infopic_attendu.py — recalcule les lignes S de tests/api/infopic.expected avec le décodeur de référence infopic.py du projet
Neo6502Ozmoo (chemin en argument ou variable INFOPIC). Écran de 320x240 initialement à 0 (mode 2 vient d'être choisi).
Usage : infopic_attendu.py [chemin/de/infopic.py] ; affiche les lignes S ; 0 si identiques à infopic.expected."""
import importlib.util, os, sys
chemin = sys.argv[1] if len(sys.argv) > 1 else os.environ.get('INFOPIC', os.path.expanduser('~/Projets/Neo6502Ozmoo/ozmoo-neo/tools/neo/infopic.py'))
spec = importlib.util.spec_from_file_location('infopic', chemin); ip = importlib.util.module_from_spec(spec); spec.loader.exec_module(ip)
ici = os.path.dirname(os.path.abspath(__file__))
def image(fichier, num):
    d, ent, imgs = ip.lire(os.path.join(ici, fichier))
    for im in imgs:
        if im.num == num:
            return d, im
    raise SystemExit('image absente')
ecran = [[0] * 320 for _ in range(240)]
def dessiner(fichier, num, x, y, opts):
    d, im = image(fichier, num)
    px = ip.decompresser(d, im)
    pal, n, transp = ip.palette(d, im)
    for j in range(im.haut):
        for i in range(im.larg):
            c = px[j * im.larg + i]
            if opts & 2 and transp is not None and c == transp: continue
            ii = i
            if opts & 1:
                if i & 1: continue
                ii = i >> 1
            sx, sy = x + ii, y + j
            if 0 <= sx < 320 and 0 <= sy < 240: ecran[sy][sx] = c
    return pal, n
def somme(x0, y0, w, h):
    a = b = 0
    for y in range(y0, y0 + h):
        for x in range(x0, x0 + w):
            a = (a + ecran[y][x]) & 255; b = (b + a) & 255
    return a, b
EG, MG = 'infopic-eg.bin', 'infopic-mg.bin'
etapes = [(EG,4,0,0,0,(0,0,255,60)),(EG,1,10,5,0,(0,0,100,50)),(EG,2,0,40,3,(0,36,255,30)),(EG,2,-100,50,1,(0,45,255,25)),
          (MG,5,-20,-10,8,(0,0,100,60)),(MG,5,250,150,0,(240,140,80,100)),(MG,5,20,100,4,(0,100,255,100))]
EGAC = ip.EGA
out = []
palette = list(EGAC)                                                           # palette matérielle supposée (entrées 0 à 15)
for k, (f, n, x, y, o, r) in enumerate(etapes):
    pal, nn = dessiner(f, n, x, y, o & 3)
    if o & 8: palette = list(EGAC)
    if o & 4:
        for i in range(2, nn): palette[i] = pal[i]
    a, b = somme(*r)
    out.append('S%02X 00 %02X %02X' % (k, a, b))
for k, e in enumerate((0, 1, 2, 7, 15)):
    out.append('P%02X %02X %02X %02X %02X' % (k, e, *palette[e]))
print('\n'.join(out))
att = [l for l in open(os.path.join(ici, 'infopic.expected')).read().splitlines() if l[0] in 'SP']
sys.exit(0 if att == out else 1)
