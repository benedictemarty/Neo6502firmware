# Backlog — Trinity (firmware de référence Neo6502, bmarty)

Décision bmarty 2026-09-19 : Trinity est **l'unique firmware de référence** ; le fork `bmarty/main` (toolbox, banques,
modes vidéo, Latin-1, R22) est archivé (`archive/bmarty-main-2026-09-19`). Une fonctionnalité n'entre dans Trinity
que validée **sur carte**. Règles : rien d'inventé, petits commits en français (`bmarty <bmarty@mailo.com>`, jamais de
Co-Authored-By), CHANGELOG à chaque modification, tag `trinity-vX.Y.Z` à chaque livraison, UF2 dans `~/neo-carte/`.

| ID | P | Story | État |
|---|---|---|---|
| T-01 | P1 | Modem USB CDC (groupe 14) + routage UART 10,19 (F-90/F-93 du fork). | **DONE** 0.0.1, validé carte (modem reconnu, netinfo, IP) |
| T-02 | P1 | TinyUSB 0.21 : clé USB derrière le hub (1 ms/secteur au lieu de 2,5 s). | **DONE** 0.1.0, validé carte |
| T-03 | P1 | BASIC découplé : `neobasic.bin` sur le stockage prioritaire (projet Neo6502Basic). | **DONE** 0.2.0, validé carte |
| T-04 | P1 | Menu de démarrage `boot/` (.neo/.bin) et `boot/auto.txt` (Échap = menu). | **DONE** 0.2.0, validé carte |
| T-05 | P1 | Lecture par blocs CDC : servir l'hôte USB pendant l'attente (R16). | **DONE** 0.2.1, validé carte (ProphetGui) |
| T-06 | P1 | ProphetGui de bout en bout sur carte (F-94) : catalogue, fiche, téléchargement. | **catalogue OK** 2026-09-19 (Trinity 0.2.1, modem Pico W, 3617.fr) ; fiche et téléchargement à confirmer |
| T-07 | P2 | Modes vidéo (Hercules 720×350, 320×256) : **écran noir sur carte** avec les commits du fork (`trinity-video`) ; isoler timing DVI à chaud / correctif PicoDVI / renderer, un build par suspect. | TODO |
| T-08 | P2 | Primitives BASIC `at`/`atline$`/`atwait` (Neo6502Basic) : livrées par `neobasic.bin`, à consigner ici par version. | WIP (Neo6502Basic) |
| T-09 | P3 | Reprises du fork, une par une et validées carte : 3,27 File Read Paged, volumes 3,24-3,26, locale FR/Latin-1, tick/IRQ trame, banques, toolbox. | TODO — sur demande |
