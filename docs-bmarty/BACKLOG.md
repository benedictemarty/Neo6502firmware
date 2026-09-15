# Backlog du fork firmware

Stories transférées depuis les projets applicatifs (référence d'origine entre
parenthèses). Statuts : TODO · WIP · DONE · UPSTREAM (fusionné en amont).

## Épopée F0 — Base

| ID | P | Story | Origine | État |
|----|---|-------|---------|------|
| F-00 | P1 | Installer la chaîne et **compiler le firmware et l'émulateur tels quels**. Fait le 2026-09-15 pour la partie PC : 64tass 1.60, python3-git/pil, `make -C firmware prelim` (+ `make -C basic convert build`, `mkdir bin`), `make -C emulator emulator` → `bin/neo`. **Bug amont corrigé** (`feat/emulator-build-fix`, à proposer en PR). Firmware RP2040 (`make -C firmware build`) : à faire (Pico SDK à récupérer). | drive US-C1, oric2 US-05 | WIP |
| F-01 | P1 | **Options de test sur l'émulateur `neo`** (voie 1, décision 2026-09-15) : `--headless`, `--cycles N`, `--screenshot-at C:FILE` (.ppm/.bmp), `--screenshot-text-at` (console 53×30), `--type-keys C:TEXT` (injection dans la file clavier du firmware), `--trace FILE`, `--dump-ram-when A:V:FILE`, `--poke-at` ; sortie déterministe. Le golden model complet est le projet à part **Phosphoneo**. | scumm US-03, civ US-13, bbc US-04 | DONE 2026-09-15 (`feat/emulator-test-options`, candidat PR amont ; `--dump-ram-when`/`--poke-at`/`--trace FILE` non portés, `trace` existant) |
| F-02 | P2 | Corriger la documentation là où elle contredit le code (ex. 2,2 Console Status) — PR documentaire. | kbd notes | TODO |

## Épopée F1 — Fonctions manquantes (petites, candidates amont)

| ID | P | Story | Origine | État |
|----|---|-------|---------|------|
| F-10 | P1 | **IRQ périodique / vsync** : le RP2040 déclenche l'IRQ du 65C02 (broche IRQB, déjà câblée) à 50/60 Hz ou sur trame, activable par API (nouvelle fonction groupe 1), avec acquittement ; vecteur `$FFFE` laissé au programme. | bbc US-16, oric2 | TODO |
| F-11 | P1 | **Blitter : format source 2 bpp** (4 pixels/octet) dans 12,3, plus option de doublage horizontal ; utile MODE 5 BBC, C64, CPC. | bbc US-11 | TODO |
| F-12 | P2 | **Son : volume instantané par canal** et/ou enveloppe simple (attaque/relâchement) en complément de 8,7. | bbc US-14 | TODO |
| F-13 | P2 | **Hôte USB CDC-ACM** : `CFG_TUH_CDC 1`, callbacks, tampon ; exposition par routage des fonctions UART 10,13-10,18 ou nouvelles fonctions ≥ 10,19 ; maquette dans l'émulateur (pty/TCP). | drive US-C2..C4 | TODO |
| F-14 | P2 | **Date/heure** : API lecture/écriture d'une horloge (RTC PCF8563 sur I2C si présent, sinon compteur), horodatage FAT. | manques | TODO |
| F-15 | P3 | **Mode vidéo 640×240 monochrome / texte 80 colonnes** (framebuffer 1 bpp de 19,2 Ko remplaçant le 320×240 à la demande). | question PO | TODO |

## Épopée F2 — Mémoire et bus (mesures de timing obligatoires)

| ID | P | Story | Origine | État |
|----|---|-------|---------|------|
| F-20 | P2 | **Fenêtre d'adresses externe** : plage où le RP2040 ne pilote pas D0-D7 en lecture (périphériques lisibles sur BUS1), gestion `RDY`. | drive US-B3 | TODO |
| F-21 | P3 | **Banques ROM en flash** commutées par registre (`$FF0x`), pour ROM étendues / cartouches. | oric2 US-22, question banking | TODO |
| F-22 | P3 | Cadence 65C02 réglable à chaud (1 MHz compat / 6,25 MHz), si le PIO le permet. | oric2 US-13 | TODO |

## Épopée F3 — Modes machine (gros, décision par ADR)

| ID | P | Story | Origine | État |
|----|---|-------|---------|------|
| F-30 | P3 | **Mode Oric** dans le firmware (ULA/VIA/AY/Microdisc) — ou fork de reload : ADR-01 de Neo6502oric2. | oric2 | TODO |
| F-31 | P3 | **Mode BBC** (6845/ULA, VIA ×2, SN76489, 8271/1770, Master 128 ?) : épopée 3 de Neo6502bbc. | bbc US-30..36 | TODO |

## Épopée F4 — Toolbox (ADR-01, `docs-bmarty/adr/0001-toolbox.md`)

| ID | P | Story | Origine | État |
|----|---|-------|---------|------|
| F-40 | P2 | **Ratifier l'ADR-01** (numérotation ≥ 32, conventions de structures et d'erreurs, ordre) et créer le squelette `config/toolbox/group32_quickdraw.inc` + `sources/interface/toolbox/`. | Télémon, portages | TODO |
| F-41 | P2 | **QuickDraw (32)** : port courant + clipping, rectangles, lignes, motifs, `CopyBits` sur le blitter, fontes proportionnelles (format + outil PC de conversion), `DrawString/TextWidth` ; captures golden Phosphoneo. | | TODO |
| F-42 | P2 | **Event Manager (33)** : file unifiée clavier/souris/fenêtre/timer, `GetNextEvent`, `WaitNextEvent` (timeout ; IRQ F-10 quand disponible). | | TODO |
| F-43 | P2 | **Window Manager (34)** : fenêtres, ordre Z, cadres dessinés par le firmware, invalidation/`update`, drag/size, `FindWindow`. | | TODO |
| F-44 | P3 | **Menu (35) et Control (36) Managers**. | | TODO |
| F-45 | P3 | **Dialog Manager (37)**, descriptions en RAM 6502. | | TODO |
| F-46 | P3 | **Resource/Font (38) et Memory RP2040 (39)** : ressources sur SD, poignées hors des 64 Ko. | | TODO |
| F-47 | P2 | **Vitrine** : le Télémon (Neo6502kbd) utilise fenêtres + menus + événements ; documentation `api.tex` complète ; proposition amont. | | TODO |

## Politique amont
Ordre de proposition : F-01 → F-02 → F-11 → F-10 → F-13 → F-14. Les épopées
F2/F3 restent dans le fork tant qu'elles ne sont pas stabilisées et mesurées.
