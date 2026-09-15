# ADR-02 — Architecture multi-modes vidéo (épopée F5)

Statut : **proposée** (2026-09-15), implémentée en F-51 (branche `feat/video-modes`),
à ratifier par bmarty.

## Contexte
Le firmware amont n'a qu'un mode : 320×240 × 256 couleurs (8 bpp, 76,8 Ko),
`GFXSetMode(int)` ignore son paramètre. Le PO a décidé (2026-09-15) de deux
modes supplémentaires : **Hercules** (texte 80×25 en 9×14, graphique 720×348)
et **320×256 × 16 couleurs** (texte 40×32 en 8×8), plus des pages écran (F-55).
Le budget de rendu (F-50) impose des tampons compacts : 1 bpp pour 720×350
(31,5 Ko), 4 bpp pour 320×256 (41 Ko) — tous deux tiennent dans les 76,8 Ko
déjà réservés, sans mémoire supplémentaire.

## Décision
1. **Descripteur de mode** (`struct GraphicsModeDescriptor`, table `gfxModes[]`
   dans `graphics.cpp`) : taille en pixels, bits par pixel, stride, taille de la
   console, cellule de caractère, timing DVI et décalage vertical. `gMode` reçoit
   `modeID`, `bitsPerPixel`, `stride`. Table v1 :

   | # | Mode | Pixels | bpp | Stride | Console | Cellule | Timing |
   |---|---|---|---|---|---|---|---|
   | 0 | `GFX_MODE_320x240x256` | 320×240 | 8 | 320 | 53×30 | 6×8 | 640×480 doublé (inchangé) |
   | 1 | `GFX_MODE_HERCULES` | 720×350 | 1 | 90 | 80×25 | 9×14 | 720×480 natif, 65 lignes de marge |
   | 2 | `GFX_MODE_320x256x16` | 320×256 | 4 | 160 | 40×32 | 8×8 | 640×480 doublé H, `VERTICAL_REPEAT=1` |

2. **Un seul tampon** `graphicsMemory` (76,8 Ko) partagé par tous les modes ;
   format compact **MSB en premier** (1 bpp : bit 7 = pixel de gauche ; 4 bpp :
   quartet haut = pixel pair). Accès générique `GFXWritePixelRaw` /
   `GFXReadPixelRaw` ; les chemins rapides 8 bpp du mode 0 sont conservés
   **octet pour octet** (vérifié par le différentiel et le golden Phosphoneo).
3. **API** : groupe 5, `FUNCTION 9 Set Graphics Mode` (P0 = mode ; erreur si
   inconnu ou non supporté par l'hôte) et `FUNCTION 10 Get Graphics Mode`
   (mode, largeur, hauteur, bpp, colonnes, lignes). `GFXSetMode` réinitialise
   écran, palette (en monochrome, l'index 1 est blanc par défaut) et console.
4. **Capacités par hôte** : `RNDModeSupported(mode)` — l'émulateur `neo` et
   Phosphoneo affichent tout (rendu via `GFXReadPixelRaw`) ; le rendu DVI de la
   carte n'accepte que le mode 0 tant que F-52/F-53 ne sont pas écrits (pas
   d'affichage corrompu possible).
5. **Périmètre des modes compacts en v1** : console (texte, curseur, défilement),
   pixel, ligne, rectangle, ellipse, lecture pixel. **Sprites, tilemaps,
   images (5,7) sont refusés hors mode 0** (erreur) : ils reposent sur la couche
   sprites dans le quartet haut du 8 bpp. Le blitter travaille sur des octets
   bruts et reste utilisable (avec le stride du mode). Police : la 5×7 (6×8)
   actuelle est dessinée dans la cellule, les lignes au-delà de 8 sont fond ; la
   police 9×14 réelle arrive avec F-52.
6. `MAXCONSOLEHEIGHT` passe de 30 à 43 (mémoire console : 80×44 mots).

## Conséquences
- + Aucun surcoût pour le mode 0 (chemins inchangés, un test `bitsPerPixel != 8`).
- + Les projets frères (Télémon, Neo6502bbc, Neo6502oric2) peuvent viser les
  modes 1 et 2 dès maintenant dans les émulateurs ; F-52/F-53 ne concernent
  plus que le rendu DVI et la police.
- − Les primitives en modes compacts passent par un chemin pixel à pixel (lent) :
  optimisations (hline par octets, memset) à faire quand un besoin apparaît.
- − Tout objet compilé contre `graphics.h` doit être recompilé (tailles de
  tableaux) : dépendances ajoutées dans le Makefile de Phosphoneo.

## Alternatives écartées
- Un tampon par mode (mémoire : 76,8 + 31,5 + 41 Ko > 47 Ko libres).
- 8 bpp partout avec réduction au rendu : 720×350 × 8 bpp = 252 Ko, impossible.
