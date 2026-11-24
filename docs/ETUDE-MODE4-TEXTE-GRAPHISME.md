# Étude : mode 4, texte et graphisme en même temps (T-163)

Date : 2026-11-24. Demande bmarty : « étudie une couche texte superposable au graphisme », précisée en « mode 4 ».
**Étude seulement : aucun code, aucune mesure faite pour ce document.** Ce qui est mesuré est cité avec sa source ;
ce qui est estimé ou non vérifié est dit tel quel.

## 1. Question

Le mode 3 (T-131) est un mode texte pur (80 × 25 cellules 8 × 8, couleur par cellule) : il refuse le dessin et n'a
aucun pixel. Le mode 0 a le graphisme et des sprites, et le texte de la console y est écrit comme des pixels (4 bits bas).
Existe-t-il un moyen d'avoir **une grille de texte colorée, indépendante des pixels, affichée par-dessus un graphisme** ?
Hypothèse retenue : un **nouveau mode 4** (`GFX_MODE_COUNT` passe de 4 à 5).

## 2. Faits relevés dans le code (0.16.113)

| Fait | Source |
|---|---|
| Modes 0..3 ; mode 3 = `bitsPerPixel == 0`, 640 × 480p60, 200 lignes de texte centrées dans 240 (`yOffset` 20) | `dvi_320x240x256.cpp`, table des modes ; `graphics.h` |
| Les lignes sont doublées : budget de **16 000 cycles** par ligne encodée (8 000 par ligne DVI) | `BACKLOG.md` T-131 |
| Mode 3 : encodeur `tmds_encode_font_2bpp` sur le cœur 1, **7 400 cycles en moyenne, 7 523 au pire sur 16 000**, aucune ligne en retard (carte, essai c1086931) | commentaire de `dvi_320x240x256.cpp`, T-131 |
| Mode 0 : le rappel de ligne (IRQ du cœur 1) convertit 320 octets d'indices en RGB565 par `palette[256]`, quatre pixels par lecture, **puis dessine le curseur souris dans le même tampon** ; l'encodeur TMDS passe ensuite | `_scanline_callback`, lignes 330-366 |
| `graphicsMemory` = `MAXGRAPHICSMEMORY` = 320 × 240 = **76 800 o**, alignée sur un mot ; en mode 3 elle contient la grille (7 000 o) et la table de l'encodeur et la police (4 096 o) | `graphics.h`, `memory.cpp`, `text80.h` |
| Le cœur 1 ne doit rien exécuter ni lire depuis la flash (T-44, T-93, T-101, T-102) ; garde-fou `make test-coeur1` | `CLAUDE.md`, commentaires |
| Marge de RAM statique : **196 o** (0.16.113 : 234 204 sur une limite de 234 400) | Makefile, README-TRINITY.md |
| Les fonctions 5,13 Text Write, 5,14 Text Scroll, 5,15 Text Clear existent, refusées hors du mode 3 | `group5_graphics.inc`, README 0.16.89 |
| Police CP437 8 × 8 de 2 048 o déjà fournie (`T80Font`, 256 × 8 o, bit de poids fort à gauche) | `text80.h` |
| **Mesure inexistante** : coût du rappel de ligne + encodeur du mode 0 sur carte (seuls `RNDCallbackMax` et les compteurs de lignes en retard existent ; T-158 n'a pas mesuré la marge du cœur 1) | `BACKLOG.md` T-158, T-162 |

## 3. Pourquoi pas « mode 3 + graphisme »

Le mode 3 encode **640 pixels natifs** par ligne avec des tables de police sur trois voies TMDS à 2 bits (RGB222). Un graphisme
256 couleurs en RGB565 ne peut pas être mêlé à ces voies sans réencoder les 640 pixels en 16 bpp non doublés, soit au moins
le double de l'encodage 16 bpp à 320 pixels des modes 0 et 2. Ce coût **n'est pas mesuré** ; comme le mode 3 seul prend déjà
environ 47 % du budget, je tiens cette voie pour non viable sans mesure contraire. Le graphisme 640 × 200 en 4 bpp
(64 000 o) tiendrait en mémoire, mais pas le temps de ligne : écartée.

## 4. Proposition : mode 4 = graphisme 320 × 200 × 256 + texte 40 × 25

### 4.1 Principe

- Timing du mode 3 (640 × 480p60, 200 lignes centrées dans 240, lignes doublées, budget 16 000 cycles) ; le chemin du mode 0
  (indices → RGB565 → `tmds_encode_data_channel_16bpp`) ; **le texte est composé dans le tampon RGB565 de 320 pixels juste après la
  conversion de palette, avant le curseur souris** : même endroit, même technique que le curseur, qui prouve que le rappel de ligne
  peut dessiner dans ce tampon.
- Cellule 8 × 8, donc 40 colonnes × 25 rangées = 320 × 200 : une rangée de texte = 8 lignes de graphisme, sans mise à l'échelle.
- Par cellule : un code de glyphe CP437 (1 octet) et un attribut `papier << 4 | encre` (entrées 0-15 de la palette, comme en mode 3).
  Papier **transparent** (le graphisme reste visible) ou **opaque** : choix à trancher, voir § 6.
- Les sprites (4 bits hauts de la VRAM) et le fond sont inchangés. Le texte est **toujours au-dessus des sprites** (il est composé
  après eux) ; il n'y a pas de moyen de passer un sprite devant le texte. Le curseur souris reste au-dessus du texte.

### 4.2 Mémoire (calcul, pas mesure)

| Zone | Octets | Position dans `graphicsMemory` |
|---|---|---|
| Pixels 320 × 200 × 8 bits | 64 000 | $0000 |
| Glyphes 40 × 25 | 1 000 | 64 000 |
| Attributs 40 × 25 | 1 000 | 65 000 |
| Police 256 × 8 recopiée de la flash en RAM (le cœur 1 ne peut pas la lire en flash) | 2 048 | 66 000 |
| **Total** | **68 048** sur 76 800 | reste 8 752 o libres |

Aucune RAM statique de plus pour les données : tout loge dans `graphicsMemory`, comme la grille du mode 3. Reste à compter le
code du rappel (inline, en RAM) et quelques variables d'état : à comparer à la **marge de 196 o**, non chiffré.

### 4.3 Coût en cycles (estimation, à confirmer par mesure)

La composition écrit au plus 320 pixels RGB565 par ligne, soit le même nombre d'écritures que la conversion de palette actuelle ;
avec un papier transparent, seuls les pixels allumés sont écrits, et une cellule vide (octet de police nul) se saute en une
comparaison. Je **ne chiffre pas** en cycles : je n'ai pas le coût du rappel du mode 0, donc pas la marge. C'est la première
mesure à faire (§ 7). Le rappel s'exécute en IRQ sur le cœur 1 et préempte l'encodeur : tout cycle ajouté se paie sur les lignes
en retard (`RNDCallbackMax`, `RNDIrqGap*`, `RNDCtrlOverrun*`).

### 4.4 Variantes

| Variante | Avantage | Inconvénient |
|---|---|---|
| **A. 40 × 25, cellules 8 × 8** (recommandée) | police existante, lisible, coût borné | 40 colonnes seulement |
| B. 80 × 25, glyphes de 4 × 8 | 80 colonnes ; la console du mode 2 a déjà des glyphes de 4 px | lisibilité faible, police à créer, coût identique en écritures mais deux fois plus de cellules à parcourir |
| C. 640 natifs | texte net 80 colonnes | § 3 : non viable sans mesure |

## 5. Interface proposée

- `5,9` P0 = 4 : entre en mode 4 ; erreur « mode inconnu » dans les firmwares sans mode 4 (contre-essai évident).
- Dessin du groupe 5 et sprites du groupe 6 : fonctionnent comme en mode 0, sur 320 × 200 (zone de dessin plus petite : 200 lignes au lieu de 240).
- `5,13`, `5,14`, `5,15` (Text Write, Scroll, Clear) : mêmes signatures qu'en mode 3, la grille est de 40 × 25 ; refusées hors des
  modes 3 et 4.
- Nouvelles fonctions à numéroter (numéros libres du groupe 5 **non vérifiés** : le fichier montre 10-15 et 32-43 occupés) :
  activer/masquer la couche, papier transparent ou opaque, et éventuellement police chargeable.
- Console (groupe 2) : à décider si elle écrit dans la couche de texte (comme en mode 3) ou en pixels (comme en mode 0) ; la première
  option touche `console.cpp` (crochets `T80*` déjà présents pour le mode 3), la seconde ne change rien.
- Palette : `5,32` recolore la couche comme en mode 3 (entrées 0-15 du fond), donc le graphisme et le texte partagent ces entrées.

## 6. Décisions que seul bmarty peut prendre

1. Papier du texte : transparent seulement, opaque seulement, ou choix **par cellule** (un bit de l'attribut) ?
2. Console du groupe 2 dans la couche ou en pixels ?
3. 40 × 25 (variante A) ou 80 × 25 (variante B) ?
4. Niveau de priorité accepté : texte toujours au-dessus des sprites ?

## 7. Plan en quatre étapes (aucun go donné)

1. **Mesure** sur carte, firmware d'essai sur une branche, mode 0 occupé (fond plein, sprites en mouvement) : durée du rappel
   (`RNDCallbackMax`), lignes en retard, et coût d'une composition de 40 cellules pleines (pire cas) ; critère de décision à fixer
   avec la marge relevée, repli : papier transparent seulement ou texte limité à certaines rangées.
2. Squelette du mode 4 dans l'émulateur `neo` (rendu de la couche) puis Phosphoneo : test API `text4` (écriture, défilement,
   effacement, transparence, texte au-dessus d'un sprite, retour au mode 0) avec contre-essai.
3. Rappel de ligne de la carte : composition inline en RAM, police copiée par le cœur 0 au démarrage du mode ; `make test-coeur1`,
   `make test-api`, `make test-fonctions` ; mesure finale sur carte.
4. Console et documentation (README-TRINITY, SPRITES-MODE0 pour la priorité, fiche de l'API).

## 8. Ce qui n'est pas connu

- Le coût réel du rappel et de l'encodeur du mode 0, donc la marge du cœur 1 : **à mesurer avant tout code**.
- La taille du code ajouté au rappel face à la marge de RAM statique (196 o).
- Les numéros libres du groupe 5.
- Le comportement du changement de mode à chaud 0 ↔ 4 (les changements 0 ↔ 1 ont été validés sur carte, pas ceux-ci).

## 9. Lien avec T-162

T-162 (couche composée **de sprites**, gelée) vise la priorité sprite/fond, sous la même condition de marge du cœur 1. La mesure
de l'étape 1 sert aux deux tickets ; si elle montre une marge, T-162 et T-163 pourraient partager le passage de composition
dans le rappel de ligne.

## 10. Variante « 640 × 200 × 16 couleurs + texte 80 × 25 » (question bmarty, 2026-11-24)

Lecture de la demande : un graphisme 640 × 200 en 16 couleurs sous un texte 80 × 25 de cellules 8 × 8 (vrais 80 colonnes).
Le mode 3 actuel refuse le dessin et NeoDOS/NeoBBS en dépendent : ce serait un **mode 4**, pas un changement du mode 3.

**Correction du § 3.** J'y écartais le 640 natif en supposant un réencodage RGB565 non doublé. PicoDVI a un autre encodeur,
`tmds_encode_palette_data` (`libdvi/tmds_encode.c`, même fichier que l'encodeur 16 bpp déjà utilisé par Trinity) : il lit **un octet
d'indice par pixel** et sort les symboles TMDS par une table de palette (6 mots par entrée, interpolateurs), sans passer par le RGB565.
Son exemple `vista-palette` encode **1 280 pixels par ligne** en 720p30 avec une palette de 256 entrées.

Conséquences, **par raisonnement et non par mesure** :

| Point | Analyse |
|---|---|
| Graphisme | 640 × 200 × 4 bits = 64 000 o ; grille 80 × 25 (glyphes + attributs 4 000 o) + police 2 048 o : **70 048 o sur 76 800** dans `graphicsMemory`. Les tampons de ligne gardent leur taille : 640 octets d'indice = 640 octets de RGB565 à 320 pixels. |
| Texte transparent | Possible **à 640 pixels** : le texte est composé dans le tampon d'indices (encre/papier = entrées 0-15) avant l'encodeur à palette, donc papier transparent pixel par pixel, ce qui était impossible en RGB565 à 320. |
| Étapes par ligne | 1. dépliage des demi-octets en octets (320 → 640 o) ; 2. composition de 80 cellules ; 3. encodeur à palette sur 640 pixels. |
| Palette | 16 entrées de 24 bits (`tmds_setup_palette24_symbols`) : plus de couleurs possibles qu'en RGB222, mais 16 à l'écran en même temps. |
| Temps | Ma déduction à partir de `vista-palette` (640 pixels « au plus la moitié » d'une ligne de 720p30) était **fausse** : voir le § 11, mesuré sur la carte. |
| Dessin et sprites | Le dessin et les sprites du mode 2 (4 bits par pixel) passent par la voie « packed » pixel par pixel, **lente et en OU exclusif** (`_SPXORDrawPacked`) : c'est la contrepartie principale de cette variante. |
| Risques | interpolateurs du RP2040 partagés avec l'encodeur 16 bpp (T-101, à vérifier) ; code et table de palette en RAM (cœur 1 jamais en flash, `make test-coeur1`) ; marge de RAM statique de 196 o. |

**Première mesure** (remplace celle du § 7 si cette variante est retenue) : firmware d'essai, 640 × 200 × 4 bits statique, encodeur à palette
sur 640 pixels + composition de 80 cellules pleines, relever `RNDCallbackMax`, les lignes en retard et les cycles par ligne, comme pour
l'essai c1086931 du mode 3 (7 400 cycles sur 16 000).

## 11. Mesures sur la carte (2026-11-24)

Firmware d'essai, branche `essai-mode4-640x16` (commit `365e5aac`, jamais livré) : le mode 3 affiche un 640 × 200 × 16 couleurs
fixe (motif pseudo-aléatoire) plus la grille de texte composée dans les indices, encodé par `tmds_encode_palette_data`. Cycles du
cœur 1 relevés par SysTick à chaque ligne, lus par SWD (RP3Sonde) sur 3 s (environ 14 400 lignes par seconde). Carte flashée avec ce
firmware, puis remise sur le build 0.16.113 normal.

| Mesure | Cycles par ligne (budget 16 000) |
|---|---|
| **Étalonnage** : voie actuelle du mode 3 (`tmds_encode_font_2bpp`) | moyenne **7 458**, maximum 7 973 (valeur connue : 7 400, l'instrument est cohérent) |
| **Mode 0 : encodeur 16 bits** (NeoDOS à l'invite) | moyenne **7 349**, maximum 7 397 (46 %) |
| Nouvelle voie, texte réel (grille presque vide) | moyenne **22 486**, maximum 26 951 |
| Nouvelle voie, texte « stress » (80 cellules pleines) | moyenne **25 290** |
| dont dépliage des demi-octets en octets | environ 3 650 |
| dont composition du texte | environ 1 900 (grille presque vide) à 4 760 (80 cellules pleines) |
| dont **encodeur à palette sur 640 pixels** | environ **16 900 à lui seul** : plus que le budget de la ligne |

Compteurs de lignes après les essais : `lateTotal` = 592 316, `publishRejects` = 121 617 (lignes perdues), `displayRestarts` = 0.

**Conclusion : la voie « 640 × 200 × 16 couleurs + texte 80 colonnes » est écartée** avec l'encodeur à palette de PicoDVI à 252 MHz :
l'encodeur seul dépasse le budget, il faudrait le rendre plus de 2,3 fois plus rapide pour y loger aussi le dépliage et le texte.
Coût secondaire relevé : le code de cet encodeur (boucles en `scratch_x`) ajoute **1 040 o** de RAM statique (235 244 contre 234 204),
au-dessus de la limite de 234 400.

**Ce que la mesure donne aussi pour la variante A (mode 4, 320 × 200 + texte 40 × 25)** : le mode 0 consomme 7 349 cycles par ligne
dans l'encodeur, auxquels s'ajoute le rappel de ligne (maximum observé 16 µs sur toute la durée de l'essai, mode 0 compris, soit environ
4 000 cycles à 252 MHz). La marge restante est donc de l'ordre de **4 500 cycles par ligne** (déduction : 16 000 − 7 349 − 4 000).
La composition mesurée à 640 pixels (1 900 à 4 760 cycles) est du même ordre de grandeur que cette marge : la variante A est
**serrée, pas exclue** ; sa composition, à 320 pixels RGB565, n'a pas été mesurée. Même remarque pour T-162.

**Reste vrai (non mesuré)** : coût de la composition RGB565 à 320 pixels ; variante B (bandes de texte à 640 pixels, § « écran coupé en
bandes » : chaque ligne est encodée par une seule voie, donc 7 458 cycles pour une ligne de texte, déjà mesurés comme voie actuelle du
mode 3).
