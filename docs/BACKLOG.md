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
| T-07 | P2 | Mode vidéo 1 Hercules 720×350 (F-51/52/53/55 du fork, sans le mode 2). | **DONE** 0.3.0, validé carte (10 builds de bissection : IRQ, encodeur, core 1, DMA) |
| T-08 | P2 | Primitives BASIC `at`/`atline$`/`atwait` (Neo6502Basic) : livrées par `neobasic.bin`, à consigner ici par version. | WIP (Neo6502Basic) |
| T-09 | P3 | Reprises du fork, une par une et validées carte : 3,27 File Read Paged, volumes 3,24-3,26, locale FR/Latin-1, tick/IRQ trame, banques, toolbox. | TODO — sur demande |
| T-10 | P2 | **NeoDOS** (`docs/MEMO-NEODOS-2026-09-19.md`) : valider sur carte `boot/neodos.neo` + `auto.txt` ; reprises demandées : volumes 3,24-26 (T-09), date/heure 1,20-21 + horodatage FAT, dates dans 3,18/3,16 (ou `3,28 Stat Extended`), `..` et casse dans l'émulateur `neo`, contrat `$FF08` documenté. | TODO |
| T-11 | P2 | **`1,3` et `boot/auto.txt` à chaque appel** (demande NeoDOS 0.8.2, `docs/MEMO-NEODOS-2026-09-19.md`) : aujourd'hui `BOOTLoadChoice()` n'agit qu'au premier `1,3` ; un programme qui se termine par `1,3` + `jmp (0)` revient donc à NeoBASIC, jamais au programme de démarrage. Proposition : `1,3` recharge le choix de `boot/auto.txt` à chaque appel (NeoDOS revient comme NeoBASIC revient, sans stub en `$0100`) ; `auto.txt` absent ou Échap = NeoBASIC. À trancher : sémantique de `1,3` pour les programmes qui comptent revenir à BASIC (option : nouvelle fonction `1,22 Reload Boot Program`, `1,3` inchangé). Test : `neo` + `boot/`. | TODO |

