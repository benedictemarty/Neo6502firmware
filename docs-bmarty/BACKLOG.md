# Backlog du fork firmware

Stories transférées depuis les projets applicatifs (référence d'origine entre
parenthèses). Statuts : TODO · WIP · DONE · UPSTREAM (fusionné en amont).

## Épopée F0 — Base

| ID | P | Story | Origine | État |
|----|---|-------|---------|------|
| F-00 | P1 | Installer la chaîne et **compiler le firmware et l'émulateur tels quels**. Partie PC le 2026-09-15 (64tass 1.60, `make -C firmware prelim`, `bin/neo`, bug amont corrigé `feat/emulator-build-fix`). Partie RP2040 le 2026-09-16 : Pico SDK **1.5.1** (`~/pico-sdk-1.5.1`), `make -C firmware build STORAGE=USB|SDCARD`, `build-multiboot`, image 4 slots — `docs-bmarty/BUILD.md`. | drive US-C1, oric2 US-05 | **DONE** 2026-09-16 (compilation ; flash sur carte à faire) |
| F-02 | P2 | Émulateur `neo` : échelle 1-4 pour tous les modes vidéo, fenêtre redimensionnée, plein écran (`fullscreen`, Ctrl+F11). | bmarty | DONE 2026-09-15 |
| F-01 | P1 | **Options de test sur l'émulateur `neo`** (voie 1, décision 2026-09-15) : `--headless`, `--cycles N`, `--screenshot-at C:FILE` (.ppm/.bmp), `--screenshot-text-at` (console 53×30), `--type-keys C:TEXT` (injection dans la file clavier du firmware), `--trace FILE`, `--dump-ram-when A:V:FILE`, `--poke-at` ; sortie déterministe. Le golden model complet est le projet à part **Phosphoneo**. | scumm US-03, civ US-13, bbc US-04 | DONE 2026-09-15 (`feat/emulator-test-options`, candidat PR amont ; `--dump-ram-when`/`--poke-at`/`--trace FILE` non portés, `trace` existant) |
| F-02 | P2 | Corriger la documentation là où elle contredit le code (ex. 2,2 Console Status). | kbd notes | **DONE** 2026-09-16 (2,2 ; 3,16 vérifié cohérent) — d'autres cas à corriger au fil de l'eau |

## Épopée F1 — Fonctions manquantes (petites, candidates amont)

| ID | P | Story | Origine | État |
|----|---|-------|---------|------|
| F-10 | P1 | **IRQ périodique / vsync** : le RP2040 déclenche l'IRQ du 65C02 (broche IRQB, déjà câblée) à 50/60 Hz ou sur trame, activable par API (nouvelle fonction groupe 1), avec acquittement ; vecteur `$FFFE` laissé au programme. | bbc US-16, oric2 | TODO |
| F-11 | P1 | **Blitter : format source 2 bpp** (4 pixels/octet) dans 12,3, plus option de doublage horizontal ; utile MODE 5 BBC, C64, CPC. | bbc US-11 | **DONE** 2026-09-17 (`feat/blit-2bpp`) : format 5 linéaire MSB en premier (12,3 + clipping 12,4), doublage = bit 0 de l'octet 3 de la source (12,3 seul, ≤ 360 valeurs) ; `blit2bpp.asm` identique neo / Phosphoneo / co-sim ; non testé sur carte. Entrelacement BBC/C64 : à convertir côté programme (pas de format dédié) |
| F-12 | P2 | **Son : volume instantané par canal** et/ou enveloppe simple (attaque/relâchement) en complément de 8,7. | bbc US-14 | TODO |
| F-13 | P2 | **Hôte USB CDC-ACM** : `CFG_TUH_CDC 1`, callbacks, tampon ; exposition par routage des fonctions UART 10,13-10,18 ou nouvelles fonctions ≥ 10,19 ; maquette dans l'émulateur (pty/TCP). | drive US-C2..C4 | **DONE** 2026-09-15 = F-90 (nouveau groupe 14 plutôt que le routage UART ; maquette pty `NEO_CDC_TTY`) |
| F-14 | P2 | **Date/heure** : API lecture/écriture d'une horloge (RTC PCF8563 sur I2C si présent, sinon compteur), horodatage FAT. | manques | TODO |
| F-15 | P3 | Mode 640×240 mono / texte 80 colonnes — **repris dans l'épopée F5 (modes vidéo)**. | question PO | → F5 |
| F-16 | P2 | **Lecture fichier directe en VRAM / RAM graphique** (nouvelle fonction `3,27 File Read Paged`, 3,27-3,31 libres) : P0 = canal, P1 = page (`00` RAM 6502, `80`/`81` VRAM, `81` au-delà de la ligne 204, `90` RAM graphique — mêmes pages que le blitter, `_BLTGetRealAddress`), P2,3 = adresse 16 bits dans la page, P4,5 = taille demandée → taille lue ; erreur si la plage sort de la page ; position du fichier avancée comme 3,8. Constat vérifié : 3,8 avec `$FFFF` lit déjà en RAM graphique mais **toujours à l'offset 0** et jamais en VRAM. Cas d'usage Neo6502civ (`nf_show_color`, image `.CLR` 8 bpp 240×200) : aujourd'hui 200 × (3,8 → tampon 6502 → 12,2) ; avec 3,27 : lecture directe ligne par ligne, voire en 2 appels (65 536 o page `80` puis le reste page `81`) si l'image est stockée au pas de 320 o ; et chargement de tuiles/sprites à un offset choisi de la RAM graphique (« chargement à la demande »). Ne concerne pas les images 1 bit `.BIN` (repaquetage 12,3 toujours nécessaire). Émulateur `neo` puis carte (mesure du gain vs 200 blits, cf. US-41 civ). | civ US-35/US-41 | **DONE** 2026-09-17 (`feat/file-read-paged`) : 3,27 dans `neo`, Phosphoneo et co-sim du vrai firmware (`readpaged.asm`), USB/SDCARD compilés, **non testé sur carte** (mesure vs 200 blits à faire, US-41) |

## Épopée F2 — Mémoire et bus (mesures de timing obligatoires)

| ID | P | Story | Origine | État |
|----|---|-------|---------|------|
| F-20 | P2 | **Fenêtre d'adresses externe** : plage où le RP2040 ne pilote pas D0-D7 en lecture (périphériques lisibles sur BUS1), gestion `RDY`. | drive US-B3 | TODO |
| F-21 | P3 | **Banques ROM en flash** commutées par registre (`$FF0x`), pour ROM étendues / cartouches — cas « lecture seule » de F-23 (même mécanisme de copie, source en flash). | oric2 US-22, question banking | TODO → F-23 |
| F-22 | P3 | Cadence 65C02 réglable à chaud (1 MHz compat / 6,25 MHz), si le PIO le permet. | oric2 US-13 | TODO |
| F-23 | P2 | **Banques mémoire 6502** (demande bmarty 2026-09-17 : « prévois de la banque dans le firmware »). Faits vérifiés (`processor_pio.cpp`) : chaque lecture du 65C02 est servie par `cpuMemory[adresse]` sur core0, chemin temps-critique (11 nop de marge après F-60), donc **pas de test d'adresse par lecture** ; pendant un appel API le 65C02 est bloqué sur sa lecture de `$FF00` (RDY tenu par le PIO), donc une copie mémoire côté RP2040 pendant l'appel est invisible pour lui. Conception retenue à étudier : **commutation par copie** — fenêtre de 16 Ko dans la RAM 6502 (adresse choisie, ex. `$4000-$7FFF` ou `$A000-$DFFF`), banques stockées hors RAM 6502 ; `1,18 Select Bank` (P0 = fenêtre, P1 = banque) recopie la banque sortante vers son stockage si elle est RAM (write-back 16 Ko) puis la banque entrante dans la fenêtre (memcpy 16 Ko, à mesurer : ~50-100 µs attendus sur RP2040, à confirmer sur carte), `1,19 Get Bank`. Stockage : **banques RAM** en SRAM du RP2040 (≈ 47 Ko libres avant F5 → 2 banques de 16 Ko au plus, à arbitrer avec F-52/F-53 qui en prennent 2 × 41 Ko en modes 1/2), **banques ROM** en flash (F-21 : lecture seule, 2 Mo, XIP → memcpy depuis la flash, à mesurer), banques « disque » = overlay classique (3,8). Variante « pointeur » (`bankPtr[a>>14][a&0x3FFF]` dans la boucle bus, sans copie, commutation instantanée) uniquement si la mesure sur carte (règle 4) montre que le surcoût tient dans la marge de 11 nop à 6,25 MHz. Émulateur `neo` et Phosphoneo alignés (la RAM 6502 y est un tableau : même API, même copie). À faire : ADR (copie vs pointeur, taille de fenêtre 8/16 Ko, nombre de banques), mesure, puis code. | bmarty (Neo6502civ : 8 Ko manquants, `$A000` occupé) | **DONE** 2026-09-17 (`feat/memory-banks`, ADR-03) : commutation par copie, fenêtre 8 Ko, **2 banques** (4 débordent la SRAM de 10,5 Ko), 1,18/1,19 ; `banks.asm` identique neo / Phosphoneo / co-sim ; **non testé sur carte** (R22 : 5,8 Ko de tas restants, coût des memcpy à mesurer) ; banques flash (F-21) non faites ; **pages blitter `$A0`/`$A1`** = stockage des banques (3,27 / 12,2 sans montage, `bankblit.asm`, décision bmarty 2026-09-17) |

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

### Épopée F9 — Télécom : série USB CDC (Neo6502drive EPIC-02 / EPIC-04)

| ID | Prio | Story | Origine | État |
|---|---|---|---|---|
| F-93 | P1 | **Routage UART UEXT <-> USB CDC** : les fonctions 10,13-10,18 visent le premier modem CDC (10,19 : 0 matériel / 1 CDC / 2 AUTO, défaut AUTO) afin que les programmes série existants (netconfig, prophet) utilisent un modem Wi-Fi USB **sans modification**. `cdcserial.cpp` (couche de routage), `group10_uext.inc`. | drive EPIC-02 (prophet en USB) | **DONE** 2026-09-16 — vérifié en co-sim (Phosphoneo) avec le vrai firmware et le Pico W réel : uarttest.s (API UART seule) dialogue avec le modem |
| F-94 | P1 | **Validation carte de la chaîne Prophet par USB** (F-90 + F-93 sur RP2040) : flasher le fork, Pico W modem 0.2.0 sur le port hôte (hub avec clavier + clé : cohabitation HID + MSC + CDC à vérifier), `run "prophetgui.neo"`, 6 étapes du mémo (durées, débit, stabilité, TLS). `MEMO-PROPHET-USB-CARTE-2026-09-16.md`. | PO via Neo6502Prophet | TODO — attend une carte ; UF2 prêts (`firmware/firmware.uf2` USB, SDCARD) |
| F-90 | P1 | **Modems et adaptateurs USB-série CDC-ACM** sur le port hôte : classe TinyUSB `cdc_host` (+ FTDI, CP210x), API groupe 14 (statut, octet/bloc en lecture et écriture, line coding), 2 périphériques ; hôtes PC sur tty/pty (`NEO_CDC_TTY`), co-sim avec le modem émulé de libemul. | bmarty « ajouter la prise en charge des modems CDC » | **DONE** 2026-09-15 (carte non testée, R15–R17) — `F-90-cdc.md` |
| F-91 | P2 | Le Neo6502 **lui-même** en périphérique CDC (câble USB vers un PC). **Étude faite** : le RP2040 n'a qu'un contrôleur USB, hôte (clavier, MSC, CDC) ou périphérique, pas les deux ; le lien PC ↔ Neo passe donc par l'**UART GP28/GP29** (230 400 bauds, `nxmit`, adaptateur USB-série côté PC) — déjà là pour le transfert (`nxmit`) et le débogage (1,10) ; F-92 y ajoute l'écran. Un pont CDC exigerait un second MCU (Pico W modem, Feather : Neo6502drive US-C5). | Neo6502drive EPIC-04 | Étude close ; pas de développement firmware |

| F-92 | P1 | **Écho console → port de débogage** (API 2,20) : le texte affiché part aussi sur l'UART GP28/GP29 (stderr dans les émulateurs) pour capturer l'écran d'une **vraie carte** depuis le PC (`nxmit`/adaptateur USB-série) et rejouer les tests golden sur le matériel. | validation carte | **DONE** 2026-09-15 (vérifié dans Phosphoneo : stderr) |

### Épopée F10 — Stockage : volumes et périphérique réseau `N:` (mémo Prophet 2026-09-16)

Origine : `MEMO-PROPHET-N-DEVICE-2026-09-16.md` (projet Neo6502Prophet, pour le
fork et Neo6502drive US-T3). Constat vérifié : un seul stockage monté à la
compilation (`STORAGE_TYPE = USB | SDCARD`, `usb_storage.cpp`/`sdcard_storage.cpp`
exclusifs), aucune notion de volume dans le groupe 3 (fonctions 3,1-3,23 et 3,32 ;
3,24-3,31 libres), aucun préfixe `N:` (`fileinterface.cpp`, `FISOpenFileHandle`).

| ID | Prio | Story | Origine | État |
|---|---|---|---|---|
| F-100 | P2 | **Périphérique réseau `N:` en lecture** dans le groupe 3 : un nom commençant par `N:` (insensible à la casse) dans 3,2 Load, 3,4 Open (mode 0), 3,8 Read, 3,5 Close, 3,16 Stat est routé vers le **proxy du modem** (Neo6502drive US-T3, mode « flux HTTP » : `open(url)` fait le GET, `read(n)` sert le corps) via le lien série routé par 10,19 ; lecture bloquante avec délai (le chargeur `.neo` lit sans reprise) ; nouveau code d'erreur « réseau ». Dépend de US-T3 côté modem. Test : Phosphoneo + faux modem ProphetGui (`tools/fake_modem.py`). | Prophet (LOAD "N:HTTP://…") | TODO — attend US-T3 (modem) |
| F-101 | P2 | **Montage multiple** SD **et** USB quand les deux sont présents (aujourd'hui exclusifs à la compilation : deux bibliothèques FatFs distinctes, `diskio` séparés à fusionner). Le préfixe de volume et l'API sont faits par F-102 ; reste la cohabitation SD SPI + USB MSC dans un seul `diskio`. | Prophet (explorateur) | TODO |
| F-102 | P2 | **API volumes** : `3,24 Volume Info` (P0 = volume, tampon nom, P3 attributs présent / lecture seule / réseau), `3,25 Select Volume`, `3,26 Get Current Volume` ; préfixe `n:` dans tous les chemins du groupe 3 (natif FatFs sur carte : clés USB à leur adresse USB, SD `0:` ; émulateurs : `<storage>`, `<storage>1..3`) ; 3,23 relatif à la racine du volume. Pas de « liste » : on énumère 0-3 par 3,24. | Prophet (explorateur) | **DONE** 2026-09-16 — vérifié dans `neo` et Phosphoneo (`volumes.neo6502`, golden + différentiel), compilé USB/SDCARD, **non testé sur carte** (plusieurs clés USB = plusieurs volumes, à valider) |
| F-103 | P1 | **Sécurité `N:`** (prérequis de F-100 en écriture) : jamais d'écriture vers un hôte non prévu sans consentement — liste d'hôtes autorisés dans le modem (`AT+NHOSTS=`) ou journal ; l'audit `neo-sandbox` signale tout appel 3,x avec un nom `N:`. Lecture seule tant que ce point n'est pas traité. | Prophet | TODO |

### Épopée F8 — Multi-boot RP2040 (sélecteur d'OS : firmware Neo ⇄ images reload-emulator)

| ID | Prio | Story | Origine | État |
|---|---|---|---|---|
| F-80 | P2 | **Étude** : plusieurs images firmware en flash (Neo ≈ 190 Ko, reload BBC/Oric/Apple //e avec ROM et images disque, 2 Mo au total), étage de démarrage qui lit le choix (registre scratch du watchdog / secteur de flash) et saute à l'image ; relocalisation des images (éditeur de liens Pico SDK, `boot2`) ; comportement PicoDVI/TinyUSB après saut. | Neo6502kbd EPIC-02, reload fork `bbc` | **DONE** 2026-09-15 : `multiboot/` (sélecteur `neoboot`, `memmap_slot_N.ld`, `mkimage.py`), `make -C firmware build-multiboot`, vérifié sur libemul (Phosphoneo `test-multiboot`) ; carte non testée (R11–R14, `F-80-multiboot.md`) |
| F-81 | P2 | API **1,14 Reboot Image** : P0 = image (0 = Neo) → mémorise le choix et redémarre (watchdog) ; le Télémon `O` liste les images. | EPIC-02 | **DONE** 2026-09-15 : 1,14 Reboot Image, 1,15 Get Image Name (`multiboot.cpp`, annuaire en flash) ; Télémon `O` → `F1..F3` |
| F-82 | P3 | Retour : une touche au reset (ou un délai) pour revenir au firmware Neo depuis une image reload (dans reload : `hid_app` F12 ?). | EPIC-02 | **DONE** 2026-09-15 dans le fork reload (branche `bbc`) : touche **Pause** → `scratch[0]=0` + `watchdog_reboot` (bbc, oric) ; liaison par slot `-DNEO_SLOT_BBC=1 -DNEO_SLOT_ORIC=2` ; sélection des slots 1/2 vérifiée sur libemul |
| F-83 | P2 | **Installer un `.uf2` depuis une clé USB** dans un slot multi-boot : API 1,16 Install Image (lit le `.uf2` par les `FIS*`, `flash_range_program` dans le slot), 1,17 Erase Slot ; le Télémon `O` liste les `.uf2` de `os/`. Une clé porte plusieurs machines, installées à la demande, sans PC. | bmarty « les uf2 sur une clé USB » | Conception (`F-83-install-usb.md`) — card-only, R18-R21 |

Dépendances : F-50 avant tout ; F-51 est le socle ; F-11 (blit 2 bpp) pour
F-53 ; la Toolbox (F4) s'appuie sur F-51/F-55 pour les surfaces hors écran.

## Épopée F4 — Toolbox (ADR-01, `docs-bmarty/adr/0001-toolbox.md`)

| ID | P | Story | Origine | État |
|----|---|-------|---------|------|
| F-40 | P2 | **Ratifier l'ADR-01** (numérotation ≥ 32, conventions de structures et d'erreurs, ordre) et créer le squelette `config/toolbox/group32_quickdraw.inc` + `sources/interface/toolbox/`. | Télémon, portages | **DONE** 2026-09-17 (`feat/toolbox-skeleton`) : ADR-01 ratifiée ; groupe 32 avec 10 fonctions (port, clip, plume, rectangles) ; sources en `interface/toolbox_*.cpp` (glob non récursif des 3 builds) ; `quickdraw.asm` identique neo / Phosphoneo / co-sim |
| F-41 | P2 | **QuickDraw (32)** : port courant + clipping, rectangles, lignes, motifs, `CopyBits` sur le blitter, fontes proportionnelles (format + outil PC de conversion), `DrawString/TextWidth` ; captures golden Phosphoneo. | | TODO |
| F-42 | P2 | **Event Manager (33)** : file unifiée clavier/souris/fenêtre/timer, `GetNextEvent`, `WaitNextEvent` (timeout ; IRQ F-10 quand disponible). | | TODO |
| F-43 | P2 | **Window Manager (34)** : fenêtres, ordre Z, cadres dessinés par le firmware, invalidation/`update`, drag/size, `FindWindow`. | | TODO |
| F-44 | P3 | **Menu (35) et Control (36) Managers**. | | TODO |
| F-45 | P3 | **Dialog Manager (37)**, descriptions en RAM 6502. | | TODO |
| F-46 | P3 | **Resource/Font (38) et Memory RP2040 (39)** : ressources sur SD, poignées hors des 64 Ko. | | TODO |
| F-47 | P2 | **Vitrine** : le Télémon (Neo6502kbd) utilise fenêtres + menus + événements ; documentation `api.tex` complète ; proposition amont. | | TODO |

| F-95 | P2 | **Police console définissable** (API **2,21 Set Console Font**) : les glyphes 8 lignes des caractères `$20-$7F` sont lus dans la **RAM 6502** (96 × 8 octets, MSB à gauche, adresse P0-1 ; 0 = police interne), sans copie côté RP2040 (SRAM : 0 octet) ; repeint tout l'écran ; police interne rétablie par 5,9 et au reset ; cellules 9×14 (Hercules) inchangées (police 8×14). Origine : Ozmoo/Neo6502 (équivalent des polices `fonts/*.fnt` du MEGA65/X16 ; seuls `$C0-$FF` étaient redéfinissables par 2,5). | Neo6502Ozmoo (EPIC 2) | **DONE** 2026-09-17 (`feat/console-font`) : `CONSetFont`, test `docs-bmarty/tests/confont.asm` identique dans `neo` et Phosphoneo (golden + différentiel + co-sim), USB compilé, **non testé sur carte** |

## Politique amont
**Décision bmarty 2026-09-16 : pas de pull request vers l'amont.** Le fork est
publié sur `origin` ; `upstream` n'est suivi que pour rebaser. (Ancien ordre de
proposition F-01 → F-02 → F-11 → F-10 → F-13 → F-14, abandonné.)
