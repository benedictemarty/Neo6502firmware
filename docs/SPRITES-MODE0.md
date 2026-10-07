# Sprites en mode 0 — fonctionnement réel (T-117)

Lu dans les sources de Trinity 0.16.74 (`sprites.cpp`, `sprites_xor.cpp`, `gfxcommands.cpp`, `graphics.cpp`,
`tilemap.cpp`, `blitter.cpp`, `console.cpp`). Tout y est le comportement du code amont, sauf le redessin forcé
(§ 3, Trinity 0.16.73) et le mode opaque (§ 2 bis, Trinity 0.16.74).
Demande de Neo6502AigleDor (2026-10-05) : le portage de L'Aigle d'Or a dû découvrir tout cela dans le source.
Ce document décrit l'existant ; les améliorations proposées sont T-112, T-113, T-116 et T-118 (`docs/BACKLOG.md`).

Les points 2 à 4 sont vérifiés dans l'émulateur `neo` par `tests/api/sprmode0.asm` (un pixel relu après chaque
opération : dessin, écrasement par 12,2, « négatif » à l'effacement, absence de redessin avec l'ancre 7,
contournement par l'ancre, cible au format 4, redessin forcé, redessin systématique avec l'ancre 0). Non vérifié sur
la carte. Le mode opaque est vérifié par `tests/api/sprmode1.asm`.

## 1. Une seule mémoire, deux couches

Le mode 0 fait 320 × 240 pixels, **un octet par pixel**, dans la VRAM (page blitter `$80`). Il n'y a pas de couche
de sprites séparée : l'octet de chaque pixel est partagé.

| Bits | Contenu |
|---|---|
| 4 bits bas (`$0x`) | le fond : tracé (groupe 5), texte, tilemap |
| 4 bits hauts (`$x0`) | les sprites |

La palette par défaut (`GFXDefaultPalette`) donne aux index `$00`–`$0F` les 16 couleurs de base, et à tout index
`$yx` avec `y ≠ 0` **la couleur `y`**, quelle que soit `x`. Résultat : là où un sprite a écrit, on voit la couleur
du sprite ; ailleurs, le fond. Un programme peut changer ces 256 entrées par 5,32 (par exemple pour des effets de
transparence), mais par défaut :

- **le fond a 16 couleurs au plus** quand des sprites sont affichés ;
- **un sprite a 15 couleurs** (1 à 15) ; la couleur 0 d'une image est transparente.

## 2. Dessin par OU exclusif

`_SPXORDrawForwardLine` / `_SPXORDrawBackwardLine` : pour chaque pixel non nul de l'image, l'octet de l'écran
reçoit `octet ^= couleur << 4`. Effacer un sprite, c'est le redessiner (`SPRPHYErase` appelle `SPRPHYDraw`).

Conséquences :

- **Deux sprites qui se chevauchent mélangent leurs couleurs** (OU exclusif des deux), sans ordre d'affichage : le
  numéro du sprite ne donne aucune priorité. Le mode opaque (§ 2 bis) règle ce point.
- **L'effacement suppose que les 4 bits hauts n'ont pas bougé** depuis le dessin. Si quelque chose les a réécrits
  (section 4), effacer le sprite y fait apparaître son « négatif ».
- Les 4 bits bas ne sont jamais touchés par un sprite.

Dans les modes à pixels empaquetés (1 et 4 bits par pixel), il n'y a pas de couche : le sprite est combiné par OU
exclusif au pixel entier (`_SPXORDrawPacked`), exact sur fond noir seulement.

## 2 bis. Mode opaque (Trinity 0.16.74, T-111)

**6,6** choisit le mode de dessin : `Parameter:0` = 0 pour le OU exclusif (amont, par défaut), 1 pour le mode
opaque ; toute autre valeur rend l'erreur 1. Les sprites déjà affichés sont effacés à l'ancienne et redessinés dans
le nouveau mode. 6,1 garde le mode ; une remise à zéro du système (1,0, démarrage) revient au OU exclusif. Les
modes à pixels empaquetés (1 et 4 bits par pixel) ne sont pas concernés : ils restent en OU exclusif.

En mode opaque :

- le sprite **écrit** sa couleur dans les 4 bits hauts, et la couleur 0 de l'image reste **transparente** ;
- le sprite de **plus petit numéro est devant** (le sprite 0 passe devant le sprite 1, comme sur C64 ou NES) ;
- pour dessiner ou effacer un sprite, le firmware **remet à 0 les 4 bits hauts de son rectangle**, puis y redessine
  tous les sprites affichés qui le touchent, du plus grand numéro au plus petit. L'effacement ne dépend donc plus de
  ce qu'il y a dans la couche : après une copie 12,2 qui l'a écrasée, il n'y a plus de « négatif » ;
- en contrepartie, ce que le programme aurait mis lui-même dans les 4 bits hauts (blitter au format 3, par exemple)
  est effacé dans les rectangles des sprites qui bougent ;
- coût : chaque changement redessine les sprites qui chevauchent l'ancienne et la nouvelle place, pixel par pixel ;
  non mesuré sur la carte.

Les couleurs restent celles du § 1 : 15 couleurs de sprite dans les 4 bits hauts, avec la palette par défaut.

## 3. Quand `SPRUpdate` (6,2) redessine

Paramètres : `[0]` numéro (0–127), `[1..2]` x, `[3..4]` y, `[5]` image et taille, `[6]` retournement (bit 0 : x,
bit 1 : y), `[7]` ancre (0–9, bit 6 : forcer, voir plus bas). La valeur `$80` dans `[5]`, `[6]` ou `[7]`, ou `$80`
dans l'octet haut de x, veut dire « inchangé ».

Le sprite n'est effacé et redessiné que si **au moins un** de ces éléments change :

- la position ;
- l'octet image et taille ;
- le retournement ;
- l'ancre ;
- ou si c'est le sprite de la tortue ;
- ou, depuis Trinity 0.16.73, si le bit « forcer » est mis.

**Piège de la position** : le firmware compare le x et le y reçus au **coin haut gauche** du sprite, calculé avec
l'ancre. Les deux ne coïncident que pour l'**ancre 7** (haut gauche). Avec toute autre ancre, chaque appel qui donne
une position compte comme un changement : le sprite est toujours redessiné, et redevient visible après 6,3.

Avec l'ancre 7, ou sans position (`$80` dans l'octet haut de x), un appel où rien ne change ne fait **rien** :

- **Une image réécrite en place** dans la RAM graphique n'est pas reprise : le numéro d'image n'a pas changé.
- **Après 6,3 (masquer)**, le sprite est marqué invisible. Seul un changement de position le rend de nouveau
  visible (`isVisible` n'est remis à vrai que dans la branche « position changée »). Le réafficher au même endroit
  ne fait rien, même avec une autre image : l'image est enregistrée mais le sprite reste caché.
- Contournement connu (L'Aigle d'Or) : changer l'ancre entre deux valeurs équivalentes, par exemple 0 (centre) et
  7 (coin haut gauche) en décalant les coordonnées d'une demi-taille (8 ou 16). Le sprite reste au même endroit à
  l'écran, mais la position reçue diffère du coin haut gauche : il est redessiné, et redevient visible après 6,3.

**Forcer le redessin (Trinity 0.16.73, T-114)** : le bit 6 de l'octet d'ancre (`$40`) force l'effacement et le
redessin, même si rien n'a changé. Le reste de l'octet est l'ancre : `$47` = ancre 7 et forcer, **`$C0` = forcer
sans changer l'ancre**. Forcer relit l'adresse de l'image dans la RAM graphique, et rend visible un sprite masqué
par 6,3, à sa dernière position, s'il en a déjà reçu une. Sur un firmware sans T-114 (amont, Trinity 0.16.72 ou
plus ancien), le bit 6 rend une ancre supérieure à 9 : erreur 1, sprite effacé et pas redessiné. Un programme qui doit
tourner sur les deux peut tester cette erreur.

Pour **changer l'image d'un sprite en place** : 6,3 (masquer) **d'abord**, puis réécrire l'image, puis 6,2 avec
`$C0`. Réécrire l'image d'un sprite affiché fausse l'effacement : le OU exclusif retire la nouvelle image alors que
c'est l'ancienne qui est à l'écran.

Ancres (comme un pavé numérique) : 0 et 5 = centre ; 7, 8, 9 = haut gauche, haut milieu, haut droite ;
4, 6 = milieu gauche, milieu droite ; 1, 2, 3 = bas gauche, bas milieu, bas droite. Une ancre supérieure à 9 rend
l'erreur 1, un numéro d'image absent l'erreur 2. **Dans ces deux cas, le sprite a déjà été effacé** quand l'erreur
est rendue.

6,1 efface toute la couche (les 4 bits hauts de tous les pixels) et oublie tous les sprites ; 6,4 teste la
collision par la distance entre les points d'ancrage (sprites visibles seulement) ; 6,5 rend la position de l'ancre.

## 4. Ce qui conserve la couche des sprites, et ce qui l'écrase

| Opération | 4 bits hauts |
|---|---|
| Tracé du groupe 5 (points, lignes, rectangles, images), **tant qu'au moins un sprite est affiché** | conservés (masque `| $F0`) ; la couleur de tracé doit alors rester dans `$00`–`$0F`, sinon elle s'ajoute par OU exclusif à la couche |
| Même tracé **sans aucun sprite affiché** | écrasés par la couleur de tracé (octet entier, 256 couleurs possibles) |
| Tilemap (5,35 / 5,8) | conservés : les tuiles n'écrivent que les 4 bits bas |
| Effacement de l'écran (console) avec des sprites affichés | conservés : seuls les 4 bits bas sont remis à 0 |
| Blitter 12,2 (copie simple, octets entiers) vers la VRAM | **écrasés** |
| Blitter 12,3 (copie complexe) vers la VRAM, cible au format 0 (octet) | **écrasés** |
| Blitter 12,3, cible au format 4 (quartet bas) | conservés : seul le quartet bas est écrit |
| Blitter 12,3 actions 3 et 4 (traduction, Trinity 0.16.75), cible au format 4 | conservés |
| Blitter 12,3, cible au format 3 (quartet haut) | réécrits : c'est la couche des sprites elle-même |

Le firmware **ne sait pas** quand la couche a été écrasée : il croit toujours les sprites dessinés. En OU exclusif,
il les efface au prochain changement en y laissant leur « négatif » (section 2) ; en mode opaque, l'effacement est
propre, mais le sprite reste absent de l'écran jusqu'à son prochain redessin (6,2 avec `$C0` pour le forcer). Pour poser un fond par le blitter sous des sprites, utiliser
une cible au **format 4** (quartet bas, valeurs 0–15) plutôt que 12,2. Si les données du fond ne sont pas déjà
des couleurs 0–15 (octets d'une autre machine, par exemple), les actions 3 et 4 de 12,3 (Trinity 0.16.75, T-115)
les traduisent pendant la copie par une table de 256 octets, l'action 4 avec une couleur transparente. Proposition pour aller plus loin : T-112.

## 5. Les images : la RAM graphique (page `$90`)

Les images de sprites et de tuiles vivent dans `gfxObjectMemory`, 32 Ko (`GFX_MEMORY_SIZE`), page blitter `$90`.
On l'emplit par 3,2 (lire un fichier) à l'adresse `$FFFF`, ou par le blitter.

En-tête de 256 octets :

| Octet | Contenu |
|---|---|
| `[0]` | non nul = graphismes présents (`makeimg.py` de NeoBASIC écrit 1) ; à 0, l'affichage d'images (5,7) ne fait rien |
| `[1]` | nombre de tuiles 16 × 16 |
| `[2]` | nombre de sprites 16 × 16 |
| `[3]` | nombre de sprites 32 × 32 |
| `[4..255]` | non lus par le firmware |

Puis, à partir de l'offset 256 et sans trou : les tuiles 16 × 16 (128 octets chacune), les sprites 16 × 16
(128 octets), les sprites 32 × 32 (512 octets). Exemple vérifié : `graphics.gfx` de NeoBASIC commence par
`01 08 06 05` et fait 256 + 8 × 128 + 6 × 128 + 5 × 512 = 4 608 octets.

Format d'une image : 4 bits par pixel, deux pixels par octet, **pixel de gauche dans le quartet haut**, ligne par
ligne de haut en bas, pixel 0 transparent.

Dans 6,2, l'octet `[5]` vaut `numéro | $40` pour un 32 × 32, `numéro` pour un 16 × 16 : bits 0–5 = numéro, d'où
**64 images au plus par taille**, et seulement ces deux tailles. Une image qui déborderait des 32 Ko est refusée
(erreur 2). Propositions : T-113 (tailles libres, palette par sprite), T-116 (plus de RAM graphique).
