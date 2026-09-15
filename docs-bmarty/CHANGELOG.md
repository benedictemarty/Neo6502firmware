# Changelog (fork bmarty)

## [Unreleased]
- 2026-09-15 : F-52 — attributs MDA en mode Hercules (souligné, gras, clignotant, inverse) portés par les quartets encre/papier ; `CONBlinkSync` appelé par `DSPSync` (carte, Phosphoneo) et `HWSync` (émulateur).
- 2026-09-15 : F-53 — sprites en modes 1 et 2 (XOR pixel à pixel dans le tampon, `SPRPHYDraw` générique, `SPRReset` efface les sprites dessinés, `SPRScreenCleared` après 2,12) ; démo tortue.
- 2026-09-15 : émulateur `neo` — l'échelle `scale=1..4` s'applique à tous les modes
  vidéo (la fenêtre s'agrandit pour 720×350), argument `fullscreen` et bascule
  **Ctrl+F11** (plein écran bureau, échelle entière maximale). Demande bmarty.
- 2026-09-15 : F-55 pages écran — `MAXGRAPHICSMEMORY` 76 800 → 81 920 (2 pages en
  modes 1/2), `gMode.pageSize/pageCount/drawPage/displayPage/displayMemory`, API
  **5,11 Set Draw Page** et **5,12 Set Display Page**, `RNDSetDisplayPage` (carte :
  bascule au début de trame ; émulateurs : immédiate), `GFXReadDisplayPixelRaw` ;
  effacement limité à la page de dessin. F-53 : images (5,7) et tilemaps (5,8)
  disponibles en modes compacts.
- 2026-09-15 : F-52 (partiel) — police 8×14 (`font_8x14.h`, script `vga14.py`) pour la
  console 80×25 en 9×14 ; renderer DVI multi-modes (`dvi_320x240x256.cpp` : timings
  par mode, boucle d'encodage core1, 1 bpp `tmds_encode_1bpp`, bandes noires,
  arrêt/redémarrage du DVI au changement de timing, `HWClockChanged` pour UART et
  son) ; correctif PicoDVI « vertical repeat dynamique » appliqué par FetchContent ;
  `DVI_1BPP_BIT_REVERSE=1`. **Non exécuté sur carte** : fiche `F-52-hercules.md`.
- 2026-09-15 : épopée F6 multitâche 6502 (F-60 tick IRQ, F-61 noyau préemptif) inscrite, avec les contraintes constatées (IRQB jamais levée par le firmware, API non réentrante).
- 2026-09-15 : F-51 — architecture multi-modes (ADR-02) : descripteur de mode et
  table (0 : 320×240×256 inchangé ; 1 : Hercules 720×350 1 bpp, texte 80×25 ;
  2 : 320×256 4 bpp, texte 40×32), `GFXSetMode(n)` effectif, **API 5,9 Set
  Graphics Mode** et **5,10 Get Graphics Mode**, accès pixel générique,
  console/primitives en modes compacts, sprites/tilemap/images limités au mode 0,
  `RNDModeSupported` par hôte (carte : mode 0 tant que F-52/F-53 ne sont pas
  faits), capture PPM et fenêtre de `neo` à la taille du mode.
- 2026-09-15 : F5 recadrée par le PO — modes v1 : Hercules 720×350 (texte 80×25 en
  9×14, graphique 720×348) et 320×256 × 16 couleurs (texte 40×32 en 8×8), pages
  écran ; 640×256 et 400×240 en variantes ultérieures.
- 2026-09-15 : F-50 — analyse du budget de rendu (PicoDVI : encodeurs, exemples,
  timings), verdict par mode, protocole de mesure sur carte.
- 2026-09-15 : épopée F5 « modes vidéo et pages écran » (F-50..F-57) avec le bilan
  mémoire par mode ; F-15 absorbée.
- 2026-09-15 : ADR-01 Toolbox (groupes 32–39 : QuickDraw, Event, Window, Menu,
  Control, Dialog, Resource/Font, Memory RP2040) et épopée F4 (F-40..F-47).
- 2026-09-15 : F-01 — options de test headless de l'émulateur (`cycles:`, `shot:`,
  `text:`, `keys:`), validées par le différentiel Phosphoneo (5 cas identiques).
- 2026-09-15 : F-00 partie PC — prelim + NeoBASIC + émulateur compilés ; correctif
  `__time_critical_func` (branche `feat/emulator-build-fix`, candidat PR amont).
- 2026-09-15 : F-01 précisée (options de test type Phosphoric sur `neo`) ; golden
  model complet confié au projet Phosphoneo.
- 2026-09-15 : création du fork (`upstream` = neo6502-firmware `v1.0.0-14-gdc70908`),
  branche `bmarty/main`, CLAUDE.md, backlog F-00..F-31 regroupant les stories
  firmware des projets Neo6502kbd, Neo6502drive, Neo6502bbc, Neo6502oric2,
  Neo6502scumm, Neo6502civ.
