#!/usr/bin/env python3
# api_fonctions.py — liste des fonctions de l'API (groupe,fonction) relevée dans les dispatchs générés
# (dispatch_code.h, dispatch_toolbox.h). Garde-fou né de T-118 : une insertion dans un .inc avait effacé
# l'en-tête de 5,42, et la fonction avait disparu sans qu'aucun test ne le voie.
#   api_fonctions.py            compare à tests/api/fonctions.txt : 1 si une fonction a disparu
#   api_fonctions.py --ref      réécrit tests/api/fonctions.txt (après l'ajout voulu d'une fonction)
# Auteur : bmarty <bmarty@mailo.com>
import os,re,sys
ICI = os.path.dirname(os.path.realpath(__file__))
RACINE = os.path.realpath(os.path.join(ICI,'..','..'))
DATA = os.path.join(RACINE,'firmware','common','include','data')
REF = os.path.join(RACINE,'tests','api','fonctions.txt')

def relever(chemin):
	vu = set()
	groupe = None
	for ligne in open(chemin):
		m = re.match(r'^\t(?:case|//\s*group)\s*(\d+)\s*:',ligne)                # case N: d'un groupe (une tabulation)
		if m: groupe = int(m.group(1));continue
		m = re.match(r'^\t\t\tcase\s+(\d+)\s*:',ligne)                           # case N: d'une fonction (trois)
		if m and groupe is not None: vu.add((groupe,int(m.group(1))))
	return vu

fonctions = set()
for nom in ('dispatch_code.h','dispatch_toolbox.h'):
	fonctions |= relever(os.path.join(DATA,nom))
texte = ''.join('%d,%d\n' % f for f in sorted(fonctions))
if '--ref' in sys.argv:
	open(REF,'w').write(texte);print('api_fonctions : %d fonctions écrites dans %s' % (len(fonctions),REF));sys.exit(0)
ref = set(tuple(int(v) for v in l.split(',')) for l in open(REF) if l.strip())
perdues = sorted(ref - fonctions);nouvelles = sorted(fonctions - ref)
for g,f in perdues: print('api_fonctions : %d,%d a DISPARU' % (g,f))
for g,f in nouvelles: print('api_fonctions : %d,%d nouvelle (absente de fonctions.txt : --ref)' % (g,f))
if perdues or nouvelles: print('api_fonctions : ÉCHEC');sys.exit(1)
print('api_fonctions : OK (%d fonctions)' % len(fonctions))
