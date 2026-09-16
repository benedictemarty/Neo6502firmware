# Changelog (fork bmarty)

## [Unreleased]
- 2026-09-16 : épopée F10 (F-100..F-103) — périphérique réseau `N:` et API volumes
  (`3,24 Volume List` / `3,25 Volume Select` proposés, montage SD + USB), d'après le
  mémo du projet Neo6502Prophet (`MEMO-PROPHET-N-DEVICE-2026-09-16.md`) ; constat
  vérifié : stockage unique à la compilation, 3,24-3,31 libres. Pas de code.
- 2026-09-16 : F-93 — routage UART UEXT (10,13-10,18) vers un modem USB CDC ;
  nouvelle fonction 10,19 (0 matériel / 1 CDC / 2 AUTO, défaut AUTO) ;
  `cdcserial.cpp` (UARTRoute*/UARTR*), `group10_uext.inc`. netconfig/prophet
  peuvent parler à un modem Wi-Fi USB sans modification. Vérifié en co-sim
  (Phosphoneo, vrai code firmware) avec le Pico W réel via un programme
  n'utilisant que l'API UART.
- 2026-09-15 : image multi-boot à 4 slots : + BBC Master 128 (`bbc_master`, slot 3, 359 Ko) → 1 028 Ko ; sélection du slot 3 vérifiée sur libemul.
- 2026-09-15 : `make -C multiboot image` produit `neo6502-multi.uf2` (Neo 194 Ko + reload BBC 279 Ko + Oric 187 Ko, 669 Ko) ; `make -C multiboot check` (libemul).
- 2026-09-15 : F-92 — API 2,20 Console Debug Echo : l'affichage console est recopié sur le port de débogage (UART carte / stderr émulateurs) pour capturer l'écran d'une vraie carte depuis le PC ; F-91 (Neo en périphérique CDC) : étude close — un seul contrôleur USB, le lien PC passe par l'UART.
- 2026-09-15 : émulateur `neo` — option de test `mouse:C:X,Y,B` (position et boutons planifiés), pour le différentiel Phosphoneo.
- 2026-09-15 : F-90 vérifiée avec le vrai Pico W modem (Neo6502drive) sur `/dev/ttyACM0`, en comportemental et sur le vrai firmware en co-sim.
- 2026-09-15 : F-90 — série USB CDC (modems, adaptateurs USB-série) : groupe 14 (statut, lecture/écriture octet et bloc, line coding), `hardware/cdc.cpp` sur TinyUSB `cdc_host` (`CFG_TUH_CDC 2`, FTDI, CP210x), hôte PC `cdc_host_tty.cpp` (`NEO_CDC_TTY`) ; vérifié sur le vrai firmware en co-sim avec un faux modem Hayes (Phosphoneo `test-cdc`), non exécuté sur carte.
- 2026-09-15 : F-60 — `IRQTickCallback` et `irqTickHz` rendus globaux pour la co-sim Phosphoneo ; tick et ordonnanceur vérifiés sur le firmware ARM réel (hors timer/GPIO, HLE).
- 2026-09-15 : F-82 (retour depuis reload : touche Pause) et liaison des images reload par slot faites dans le fork reload ; image multi-boot Neo + BBC + Oric assemblée (639 Ko), sélection des slots vérifiée sur libemul.
- 2026-09-15 : multi-boot — plus d'annuaire : présence et nom des images lus dans la flash elle-même (table de vecteurs + `binary_info`), le menu suit ce qui est flashé (demande bmarty) ; `pico_set_program_name(firmware "Neo6502")`.
- 2026-09-15 : F-80/F-81 multi-boot RP2040 — `multiboot/` (sélecteur `neoboot` 8,9 Ko, scripts de liaison par slot, `mkimage.py`), `make -C firmware build-multiboot` (slot 0), API 1,14 Reboot Image / 1,15 Get Image Name (`hardware/multiboot.cpp`, annuaire en flash) ; vérifié sur libemul via Phosphoneo (`test-multiboot`), non exécuté sur carte (`F-80-multiboot.md`).
- 2026-09-15 : épopée F8 multi-boot RP2040 (F-80..F-82) pour lancer les images reload-emulator (BBC, Oric, Apple //e) depuis le Télémon.
- 2026-09-15 : épopée F7 (modes vidéo historiques Apple II / Oric / BBC rendus depuis la RAM 6502, interception d'adresses) inscrite pour le sélecteur d'OS (Neo6502kbd EPIC-02).
- 2026-09-15 : F-61 — ordonnanceur préemptif dans le noyau 6502 (`kernel/rtos.asm` + `rtos_data.asm`) : 4 tâches, tourniquet sur le tick F-60, `KTaskInit/Create/Yield/Sleep/Exit/Lock/Unlock/Ticks`, `KSemWait/Signal`, WAI + acquittement si rien de prêt ; IRQ vectorisée sur `KIrqHandler` ; émulateur `neo` : opcode WAI et relâchement d'IRQ sur lecture de `$FFFF`.
- 2026-09-15 : F-60 — tick d'interruption vers le 65C02 : API 1,12 Set Interrupt Tick / 1,13 Get ; module commun `irq.cpp` ; carte : timer matériel sur core0 + relâchement d'IRQB à la lecture de `$FFFF` dans la boucle bus (3 `nop` retirés, **non testé sur carte**, R9/R10) ; `neo` : `CPUTriggerIRQ` en attente tant que I=1 ; fiche `F-60-tick-irq.md`.
- 2026-09-15 : **jalon F5** — `feat/video-modes` fusionnée dans `bmarty/main` (`dc655ac`) : modes 1 et 2, pages, sprites XOR, attributs MDA, émulateur multi-modes ; pas d'étiquette git (la bannière du firmware vient de `git describe`, réservé aux versions amont). Rendu carte à valider (F-52-hercules.md).
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
