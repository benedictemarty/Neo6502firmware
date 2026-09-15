# CLAUDE.md — Neo6502firmware (fork bmarty, départ 2026-09-15)

## Projet

Fork du firmware officiel **neo6502-firmware** (Paul Robson et contributeurs,
MIT ; remote `upstream`, base `v1.0.0-14-gdc70908`). Il regroupe **toutes les
évolutions du firmware** dont ont besoin les projets Neo6502 de bmarty
(Neo6502kbd/Télémon, Neo6502drive, Neo6502bbc, Neo6502oric2, Neo6502scumm,
Neo6502civ), afin qu'aucun projet applicatif ne porte de fork privé.

Objectif : **chaque évolution est conçue pour être proposée en amont** (pull
request), petite, documentée dans `dispatch.config`/`api.tex`, testée dans
l'émulateur `neo` (qui partage `firmware/common/`).

## Règles

1. **Branche `bmarty/main`** = intégration ; une branche par fonctionnalité
   (`feat/irq-vsync`, `feat/blit-2bpp`, …) rebasée sur `upstream/main`.
2. **Rien d'inventé** : comportement du firmware vérifié dans le code avant
   modification ; toute API ajoutée est documentée dans le `groupN.inc`
   concerné (`DOCUMENTATION`) et reportée dans `docs-bmarty/API-DELTA.md`.
3. **L'émulateur d'abord** : toute modification de `firmware/common/` est
   validée dans `neo` avant flash ; une modification RP2040-only
   (`firmware/sources/`) est testée sur carte et consignée.
4. **Timing** : la boucle mémoire (`processor_pio.cpp`, PIO) et le rendu DVI
   sont temps-critiques ; toute modification y est mesurée (cycles, stabilité
   à 6,25 MHz) avant merge.
5. Rétrocompatibilité : les programmes existants (NeoBASIC, jeux Prophet)
   doivent continuer de fonctionner ; nouvelles fonctions = nouveaux numéros.
6. Agile : `docs-bmarty/BACKLOG.md`, `docs-bmarty/CHANGELOG.md` à chaque
   modification ; commits en français ; auteur `bmarty <bmarty@mailo.com>` ;
   jamais de Co-Authored-By ni de mention d'IA. Le `CHANGELOG`/`README`
   amont ne sont modifiés que dans les PR.

## Chaîne de build (à installer, US-00)

`arm-none-eabi-gcc` + newlib, `cmake`, `64tass`, `python3` (pillow,
gitpython), `libsdl2-dev` ; dépendances Pico SDK / TinyUSB / PicoDVI /
pico-fatfs via variables d'environnement (voir `README.md` amont).
`make -C emulator` pour l'émulateur, `make -C firmware build STORAGE=sd|usb`.
