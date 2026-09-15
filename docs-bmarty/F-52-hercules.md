# F-52 — Mode Hercules (720×350 × 1 bpp, texte 80×25 en 9×14) et rendu DVI multi-modes

Date : 2026-09-15. État : **code écrit et compilé, non exécuté sur carte** (aucun
Neo6502 disponible). Ce qui est vérifié, ce qui ne l'est pas, et le protocole pour
la première carte.

## Vérifié (émulateurs)
- Console 80×25 en cellules 9×14 avec la police 8×14 (`font_8x14.h`, générée par
  `firmware/scripts/vga14.py` depuis `Lat15-VGA14.psf.gz` de console-setup — Debian :
  « All console fonts are public domain by nature ») ; 9e colonne = fond ; glyphes
  8 lignes ($80-$BF, UDG $C0-$FF) centrés verticalement.
- Démo `videomode1` identique entre Phosphoneo et `neo` (`make test-diff`), et sur le
  **vrai firmware ARM en co-simulation** (`--neo-emu firmware.elf`, même texte
  console) : tout le code commun du mode 1 est donc exercé ; seul le rendu DVI
  (`dvi_320x240x256.cpp`) ne l'est pas (HLE de `DVIStart`).

## Écrit, non vérifié (carte)
`firmware/sources/hardware/dvi_320x240x256.cpp` :
1. Table `displayTimings[]` : mode 0 = 640×480p60 (252 MHz) répétition 2 (chemin
   inchangé) ; mode 1 = 720×480p60 (270 MHz) répétition 1, 350 lignes à partir de la
   ligne 65 ; mode 2 = 640×480p60 répétition 1, 256 lignes à partir de la ligne 112.
2. **Répétition verticale dynamique** : correctif PicoDVI
   (`firmware/patches/picodvi-vertical-repeat.diff`, appliqué par `FetchContent`
   `PATCH_COMMAND`, idempotent) — `inst->vertical_repeat` remplace la constante
   `DVI_VERTICAL_REPEAT` dans `dvi.c`.
3. Boucle d'encodage propre sur core1 (`_encode_loop`) au lieu de
   `dvi_scanbuf_main_16bpp` : lignes noires par `memcpy` d'un canal pré-encodé
   (`0x7fd00`), 1 bpp par `tmds_encode_1bpp` (une passe, copiée sur les canaux
   « allumés » ; `DVI_1BPP_BIT_REVERSE=1` pour l'ordre MSB en premier), 16 bpp
   doublé comme PicoDVI (modes 0 et 2 ; le 4 bpp est dépaqueté par la palette dans
   le callback).
4. **Changement de timing à chaud** (`DVIStopMode`) : `multicore_reset_core1`, arrêt
   des sérialiseurs et de l'horloge PWM, abandon/libération des 6 canaux DMA, SM et
   programme PIO, libération des tampons TMDS et des files, puis `DVIStart` avec la
   nouvelle horloge système. `HWClockChanged()` reprogramme le débit UART
   (`SERClockChanged`) et recalcule la cadence d'échantillonnage PWM
   (`SNDClockChanged`, désormais dérivée de `clock_get_hz(clk_sys)`).
5. Encre monochrome = entrée 1 de la palette (canal allumé si composante ≥ 128) :
   blanc par défaut, ambre `5,32` avec (255,176,0), vert (0,255,0).

## Risques identifiés (à mesurer sur carte)
| # | Risque | Vérification |
|---|---|---|
| R1 | À 270 MHz le PIO du bus 65C02 et les `nop` de core0 vont ~7 % plus vite (PHI2 ≈ 6,75 MHz au lieu de 6,3) : marges du W65C02S et de la RAM, et **le changement d'horloge a lieu pendant `DSPHandler` (65C02 gelé, PHI2 à l'arrêt)** | oscilloscope sur GPIO21 avant/après `5,9` ; boucle BASIC qui compte, stabilité 10 min |
| R2 | `set_sys_clock_khz` change `clk_peri` : UART (nxmit) et PWM son | nxmit après passage en mode 1 ; note jouée avant/après |
| R3 | Tension : 1,20 V suffit à 270 MHz d'après l'exemple `terminal` de PicoDVI | si écran noir/instable, essayer `VREG_VOLTAGE_1_25` |
| R4 | `DVIStopMode` : ordre d'arrêt (core1 reset avant abandon DMA) ; fuite si un tampon TMDS est dans un état non couvert | commuter 0→1→2→0 cent fois, surveiller `malloc` (pas de panique « TMDS buffer allocation failed ») |
| R5 | Budget core1 en mode 1 : ~1 530 cycles d'encodage + 3 `memcpy` de 1 440 octets par ligne sur ~8 570 cycles (270 MHz / 31,5 kHz) ≈ 35 % ; en mode 2 : 480 lignes dont 224 noires par `memcpy` | ligne rouge = ligne en retard (`late_scanline`) ; capture photo |
| R6 | Dépassement de `tmds_encode_1bpp` (blocs de 32 pixels, 720 = 22,5 blocs) : encodage dans `monoEncoded` avec marge puis copie de 360 mots | — (par construction) |
| R7 | Ordre des bits 1 bpp (`DVI_1BPP_BIT_REVERSE`) : texte en miroir par octet si faux | lecture du texte à l'écran |
| R8 | Moniteurs : 720×480p60 est un timing CEA (480p) — pixels non carrés, certains écrans l'affichent en 16:9 | essayer 2 moniteurs |

## Reste à faire dans F-52
- Attributs MDA par caractère (souligné, brillant, clignotant) : non commencés ;
  l'inverse est déjà obtenu par fond ≠ 0.
- Mesure F-50 réelle sur carte (protocole dans `F-50-budget-rendu.md`).
