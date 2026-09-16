# Sprints du fork

## Sprint 1 — 2026-09-15 — « Émulateur et golden » (F-00 PC, F-01) 
Compilation de la chaîne PC, options de test headless de `neo`, différentiel Phosphoneo.

## Sprint 2 — 2026-09-15 — « Modes vidéo » (F5 : F-50..F-55)
Analyse du budget PicoDVI, architecture multi-modes (ADR-02), Hercules 720×350 avec
police 9×14 et attributs MDA, 320×256×16, pages écran, sprites XOR, tilemaps/images en
modes compacts, `neo` multi-modes (échelle, plein écran). Renderer DVI, correctif
PicoDVI et changement d'horloge **non exécutés sur carte** (R1–R8).

## Sprint 3 — 2026-09-15 — « Système » (F6 : F-60, F-61 ; F8 : F-80..F-82)
Tick IRQ (1,12/1,13, IRQB relâchée à la lecture de `$FFFF`), ordonnanceur préemptif
dans le noyau 6502 (10 vecteurs), multi-boot RP2040 (sélecteur, slots, `mkimage.py`,
API 1,14/1,15, images reload liées par slot, touche Pause). Vérifié dans `neo`,
Phosphoneo et sur libemul (co-sim, sonde multi-boot) ; timer et boucle bus **non
exécutés sur carte** (R9–R14).

## Sprint 4 — 2026-09-15/16 — « Télécom » (F9 : F-90..F-93)
Série USB CDC (groupe 14), routage UART → CDC (10,19), écho console (2,20), étude
F-91 close ; F-83 (install `.uf2` depuis USB) conçue. Vérifié en co-sim avec le
Pico W réel ; carte non testée (R15–R21).

## À planifier
F10 (`N:` et volumes, F-100..F-103, quand US-T3 du modem existe) ; F-83 ;
Validation sur carte (dès qu'un Neo6502 est disponible : protocoles F-50, F-52, F-60,
F-80) ; Toolbox F-40 (ADR-01 à ratifier) ; F7 modes historiques (optionnel).
