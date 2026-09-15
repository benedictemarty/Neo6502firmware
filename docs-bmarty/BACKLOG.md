# Backlog du fork firmware

Stories transférées depuis les projets applicatifs (référence d'origine entre
parenthèses). Statuts : TODO · WIP · DONE · UPSTREAM (fusionné en amont).

## Épopée F0 — Base

| ID | P | Story | Origine | État |
|----|---|-------|---------|------|
| F-00 | P1 | Installer la chaîne et **compiler le firmware et l'émulateur tels quels**. Fait le 2026-09-15 pour la partie PC : 64tass 1.60, python3-git/pil, `make -C firmware prelim` (+ `make -C basic convert build`, `mkdir bin`), `make -C emulator emulator` → `bin/neo`. **Bug amont corrigé** (`feat/emulator-build-fix`, à proposer en PR). Firmware RP2040 (`make -C firmware build`) : à faire (Pico SDK à récupérer). | drive US-C1, oric2 US-05 | WIP |
| F-02 | P2 | Émulateur `neo` : échelle 1-4 pour tous les modes vidéo, fenêtre redimensionnée, plein écran (`fullscreen`, Ctrl+F11). | bmarty | DONE 2026-09-15 |
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
| F-15 | P3 | Mode 640×240 mono / texte 80 colonnes — **repris dans l'épopée F5 (modes vidéo)**. | question PO | → F5 |

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

## Épopée F5 — Modes vidéo et pages écran (`GFXSetMode` réel)

Faits (vérifiés) : signal DVI 640×480@60 (PicoDVI ; autres timings dans la
bibliothèque : 800×480, 800×600, 960×540 sur-cadencé), rendu ligne par ligne
par core1 (index → RGB565 → TMDS), framebuffer unique 320×240 × 8 bits
(76,8 Ko), ≈ 47 Ko de SRAM libres, `GFXSetMode()` ignore son paramètre.
Bilan mémoire 256 couleurs : 320×256 = 81,9 Ko ✓, 400×240 = 96 Ko ✓ (timing
800×480), 480×270 = 129,6 Ko limite, 640×240 = 153,6 Ko ✗ ; en 16 couleurs :
640×240 = 76,8 Ko ✓, 320×240 double tampon = 2 × 38,4 Ko ✓ ; en 2 couleurs :
640×480 = 38,4 Ko ✓ (double tampon ✓), Hercules 720×348 = 31,3 Ko ✓ (timing
800×480).

| ID | P | Story | Origine | État |
|----|---|-------|---------|------|
| F-50 | P1 | **Mesure du budget de rendu** : coût par ligne de core1 (conversion + TMDS) pour 320, 640 et 720 pixels, à 60 Hz, sur carte (F-00 partie ARM faite) — et à quelles fréquences (252 MHz, 372 MHz). Décide de la faisabilité de tout le reste. | question PO | WIP : analyse sur sources faite (`docs-bmarty/F-50-budget-rendu.md`) — 640×480 1 bpp et 2 bpp prouvés par PicoDVI, 16/256 couleurs pleine largeur exclus, 320×256 à trancher par mesure ; protocole de mesure sur carte rédigé, en attente d'une carte |
| F-51 | P1 | **Architecture multi-modes** : descripteur de mode (largeur, hauteur, bits/pixel, doublage H/V, timing DVI), `GFXSetMode(n)` effectif, console adaptée (largeur/hauteur en caractères, police 6×8 ou 8×8), émulateur `neo` et Phosphoneo (`neo_host`) alignés ; ADR-02. | | **DONE** 2026-09-15 (`feat/video-modes`) : `gfxModes[]`, `GFXSetMode` effectif, 5,9 / 5,10, accès pixel générique 1/4/8 bpp, console/pixel/ligne/rectangle/ellipse en modes compacts, sprites/tilemap/images refusés hors mode 0, `RNDModeSupported` (carte : mode 0 seul), rendu bpp-aware de `neo` ; démo `videomode1/2` identique neo ↔ Phosphoneo. Reste pour F-52/F-53 : rendu DVI et police 9×14 |
| F-52 | **P1** | **Mode Hercules** (décision PO 2026-09-15) : 720×350 × 1 bpp (31,5 Ko) dans le timing 720×480p60 (270 MHz), 350 lignes centrées ; **mode texte 80×25 en cellules 9×14** (police MDA/Hercules 9×14 : 8 colonnes de glyphe + 1 d'espacement, prolongée pour les caractères graphiques `$C0-$DF` comme sur MDA), attributs par caractère (normal, inverse, souligné, brillant, clignotant), curseur ; **mode graphique 720×348** (Hercules) sur le même tampon ; encre au choix (blanc, ambre, vert = même plan sur les canaux R/G/B choisis), fond noir ; double tampon possible. Console 2,x et éditeur adaptés (80 colonnes). | PO | **WIP** 2026-09-15 : police 8×14 en cellule 9×14 (vérifiée dans les 3 backends, dont le vrai firmware en co-sim), renderer DVI multi-modes + correctif PicoDVI + changement de timing à chaud **écrits et compilés, non exécutés sur carte** (`F-52-hercules.md`, risques R1–R8) ; attributs MDA faits (souligné, gras, clignotant, inverse — `mda.neo6502`) ; reste le passage sur carte |
| F-53 | **P1** | **Mode 320×256 × 16 couleurs** (décision PO) : 4 bpp (41 Ko), palette RGB565 (16 parmi 65 536), doublage horizontal, `VERTICAL_REPEAT=1` (256 lignes utiles + bandes) ; **mode texte 40×32 en 8×8** (encre/papier 16 couleurs par caractère) et variante 53×32 en 6×8 (police actuelle) ; sprites/blitter/primitives du groupe 5 opérant en 4 bpp ; **double tampon** (2 × 41 Ko) → F-55. Repli si la mesure F-50 refuse 480 encodages : 320×240 × 16 couleurs (texte 40×30). | PO | WIP : rendu DVI 4 bpp (dépaquetage palette + 16 bpp doublé, répétition 1, bandes noires par memcpy) écrit avec F-52, non exécuté sur carte ; primitives 4 bpp faites en F-51 ; images (5,7) et tilemaps (5,8) fonctionnent en modes compacts (tampon de ligne) ; pages faites (F-55) ; **sprites en modes 1/2 : XOR dans le tampon** (demande bmarty 2026-09-15 ; même moteur erase/redraw que le mode 0, sans couche ni priorité ; démo tortue `turtle1/2`) — reste : rendu carte à tester |
| F-54 | P2 | Mesure F-50 spécifique au 320×256 (`VERTICAL_REPEAT=1`, encodeur palette 4 bpp) ; décision 256 vs 240 lignes. | F-53 | TODO |
| F-55 | P2 | **Pages écran** pour F-53 (page visible / page de travail, bascule à la trame, « wait vsync ») et pour le mode Hercules. | PO (« plusieurs screens ») | **DONE** 2026-09-15 : `MAXGRAPHICSMEMORY` = 2 × 40 960 (2 pages en modes 1 et 2, 1 en mode 0), API 5,11 Set Draw Page / 5,12 Set Display Page, bascule à la trame sur carte (`pendingDisplayMemory`), immédiate dans `neo`/Phosphoneo ; attente par 5,37 ; démo `pages.neo6502` identique neo ↔ Phosphoneo. Rendu carte non testé (F-52) |
| F-56 | P3 | Variantes ultérieures : 640×256 × 1 bpp (encre au choix) et 640×256 × 8 couleurs (3 plans, 61 Ko) ; 400×240 × 8 bits en 800×480. | | TODO |
| F-57 | P3 | Police 9×14 et 8×8 chargeables (fichiers), jeu de caractères graphiques ; modes texte 80×43 (9×8) sur Hercules. | Télémon | TODO |

Modes v1 retenus : **mode 0** 320×240 × 256 couleurs (actuel) · **mode 1** Hercules
720×350 (texte 80×25 9×14) / 720×348 graphique · **mode 2** 320×256 × 16
couleurs (texte 40×32 8×8). Faits : 1 bpp pleine largeur prouvé par PicoDVI
(`terminal` 720×480) ; 4 bpp doublé = chemin palette existant ; l'inconnue est
le coût de 480 encodages/trame (F-54).

### Épopée F6 — Multitâche 6502 (question bmarty 2026-09-15 : « peut-on proposer du multithread préemptif ? »)

| ID | Prio | Story | Origine | État |
|---|---|---|---|---|
| F-60 | P3 | **Tick IRQ vers le 65C02** : fonction API « activer/désactiver un tick périodique » (50 Hz trame ou 100 Hz timer) sur IRQB (GPIO25, `wdc65C02cpu_set_irq()` existe mais **n'est appelé nulle part** aujourd'hui) + acquittement ; disponible dans `neo`/Phosphoneo (co-sim US-27 P4). | bmarty | **DONE** 2026-09-15 : API 1,12/1,13, IRQB relâchée à la lecture de `$FFFF` (pas d'acquittement), carte (timer matériel) écrite non testée, `neo` et Phosphoneo vérifiés (`irqtick.neo6502`) — `F-60-tick-irq.md` |
| F-61 | P3 | **Noyau préemptif 6502** (bibliothèque côté 6502, pas firmware) : contexte par tâche (A/X/Y/P/PC/S, tranche de page zéro et de pile), commutation sur le tick, créer/terminer/dormir/sémaphore ; **l'API `$FF00` n'est pas réentrante** → tick masqué pendant un appel API ou mutex API dans le noyau. Côté RP2040, pas de RTOS préemptif (core0 sert le bus cycle par cycle, core1 rend le DVI) : coopératif seulement. | bmarty | **DONE** 2026-09-15 dans le noyau 6502 (`kernel/rtos.asm`, 10 vecteurs `$FFC1-$FFDC`) : 4 tâches, tourniquet, sommeil, sémaphores, verrou API, WAI si rien de prêt ; démos `rtos`/`rtos_idle` identiques neo ↔ Phosphoneo — `F-61-rtos.md` |

### Épopée F7 — Modes vidéo « historiques » et interception d'adresses (sélecteur d'OS, Neo6502kbd EPIC-02)

| ID | Prio | Story | Origine | État |
|---|---|---|---|---|
| F-70 | P3 | Rendu de la page texte Apple II (`$0400-$07FF`, entrelacée, 40×24, police Apple) depuis la RAM 6502, comme mode vidéo 5,9. | EPIC-02 | TODO |
| F-71 | P3 | Rendu TEXT (`$BB80`, attributs série) et HIRES (`$A000`, 240×200) Oric. | EPIC-02 | TODO |
| F-72 | P3 | Rendu MODE 7 télétexte BBC (`$7C00`, SAA5050) et MODE 0-6. | EPIC-02 / Neo6502bbc | TODO |
| F-73 | P3 | Interception d'adresses dans la boucle bus (soft switches Apple `$C0xx`, VIA Oric `$0300`) : table d'adresses → gestionnaire côté RP2040 ; coût mesuré (cf. R9). | EPIC-02 | TODO |

### Épopée F8 — Multi-boot RP2040 (sélecteur d'OS : firmware Neo ⇄ images reload-emulator)

| ID | Prio | Story | Origine | État |
|---|---|---|---|---|
| F-80 | P2 | **Étude** : plusieurs images firmware en flash (Neo ≈ 190 Ko, reload BBC/Oric/Apple //e avec ROM et images disque, 2 Mo au total), étage de démarrage qui lit le choix (registre scratch du watchdog / secteur de flash) et saute à l'image ; relocalisation des images (éditeur de liens Pico SDK, `boot2`) ; comportement PicoDVI/TinyUSB après saut. | Neo6502kbd EPIC-02, reload fork `bbc` | TODO |
| F-81 | P2 | API **1,14 Reboot Image** : P0 = image (0 = Neo) → mémorise le choix et redémarre (watchdog) ; le Télémon `O` liste les images. | EPIC-02 | TODO |
| F-82 | P3 | Retour : une touche au reset (ou un délai) pour revenir au firmware Neo depuis une image reload (dans reload : `hid_app` F12 ?). | EPIC-02 | TODO |

Dépendances : F-50 avant tout ; F-51 est le socle ; F-11 (blit 2 bpp) pour
F-53 ; la Toolbox (F4) s'appuie sur F-51/F-55 pour les surfaces hors écran.

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
