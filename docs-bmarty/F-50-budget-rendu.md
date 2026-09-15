# F-50 — Budget de rendu vidéo de core1 (analyse sur sources, 2026-09-15)

Sources : PicoDVI récupéré par le build (`firmware/build/_deps/picodvi-src`,
`Readme.md`, `libdvi/tmds_encode.{c,h,S}`, `dvi_timing.c`, exemples `apps/`),
`firmware/sources/hardware/dvi_320x240x256.cpp`. Marquage [C] lu, [M] mesuré,
[I] inféré. **Aucune mesure sur carte n'a encore été faite** (pas de carte
raccordée) : le protocole est en §4.

## 1. Ce que fait le firmware Neo6502 aujourd'hui [C]
Timing `dvi_timing_640x480p_60hz` (bit clock **252 MHz** = horloge système),
`DVI_VERTICAL_REPEAT = 2` (défaut PicoDVI), rendu par ligne sur core1 :
conversion index → RGB565 par palette (320 pixels, boucle C) puis encodage TMDS
« pixel-doubled » 16 bpp de PicoDVI. Le Readme de PicoDVI donne pour ce chemin
**≈ 58–60 % d'un cœur M0+ à 252 MHz** pour l'encodage seul (boucle de 20
cycles pour 4 pixels, interpolateurs), le second cœur restant libre — c'est
exactement la configuration du produit, donc **validée par construction**.

## 2. Ce que PicoDVI sait faire, avec preuve par ses exemples [C]
| Encodeur | Largeur | Répétition verticale | Exemple fourni | Coût |
|---|---|---|---|---|
| `tmds_encode_data_channel_16bpp` / `_8bpp` (doublage H) | 320 → 640 | 2 | tous les jeux | ≈ 60 % core (8 bpp un peu moins cher que 16) |
| **`tmds_encode_1bpp`** (`DVI_MONOCHROME_TMDS=1`) | **640, 720, 800, 960, 1280** | **1** | `apps/terminal` (640×480, 720×480, 800×600, 960×540, 1280×720@30) | faible (table 1 bpp → symboles) |
| **`tmds_encode_2bpp`** | 640 … 1280 | 1 | `apps/colour_terminal` (police 2 bpp, 4 couleurs) | faible/moyen |
| `tmds_encode_palette_data` (palette N bits, doublage H) | 320 (1280/2 en 720p) | 2 | `apps/vista-palette` | comme 8 bpp doublé |
| `tmds_encode_data_channel_fullres_16bpp` | 640 pleine résolution couleur | 1 | image du Readme | « just barely possible, utterly impractical » (les deux interpolateurs, balance DC hors spec) |

Timings disponibles (`dvi_timing.c`) : 640×480p60 (252 MHz), **720×480p60
(270 MHz)**, **800×480p60 (295,2 MHz)**, 800×600p60 (400 MHz, 1,30 V),
800×600 réduit (354 MHz), 960×540p60 (372 MHz, celui de reload-emulator),
1280×720p30 (372 / 319,2 MHz).

## 3. Conclusions pour l'épopée F5
| Mode visé | Verdict | Base |
|---|---|---|
| **F-52 640×480 × 1 bpp** (texte 80 col., « Hercules/VGA mono ») | **faisable, prouvé** (`terminal`) ; 38,4 Ko ; double tampon possible | [C] |
| **F-53 640×480 × 2 bpp** (4 couleurs) | **faisable, prouvé** (`colour_terminal`, `tmds_encode_2bpp`) ; 76,8 Ko | [C] |
| F-53 640×240 × 4 bpp (16 couleurs) | **pas d'encodeur 4 bpp pleine résolution** dans PicoDVI ; il faudrait le dériver du 2 bpp ou du fullres 16 bpp (impraticable) → **non retenu** | [C] |
| F-56 **Hercules 720×348 × 1 bpp** | faisable : timing **720×480p60** (270 MHz) + `tmds_encode_1bpp`, 348 lignes centrées (66 lignes noires haut/bas) | [C] timing + encodeur ; centrage [I] |
| F-56 400×240 × 8 bits en 800×480 | timing 800×480 (295 MHz) + doublage : coût ≈ 60 % × 800/640 ≈ **75 %** d'un cœur [I] ; RAM 96 Ko ✓ | [I] |
| F-54 **320×256 × 8 bits** (lignes non doublées) | avec `VERTICAL_REPEAT=1` il faut encoder **480 lignes au lieu de 240** : si les 60 % du Readme sont mesurés avec répétition 2, le coût double (**≈ 120 %, infaisable**) ; sinon (60 % pour 480 encodes) c'est bon → **à trancher par la mesure §4** | [I] |
| F-55 pages écran 320×240 × 4 bpp ×2 | encodage inchangé (doublé), seul le tampon change : faisable ; **4 bpp doublé** : `tmds_encode_palette_data` avec `PALETTE_BITS=4` ou conversion palette → 8 bpp existante | [C]/[I] |
| 640×480 × 16 ou 256 couleurs | **non** (encodeur impraticable + RAM) | [C] |

## 4. Protocole de mesure sur carte (à faire dès qu'une carte est raccordée)
1. Dans `dvi_320x240x256.cpp`, autour du corps de `_scanline_callback` :
   `uint32_t t0 = timer_hw->timerawl;` … accumuler `(timer_hw->timerawl - t0)`
   sur 240 lignes ; publier la moyenne et le max par trame dans deux mots lus
   par une nouvelle fonction API de debug (groupe 1, ≥ 12) ou sur l'UART de
   debug (`FDBWrite`) ; lire depuis le 6502 via `nxmit`/BASIC.
2. Rapporter à la durée d'une ligne : 640×480p60 = 31,8 µs/ligne (8 000 cycles
   à 252 MHz) ; le budget de core1 = cycles de rendu + encodage / 8 000.
3. Refaire avec `DVI_VERTICAL_REPEAT=1` (480 encodes) pour trancher F-54, puis
   avec `tmds_encode_1bpp` sur 640 et 720 (F-52/F-56) et `tmds_encode_2bpp`.
4. Vérifier la stabilité DVI (pas de décrochage) et la température/tension
   (1,20 V à 252/270 MHz ; 1,30 V au-delà).
En co-simulation (Phosphoneo) rien de tout cela n'est mesurable : le DVI y est
HLE (`DVIStart` court-circuité).
