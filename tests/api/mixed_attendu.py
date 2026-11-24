#!/usr/bin/env python3
"""mixed_attendu.py — recalcule les lignes D et P de tests/api/mixed.expected avec le décodeur de référence infopic.py du projet
Neo6502Ozmoo (chemin en argument ou variable INFOPIC). D : somme (a += pixel, b += a) du rectangle 63 x 32 de l'image 1 de l'EG1 dessinée
avec la couleur de base 37 ; P : entrées 102 et 115 après l'image 5 du MG1 avec palette et base 100. Les autres lignes sont des
octets de la grille calculés à la main dans mixed.asm (HELLO, AB, 1111/2222...). 0 si D et P sont identiques à l'attendu."""
import importlib.util, os, sys
chemin = sys.argv[1] if len(sys.argv) > 1 else os.environ.get('INFOPIC', os.path.expanduser('~/Projets/Neo6502Ozmoo/ozmoo-neo/tools/neo/infopic.py'))
spec = importlib.util.spec_from_file_location('infopic', chemin); ip = importlib.util.module_from_spec(spec); spec.loader.exec_module(ip)
ici = os.path.dirname(os.path.abspath(__file__))
d, ent, imgs = ip.lire(os.path.join(ici, 'infopic-eg.bin'))
im = [i for i in imgs if i.num == 1][0]
a = b = 0
for k, v in enumerate(ip.decompresser(d, im)):
    if k % 64 == 63: continue                                       # le rectangle relu fait 63 colonnes
    a = (a + v + 37) & 255; b = (b + a) & 255
out = ['D  00 %02X%02X' % (a, b)]
d, ent, imgs = ip.lire(os.path.join(ici, 'infopic-mg.bin'))
im = [i for i in imgs if i.num == 5][0]
pal, n, t = ip.palette(d, im)
out.append('P  00')
for e in (102, 115):
    out.append('P%02X %02X %02X %02X' % (e, *pal[e - 100]))
print('\n'.join(out))
att = [l for l in open(os.path.join(ici, 'mixed.expected')).read().splitlines() if l[:1] in 'DP']
sys.exit(0 if att == out else 1)
