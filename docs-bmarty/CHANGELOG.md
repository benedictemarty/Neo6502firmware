# Changelog (fork bmarty)

## [Unreleased]
- 2026-09-18 : F-17 — **locale FR et caractères Latin-1**. (a) `fr.locale` = **AZERTY PC** (table de
  `layouts/fr.kmap` de Neo6502kbd : lettres permutées, rangée des chiffres, `ù ² ° ¨ £ µ § ¤`, Alt/AltGr
  `~ # { [ | \` \\ ^ @ ] }`), codes **Latin-1** pour les accents ; l'ancien `fr.locale` amont (disposition
  **Apple** française : `@ § ! - = \``) devient `fm.locale`, complété des accents ; `keymaps.py` accepte les
  codes 160-255. (b) Glyphes 5×7 : `$A0-$BF` (symboles Latin-1, flash, `scripts/latin1.py` →
  `data/latin1font.h`) et `$C0-$FF` (lettres accentuées, composées depuis `font_5x7` : accent sur 2 lignes,
  capitales resserrées sur 6) chargées dans la police utilisateur au reset (`CONResetUserFont`) — 2,5 les
  remplace comme avant. La console (2,6, cellules de 8 et 14 lignes), 5,x Draw Text et QuickDraw 32,16/17
  (police système) affichent `$A0-$FF` (`CONGlyph`) ; `$80-$9F` restent des codes de couleur. Vérifié dans
  NeoBASIC (`a$="éè"` : `len` 2, `asc` 233). Test `latin1.asm` (touches AZERTY injectées, pixels, 2,5,
  32,16) ; tas inchangé (USB 3 280 o).
- 2026-09-18 : correctif `neo` (F-01) — l'autotype `keys:` sautait le code HID `$32` (non-US #) : `; ' \` , . /`
  envoyaient la touche précédente (`;` → `\`, etc.), invisible en locale US, faux en AZERTY.
- 2026-09-18 : backlog — F-17 **Locale FR** inscrite (demande bmarty) ; constat : `fr.locale` amont existe (AZERTY) sans accents ; périmètre à trancher.
- 2026-09-18 : F-45 — **Dialog Manager (groupe 37)** : dialogues modaux construits depuis un descripteur en RAM
  6502 lu en place (rect écran, drapeau barre de titre, titre, jusqu'à 12 items : genre 1-5 = contrôles du
  groupe 36, 6 = texte statique multi-lignes ; drapeaux inactif / défaut (Return, anneau) / annulation
  (Escape) ; rect relatif au contenu, maximum, texte ou tampon préfixé), sur les gestionnaires de fenêtres
  (34) et de contrôles (36). Aucun appel bloquant : `Dialog Event` reçoit l'EventRecord du groupe 33 et
  rend l'item qui a agi et si l'événement appartenait au dialogue (souris et clavier toujours ; update
  du dialogue redessiné sur place ; update/activate des autres fenêtres et timers rendus au programme) ;
  suivi des contrôles par phases, saisie vers le premier champ de texte (Tab : suivant). `Alert` :
  message centré (lignes sur 13) et boutons OK / OK-Cancel / Yes-No (titres en flash, `CTNewRaw`,
  `WMNewWindowRaw`). 2 dialogues (une alerte au-dessus d'un dialogue). Test `dlg.asm` identique dans
  `neo`, Phosphoneo et co-sim ; tas USB 3 280 o, SDCARD 4 528 o ; **non testé sur carte**.
- 2026-09-18 : correctif toolbox — la couleur « gris » des barres de titre inactives (34), items de menu
  et contrôles inactifs (35, 36) était l'index **8, noir opaque** dans la palette par défaut (le gris
  sombre est 9) : passage à 9 (`wm.asm` : `PIX 0F 0F 09`).
- 2026-09-18 : correctif `fix/hotkey-keyup` — depuis F-42, `KBDMapToASCII` était aussi appelé à la **relâche**
  d'une touche (`EVTPostKey(EVT_KEYUP, …)`) ; comme il insérait le texte des touches de fonction (2,4 Define
  Hotkey) en effet de bord, chaque appui sur F1..F10 injectait la chaîne **deux fois**. L'insertion est
  déplacée dans la branche *key down* de `KBDProcessEvent` ; `KBDMapToASCII` est de nouveau pur. Détecté par
  Ozmoo-Neo6502 (F1 = mode sombre basculait puis rebasculait, trace 2,1 de Phosphoneo).
- 2026-09-18 : R22 — console dimensionnée aux modes conservés (`MAXCONSOLEHEIGHT` 32, −1 760 o) ; tas USB 3 804 o,
  SDCARD 5 052 o ; mémo `R22-budget-sram.md` mis à jour (modes 3/4 retirés).
- 2026-09-18 : décision bmarty — **modes vidéo 3 (F-57, Hercules 80×43) et 4 (ADR-04 a, TEXT Oric) retirés**
  (revert de `13f6042` et `3236937`) pour rendre la SRAM au tas (R22) : le Control Manager ne tenait plus
  dans le firmware USB (2 220 o de tas sur cette base, minimum 2 048).
- 2026-09-18 : F-44 (2/2) — **Control Manager (groupe 36)** : 24 contrôles (bouton, case, radio, ascenseur
  vertical/horizontal, champ de texte) attachés à une fenêtre (rect relatif au contenu), titres/tampons lus en
  place, dessin sur demande (`Draw Controls` dans l'update), suivi par phases (`Track Control` : surbrillance
  du bouton, bascule, flèches/pages/curseur d'ascenseur), `Key Control` pour la saisie ; `WMContentRectOf`,
  `WMIsVisible`, `CTWindowDisposed`. Test `ctl.asm` identique dans `neo`, Phosphoneo et co-sim.
- 2026-09-18 : ADR-04 tranche (a) — **mode vidéo 4 : écran TEXT Oric rendu depuis la RAM 6502**
  (`$BB80`, 40×28 en 6×8, attributs série encre/papier/texte rejoués par ligne, inverse par XOR 7,
  double hauteur, clignotement, jeux de caractères `$B400`/`$B800` lus en RAM, couleurs Oric en
  palette 0-7, 240×224 centré dans 320×240). `memvideo.cpp` commun (`MEMRenderLine`,
  `MEMReadPixel`, `MEMSetPalette`), champ `layout` des descripteurs (`graphics.h`), crochet dans
  `_scanline_callback` (carte) et `GFXReadPixelRaw` (émulateurs) ; boucle bus intacte. Test
  `docs-bmarty/tests/orictext.asm` vérifié pixel à pixel dans `neo` contre les règles de Phosphoric
  (`video.c`) ; USB et SDCARD compilés (tampons ramenés à 320 octets : la RAM USB débordait après
  F-44, marge restante 2 220 octets), **non testé sur carte** (mesure F-50 §4 à faire avant merge
  définitif, R30).
- 2026-09-18 : **revert** du mode vidéo 4 (ADR-04 tranche a, `3236937`) à la demande de bmarty : la tranche
  avait été implémentée sur un « go » ambigu, sans demande explicite. L'ADR-04 reste une simple
  proposition (statut « proposée », aucune tranche réalisée). Retour à 4 modes vidéo, SRAM rendue.
- 2026-09-18 : F-44 (1/2) — **Menu Manager (groupe 35)** : barre de menus (12 lignes), 6 menus décrits en RAM
  6502 et lus en place (titre, items avec drapeaux inactif / coché / séparateur), menus déroulants suivis par
  35,3 Select / 35,4 Track / 35,5 Track End (pas d'appel bloquant), items en surbrillance, 35,6/35,8 drapeaux,
  35,7 Dispose ; fermeture = effacement + `WMInvalidate` (update des fenêtres recouvertes). Test `menu.asm`
  identique dans `neo`, Phosphoneo et co-sim. Travail fait dans un worktree séparé (`feat/menu-manager`).
- 2026-09-18 : **ADR-04 proposée** (`docs-bmarty/adr/0004-modes-memoire.md`) pour F-70/71/72/73 — modes vidéo
  rendus depuis la RAM 6502 (Oric TEXT/HIRES, Apple II texte, BBC MODE 7) : descripteur de mode
  « mémoire », convertisseur de ligne commun aux trois backends, attributs série Oric rejoués par
  ligne, **refus de l'interception d'adresses dans la boucle bus** (F-73 → scrutation par core 1),
  ordre de réalisation et mesure F-50 obligatoire avant merge. Aucun code ; décisions attendues du PO.
- 2026-09-18 : F-10 — **1,16 Set Frame Interrupt** / **1,17 Get Frame Interrupt** : interruption « sur
  trame » (vsync) du 65C02. Carte : `frameIrqOn` testé dans `_scanline_callback` (core 1, début de
  trame : `irqAsserted = true`, `wdc65C02cpu_set_irq(true)`, `gpio_put` sûr entre cœurs), relâchement
  par la lecture de `$FFFF` comme F-60, sans toucher à la boucle bus de core 0 ; `HWIRQSetFrame` /
  `HWIRQSetTick` ne relâchent IRQB que si l'autre source est inactive ; arrêt au DSP Reset. `neo` :
  une IRQ par trame émulée (`irqFrame`). Test `docs-bmarty/tests/frameirq.asm` (`ON 00 01 FRAMES 003C
  STOP 00 00 STILL 003C`) dans `neo` ; USB et SDCARD compilés, **non testé sur carte** (coût sur core 1
  et comportement de la ligne IRQB à mesurer).
- 2026-09-17 : F-14 (suite) — **horodatage FAT sur carte SD** : `HWClockSet` (`firmware/sources/hardware/clock.cpp`,
  `hardware_rtc`) programme la RTC du RP2040 au réglage 1,21 et à la détection du PCF8563 ; la
  bibliothèque SD (`rtc.c`, `get_fattime()`, `FF_FS_NORTC = 0`) horodate alors les fichiers. Stockage
  USB : le FatFs de TinyUSB est compilé `FF_FS_NORTC = 1` (dépendance FetchContent) → pas d'horodatage.
  No-op dans `neo`. USB et SDCARD compilés, **non testé sur carte**. `BUILD.md` : les objets de `neo`
  ne suivent pas les en-têtes (suppression des `.o` après un changement de `.h`).
- 2026-09-17 : R22 — **revue du budget SRAM** (`docs-bmarty/R22-budget-sram.md`) : tampon de ligne
  partagé `gfxLineScratch` (tilemap + doublage blitter, −720 o), rapport et **seuil de tas** au build
  (`make -C firmware build` refuse sous 2 048 o) ; tas USB 2 692 → 3 408 o. Options chiffrées à décider :
  `FF_FS_TINY` (−4 Ko, patch de dépendance), 1 banque (−8 Ko), 2 volumes (−1,4 Ko), 1 page en modes 1/2.
- 2026-09-17 : F-43 — **Window Manager (groupe 34)** : 8 fenêtres (cadre, barre de titre 10 lignes, case de
  fermeture, case de taille), ordre Z, 34,1-34,12 (New/Dispose/Show/Select/Move/Size/Find/Get Content Rect/
  Set Title/Front/End Update/Set Port) ; le firmware dessine les cadres et efface les contenus, le 6502 dessine
  les contenus sur les événements **update (9)** / **activate (10)** ajoutés au groupe 33 ; `End Update`
  repeint les cadres au-dessus. Primitives internes QuickDraw (`QDFillRaw`, `QDFrameRaw`, `QDTextRaw`…).
  Test `wm.asm` (deux fenêtres, Find Window par partie, sélection, événements, déplacement, destruction,
  Set Port → clip) identique dans `neo`, Phosphoneo et co-sim. USB/SDCARD compilés, non testé sur carte.
- 2026-09-17 : F-14 — **1,20 Get Date Time** / **1,21 Set Date Time** (`interface/clock.cpp`) : PCF8563
  (I2C UEXT, `$51`, registres BCD `$02-$08`, bit VL) sondé à la première utilisation, sinon horloge
  logicielle (secondes depuis 1970 réglées par 1,21, avancées par le timer 100 Hz, débordement 32 bits
  géré) ; conversion date civile ↔ jours (algorithmes de H. Hinnant), validation des champs ; source
  dans P7. Émulateur `neo` : PCF8563 modélisé à `$51` sur l'heure de l'hôte (écriture = décalage).
  Déclarations en portée de bloc dans `group1_system.inc` (common.h volontairement non touché : en
  cours de modification par F-42). Test `docs-bmarty/tests/datetime.asm` (source 2 dans `neo`, réglage
  2026-09-17 12:34:56 relu, mois 13 refusé) ; USB compilé, **RTC/I2C non testés sur carte** ;
  horodatage FAT non fait.
- 2026-09-17 : F-42 — **Event Manager (groupe 33)** : file unique de 32 EventRecord (8 o) alimentée par
  `KBDEvent`/`KBDCheckTimer` (key down/up/auto) et `MSE*` (down/up avec double clic, move fusionnés, wheel) ;
  33,1-33,6 (Init, Get Next Event avec masque, Available, Flush, Set Timer ×4 en ticks 10 ms générés à la
  scrutation, Status avec débordement). Pas d'attente bloquante (choix documenté : appels synchrones, hôtes
  qui lisent leurs entrées entre deux appels ; sommeil par 1,12 + WAI). Tests `events.asm` (souris + touches
  injectées : neo, Phosphoneo golden/diff, co-sim) et `evtimer.asm` (timers/masque/flush : Phosphoneo et
  co-sim seulement, `TMRRead` de `neo` = horloge murale). USB/SDCARD compilés, non testé sur carte.
- 2026-09-17 : F-12 — **8,9 Set Channel Volume** / **8,10 Get Channel Volume** : volume (0-100) de la note
  en cours d'un canal, immédiat ou atteint par rampe linéaire en P2-3 centièmes de seconde (tick 50 Hz de
  `SNDManager`, champs `fadeTarget`/`fadeStep`), appliqué au générateur sans réinitialiser la phase
  (`SNDSetCreatorVolume`, pas de clic) ; les notes en file gardent leur volume, une nouvelle note annule
  la rampe. Test `docs-bmarty/tests/sndvol.asm` (note 3 s, 100 → 50 immédiat, 50 → 0 en 1 s : valeur
  intermédiaire puis 0 ; canal inexistant et canal silencieux refusés) vérifié dans `neo` ; USB
  compilé, **non testé sur carte** (niveau PWM réel non mesuré).
- 2026-09-17 : F-95 (2/2) — **2,21 Set Console Font, 2e adresse** (P2-3) : police 14 lignes des cellules
  9×14 du mode Hercules (96 × 14 octets, 8 colonnes, lue en place ; 0 = 8×14 interne), contrôle de
  bornes séparé, repeint ; `CONSetFont(addr, addr14)`. Test `docs-bmarty/tests/confont14.asm`
  (mode 1, 'A' = 8 colonnes × 14 lignes, 9e colonne éteinte, adresse hors RAM refusée) vérifié
  dans `neo` (capture 720×350) ; USB compilé, **non testé sur carte**.
- 2026-09-17 : F-41 incrément 2 — QuickDraw **fontes proportionnelles** : format **NF1** (en-tête 8 o :
  "NF", 1, hauteur, premier, nombre, espacement, octets/ligne ; glyphes = largeur + lignes), **32,15 Set Font**
  (page:adresse du blitter, lue en place ; 0:0 = police système 6×8), **32,16 Draw String** (transparent,
  clippé, plume avancée), **32,17 Text Width**, **32,18 Get Font Info** ; outil PC
  `docs-bmarty/tools/mkfont.py` (TTF → NF1 par Pillow, seuil de gris, débord gauche) ; police d'essai
  `tests/dejavu9.nf1`. Test `qdfont.asm` (police système puis NF1 chargée par 3,27 en `$90`) identique dans
  `neo`, Phosphoneo et co-sim. Régions : non faites (sans besoin exprimé).
- 2026-09-17 : F-41 incrément 1 — QuickDraw : **32,11 Line To** (Bresenham clippé pixel à pixel),
  **32,12 Set Pattern** / **32,13 Fill Rect** (motif 8×8 aligné écran, plume / fond), **32,14 Copy Bits**
  (structure source de 12,3 blittée sur la page de travail, clippée au clip — `BLTCopyArea`/`BLTLoadArea`
  exportés du blitter ; mode 0 seul). Test `quickdraw2.asm` identique dans `neo`, Phosphoneo et co-sim.
  Reste pour l'incrément 2 : fontes proportionnelles (format, outil PC), Draw String / Text Width, régions.
- 2026-09-17 : F-95 — **2,21 Set Console Font** : la police 8 lignes des caractères `$20-$7F` de la
  console peut être remplacée par 96 glyphes lus **en place dans la RAM 6502** (P0-1 = adresse, MSB =
  pixel gauche, 6 colonnes utilisées en cellule 6×8, 8 en 8×8 ; 0 = police interne) : aucune SRAM
  RP2040 consommée, écran repeint, police interne rétablie par 5,9 et au reset, mode Hercules
  (9×14) inchangé, `$80-$BF` et `$C0-$FF` (2,5) inchangés. `CONSetFont` (console.cpp). Test
  `docs-bmarty/tests/confont.asm` (police de test : 'A' plein, 'B' damier ; adresse hors RAM refusée ;
  retour à la police interne) identique dans `neo` et Phosphoneo (golden + différentiel + co-sim) ;
  firmware USB compilé, **non testé sur carte**. Demande : Ozmoo/Neo6502 (équivalent des polices
  `fonts/*.fnt` des cibles MEGA65/X16).
- 2026-09-17 : F-40 — **ADR-01 ratifiée**, squelette de la Toolbox : groupe **32 QuickDraw**
  (`config/toolbox/group32_quickdraw.inc`, `toolbox_quickdraw.cpp`, `toolbox.h`) avec les conventions
  (structures en RAM 6502 par adresse, `Rect` int16 droite/bas exclus, erreurs 0/1/2) et 10 fonctions :
  Init Graf, Set/Get Clip, Set Pen Colour, Move To, Get Pen, Frame/Paint/Erase/Invert Rect (clippés,
  tout mode via `GFXWritePixelRaw`). Lignes, motifs, fontes, CopyBits → F-41. Test
  `docs-bmarty/tests/quickdraw.asm` identique dans `neo`, Phosphoneo et en co-sim ; USB/SDCARD
  compilés, non testé sur carte.
- 2026-09-17 : F-11 — **blitter 2 bpp** : format source 5 (`BLTFMT_QUAD`, 4 valeurs de 2 bits, MSB
  en premier) dans 12,3 (copy / masked / solid, cibles octet et quartets) et 12,4 (clipping) ;
  **doublage horizontal** par le bit 0 de l'octet 3 de la source (ex-pad) dans 12,3 : ligne dépaquetée
  puis doublée (≤ 360 valeurs, 720 o de RAM), tous formats ; 12,4 le refuse. Test `docs-bmarty/tests/blit2bpp.asm`
  identique dans `neo`, Phosphoneo et en co-sim du vrai firmware ; USB/SDCARD compilés, non testé sur
  carte. Les dispositions entrelacées (BBC MODE 5, C64) restent à convertir en amont.
- 2026-09-17 : F-23 (suite, décision bmarty) — le stockage des banques est adressable par le blitter :
  pages **`$A0`/`$A1`** dans `BLTGetRealAddress` (donc 3,27 et 12,2) ; une banque se charge depuis
  le disque ou se copie depuis/vers la VRAM sans être montée. Test `bankblit.asm` (3,27 → `$A1`,
  montage, write-back visible par 12,2, débordement et page inconnue refusés) identique dans `neo`,
  Phosphoneo et en co-sim du vrai firmware ; USB/SDCARD compilés, non testé sur carte.
- 2026-09-17 : F-23 — **banques mémoire 6502** (ADR-03) : `banks.cpp` (2 banques de 8 Ko hors RAM
  6502, commutation par copie pendant l'appel API avec write-back), **1,18 Select Bank**, **1,19 Get
  Bank Info**, oubli au DSP Reset. 4 banques débordaient la SRAM du RP2040 de 10,5 Ko ; à 2 banques
  il reste 5,8 Ko de tas (USB) — R22, à valider sur carte. Test `docs-bmarty/tests/banks.asm`
  identique dans `neo`, Phosphoneo (golden + différentiel) et en co-sim du vrai firmware ;
  USB et SDCARD compilés, **non testé sur carte**.
- 2026-09-17 : F-16 — **3,27 File Read Paged** : lecture d'un fichier ouvert directement dans une
  page du blitter (`$00` RAM 6502, `$80`/`$81` VRAM, `$90` RAM graphique) ; `FIOReadFileHandlePaged`
  (commun, bornes par `BLTGetRealAddress` désormais exporté), `FISReadFileHandleBuffer` (carte FatFs,
  `neo`, Phosphoneo + crochet co-sim). Test `docs-bmarty/tests/readpaged.asm` (VRAM page `$81`,
  RAM graphique, RAM 6502, page inconnue, débordement, fin de VRAM, EOF) : sortie identique dans
  `neo`, Phosphoneo (golden + différentiel) et le vrai firmware ARM en co-sim ; firmware USB et
  SDCARD compilés, **non testé sur carte**. Constat : la console qui défile redessine tout l'écran.
- 2026-09-17 : story F-23 au backlog — banques mémoire 6502 par copie pendant un appel API
  (fenêtre 16 Ko, banques RAM en SRAM / ROM en flash, `1,18`/`1,19` proposés), F-21 rattachée ;
  faits vérifiés dans `processor_pio.cpp` (lecture servie par `cpuMemory[a]`, 11 nop de marge).
- 2026-09-17 : story F-16 au backlog — `3,27 File Read Paged` (lecture fichier directe vers une page blitter `00`/`80`/`81`/`90` + adresse 16 bits) pour supprimer l'aller-retour RAM 6502 → 12,2 du streaming d'images de Neo6502civ ; constat vérifié : 3,8 `$FFFF` lit en RAM graphique à l'offset 0 seulement. Pas de code.
- 2026-09-16 : F-02 — doc 2,2 Console Status corrigée ($FF = touche disponible, comme le code) ; nettoyage des 3 avertissements amont (`unused variable`) ; carte : 3,11 File Set Size conserve la position du fichier (comme `ftruncate` dans l'émulateur, `oldPos` enfin utilisé) — non testé sur carte.
- 2026-09-16 : émulateur `neo` (et Phosphoneo) — un fichier absent renvoie désormais `$11` No File comme FatFs sur la carte (ENOENT n'était pas converti : erreur 1 Unknown). Fidélité carte ; le test api-log de Phosphoneo attend 17.
- 2026-09-16 : mémo Prophet « USB sur carte » versionné, story F-94 (validation carte F-90/F-93 avec le Pico W, ProphetGui) au backlog.
- 2026-09-16 : F-102 — volumes du groupe 3 : **3,24 Volume Info**, **3,25 Select Volume**,
  **3,26 Get Current Volume**, préfixe `n:` dans les chemins (`FISGetVolumeInfo/SelectVolume/
  GetCurrentVolume` : carte = lecteurs logiques FatFs, `f_chdrive` ; `neo` = `<storage>` et
  `<storage>1..3`, 3,23 relatif à la racine du volume) ; `group3_fileio.inc`, `API-DELTA.md`.
  Vérifié dans `neo` et Phosphoneo (`volumes.neo6502`, golden + différentiel), firmware USB et
  SDCARD compilés, non testé sur carte.
- 2026-09-16 : décision bmarty — plus de pull request vers l'amont ; CLAUDE.md et politique
  amont du backlog mis à jour. La branche `pr/emulator-build-fix` reste sur origin, sans suite.
- 2026-09-16 : dépôt distant `origin` = fork GitHub `benedictemarty/Neo6502firmware`
  (fork de `paulscottrobson/neo6502-firmware`) ; `bmarty/main` et les branches `feat/*`
  poussées ; branche `pr/emulator-build-fix` (un commit sur `upstream/main`, émulateur
  recompilé : 0 erreur) prête pour la PR amont ; `firmware/build_mb/` ignoré.
- 2026-09-16 : F-00 partie RP2040 — firmware compilé (USB, SDCARD, slot 0 multi-boot) avec
  le Pico SDK 1.5.1 ; `firmware/sources/CMakeLists.txt` accepte le `boot2` des SDK 1.x et 2.x ;
  `multiboot/Makefile` prend `PICO_SDK_PATH` ; procédure dans `docs-bmarty/BUILD.md`.
  Le SDK 2.2.0 est écarté (PicoDVI amont : `tcr`). Phosphoneo `make test` : 0 échec.
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
