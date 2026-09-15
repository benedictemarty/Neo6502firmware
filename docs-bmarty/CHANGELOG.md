# Changelog (fork bmarty)

## [Unreleased]
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
