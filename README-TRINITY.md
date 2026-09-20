# Trinity — firmware Neo6502 de référence (bmarty)

**Firmware de référence unique depuis le 2026-09-19** (le fork `bmarty/main` est archivé). Backlog : `docs/BACKLOG.md`.

Branche `trinity` (bmarty, 2026-09-18) : le firmware **amont** (`v1.0.0-14-gdc70908`, Paul Robson, MIT)
plus : la reconnaissance d'un modem série USB (Pico W « picowifiusb », CDC-ACM) sur un port USB-A de la carte —
F-90 (groupe 14, `cdc.cpp`, `cdcserial.cpp`, TinyUSB `cdc_host`) et F-93 (routage des fonctions UART 10,13-10,18
vers le modem, 10,19, AUTO par défaut) ; TinyUSB 0.21 ; le menu `boot/` ; le mode vidéo 1 Hercules ; et, depuis
la **0.4.0**, **NeoDOS comme environnement résident à la place de NeoBASIC** (T-15).

## Construction (0.4.0)

`make -C firmware build STORAGE=USB` (SDK 1.5.1, TinyUSB 0.21, PicoDVI amont). L'image de NeoDOS est lue dans
`$(NEODOSDIR)build/neodos.bin` (défaut `../Neo6502Msdos/`, `make NEODOSDIR=/chemin/`) et convertie en
`firmware/common/include/data/neodos_binary.h` (`kernel/scripts/hconvert.py … neodos C000`) ; même chose pour
l'émulateur `neo` (`make -C emulator elinux`). Le dépôt Neo6502Basic n'est plus nécessaire au firmware ni à `neo` ;
`BASICDIR` ne sert qu'aux cibles `examples/` et `release/` de l'amont.

Bannière : `Trinity Firmware: v0.0.1` (tag `trinity-v0.0.1` ; entre deux tags : `v0.0.1-N-gXXXXXXX`). Compilation : comme l'amont
(`make -C firmware build STORAGE=USB`, SDK 1.5.1, TinyUSB 0.16.0, PicoDVI amont non modifié).

## Budget mémoire (T-13)

`make -C firmware size` (appelé par `build`) affiche `.text/.rodata/.data/.bss` et **échoue si `.data` + `.bss` + vecteurs
dépasse `RAM_LIMIT`** (230 000 o par défaut) sur les 262 144 o de SRAM principale (les piles de core 0 sont dans les
2 × 4 Ko de scratch ; `core1_stack` 2 Ko est dans `.bss`). Ce qui reste est le tas (`malloc`, FatFs, TinyUSB).

| Version (USB) | .text | .rodata | .data | .bss | RAM occupée | Libres (tas) | UF2 |
|---|---|---|---|---|---|---|---|
| Morpheus amont `dc70908` | — | — | — | 207 596 (Berkeley) | — | — | 370 176 |
| Trinity 0.3.0 | 126 112 | 60 788 | 17 920 | 212 488 | 230 600 | 31 544 | 413 696 |
| Trinity 0.3.1 | 117 696 | 57 440 | 13 828 | 211 000 | **225 020** | **37 124** | 379 904 |
| Trinity 0.4.0 | 117 448 | 48 312 | 13 828 | 211 000 | 225 020 | 37 124 | 360 960 |

Règle : une nouvelle fonctionnalité prend sa mémoire dans `graphicsMemory` (76 800) / `gfxObjectMemory` (32 768) /
`cpuMemory` (65 536), jamais dans un nouveau tableau `static` (`docs/BACKLOG.md`, T-13).

## Versions

- **0.6.0** (2026-09-20, **à valider sur carte** : `~/neo-carte/trinity-0.6.0-volumes-clock-USB.uf2`) — **Reprises du fork
  demandées par NeoDOS (T-18, mémo T-10)** : **volumes** (F-102 : `3,24` Volume Info, `3,25` Select Volume, `3,26` Get Current
  Volume, préfixe `n:` dans tous les chemins du groupe 3 ; carte : lecteurs logiques FatFs, clés USB montées à leur adresse
  USB — `FF_VOLUMES` = 4 déjà dans `firmware/lib/fatfs` ; `neo` : `storage`, `storage1`..`3`), **`3,27` File Read Paged**
  (F-16 : lecture directe en RAM 6502 / VRAM / RAM graphique ; le Resource Manager l'utilise désormais), **date et heure**
  (F-14 : `1,20` Get / `1,21` Set Date Time, `clock.cpp` — PCF8563 à `$51` sur l'I2C UEXT si présent, sinon horloge
  logicielle sur le timer 100 Hz ; `neo` modélise le PCF8563 sur l'heure de l'hôte). **Horodatage FAT** : Trinity
  compile sa propre FatFs → `FF_FS_NORTC = 0` et `get_fattime()` calculé depuis l'horloge du firmware
  (`firmware/sources/hardware/clock.cpp`) : les fichiers écrits par NeoDOS/NeoBASIC sur la clé sont datés (le fork ne
  l'avait que sur SD via la RTC du RP2040, inutile ici). Diffs du fork appliqués en fusion 3 voies depuis l'amont
  `dc70908` (conflits : `FISReadFileHandleBuffer` déjà présent) ; `neo` : répertoire courant par volume initialisé
  paresseusement. Tests `tests/api/` (`make test-api`) : `readpaged.asm` et `datetime.asm` du fork (sorties identiques,
  seconde en regex), `volumes.asm` nouveau (3,24-26, ouverture `1:vol1.txt`). RAM 35 344 o libres ; UF2 406 528 o.
  Reste à faire pour NeoDOS : dates dans `3,18`/`3,16` (demande 3 du mémo).

- **0.5.6** (2026-09-20, **à valider sur carte** : `~/neo-carte/trinity-0.5.6-hercules-USB.uf2`) — **Toolbox en mode 1
  Hercules** (T-12, dernier point) : tout pixel de QuickDraw passe par `_QDPut` — en 1 bpp le gris de la toolbox
  (couleur 9 : barres de titre inactives, items et contrôles désactivés) devient un **damier**, les autres couleurs leur
  bit 0 (15 = allumé, 0 = éteint : cadres blancs, barres blanches à texte noir, comme en mode 0) ; `32,14 CopyBits`
  a un chemin pixel par pixel hors mode 0 (sources BYTE/PAIR/BITS, actions copy/masked/solid) au lieu de l'erreur 2.
  Police 6×8 système conservée (lisible en 720×350). Test `tests/toolbox/hercules.asm` (5,9 → 1, PaintRect, gris,
  CopyBits BITS, fenêtre « Herc », retour en mode 0 ; pixels lus par `5,33`) : OK dans `neo`, capture conforme.
  `make test-toolbox` 9/9. RAM inchangée (35 620 o libres) ; UF2 401 408 o. **T-12 est complète côté `neo`** ;
  toute la 0.5.x reste à valider sur carte.

- **0.5.5** (2026-09-20, **à valider sur carte** : `~/neo-carte/trinity-0.5.5-resources-USB.uf2`) — **Toolbox : groupe 38
  Resource Manager** (T-12), repris du fork : fichier de ressources NR1 sur le stockage (`tools/toolbox/mkres.py`,
  `mkfont.py` repris de l'archive), ouvert sur un canal (`38,1`), table lue à la demande (`38,3` Count, `38,4` Find
  type/id, `38,5`/`38,6` Info, `38,7` Load dans une page du blitter `$00`/`$90`, `38,8` Use Font = Load + `32,15`), rien de
  mis en cache côté RP2040. Adaptations : pas de `3,27` (lecture paginée locale `_RSReadPaged`), nom de fichier par tampon
  fixe (T-13), `FISReadFileHandleBuffer` (F-16 du fork) ajouté aux deux hôtes (`fileimplementation.cpp`, `hardware.cpp`
  de `neo`) ; `RSReset` au reset. Test `res.asm` + `test.res` (police `dejavu9.nf1` chargée en `$90:0000` et
  sélectionnée) : sortie identique au fork. **La toolbox du fork est intégralement reprise (32-38), mode 0.**
  RAM inchangée (35 620 o libres) ; UF2 400 384 o.

- **0.5.4** (2026-09-20, **à valider sur carte** : `~/neo-carte/trinity-0.5.4-dialogs-USB.uf2`) — **Toolbox : groupe 37
  Dialog Manager** (T-12), repris du fork sans modification (`toolbox_dialogs.cpp`, `dialogs.h`, `group37_dialogs.inc`) :
  dialogues modaux construits depuis un descripteur en RAM 6502 (fenêtre + items : boutons, cases, radios, champs,
  textes ; bouton par défaut avec anneau, Entrée/Échap), `37,2` Alert (message + Yes/No centrés), `37,3` Dialog Event
  (consomme les événements du groupe 33 destinés au dialogue) ; `DLReset` au reset. Aucune dépendance à la locale
  constatée dans le code (la note de T-12 visait les textes accentués des tests golden Phosphoneo). Test `dlg.asm` du
  fork : sortie identique ; capture : alerte « Save? » avec « Yes » par défaut. 35620 o de RAM libres ; UF2 397824 o.

- **0.5.3** (2026-09-20, **à valider sur carte** : `~/neo-carte/trinity-0.5.3-controls-USB.uf2`) — **Toolbox : groupe 36
  Control Manager** (T-12), repris du fork sans modification (`toolbox_controls.cpp`, `controls.h`, `group36_controls.inc`) :
  24 contrôles liés à une fenêtre — bouton, case à cocher, bouton radio, ascenseur, champ de texte (texte dans la RAM
  6502) — `36,1` New … `36,10` Key ; `CTWindowDisposed` rebranché dans le Window Manager (les contrôles partent avec
  leur fenêtre) ; `CTReset` au reset. Test `ctl.asm` du fork : sortie identique. RAM +480 o (`controls[24]`),
  35 776 o libres ; UF2 390 656 o.

- **0.5.2** (2026-09-20, **à valider sur carte** : `~/neo-carte/trinity-0.5.2-menus-USB.uf2`) — **Toolbox : groupe 35 Menu
  Manager** (T-12), repris du fork sans modification (`toolbox_menus.cpp`, `menus.h`, `group35_menus.inc`) : barre de menus
  (12 lignes, 6 menus), menus déroulants suivis par `35,3` Menu Select / `35,4` Track / `35,5` Track End, descripteurs
  lus en place dans la RAM 6502 (titre, items : drapeaux désactivé/coché/séparateur), `35,6`/`35,8` drapeaux d'item,
  `35,7` Dispose ; les fenêtres recouvertes par un menu reçoivent un update (`WMInvalidate`). `MNReset` au reset. Test
  `menu.asm` du fork : sortie identique. RAM +56 o (36 256 o libres) ; UF2 384 000 o.

- **0.5.1** (2026-09-20, **à valider sur carte** : `~/neo-carte/trinity-0.5.1-events-USB.uf2`) — **Toolbox : groupe 33 Event
  Manager** (T-12), repris du fork (`toolbox_events.cpp`, `events.h`, `group33_events.inc`) : file unique de 32 événements
  (clavier down/up/auto, souris down/up/move/molette, 4 timers, update/activate des fenêtres), `33,1` Init (masque),
  `33,2` Get Next Event, `33,3` Available, `33,4` Flush, `33,5` Set Timer, `33,6` Status. Crochets dans `keyboard.cpp`
  (`EVTPostKey` ; touches de fonction traitées à l'appui seulement, comme le fork) et `mouse.cpp` (`EVTPostMouseMove`,
  `EVTPostWheel`, `EVTPostMouseButtons`) ; `EVTReset` au reset ; le Window Manager poste réellement ses événements
  (crochet no-op retiré). Tests : `wm.asm` avec `EVENTS = 1` → sortie **identique au fork, lignes `EV` comprises** ;
  `evtimer.asm` identique ; `manuel/events.asm` (souris + « ab ») pour la carte, `neo` de Trinity n'injectant pas d'entrées.
  RAM : +384 o (file de 32 × 8 o + timers), 36 312 o libres ; UF2 379 392 o.

- **0.5.0** (2026-09-20, **à valider sur carte** : `~/neo-carte/trinity-0.5.0-toolbox-USB.uf2`) — **Toolbox, première livraison
  (T-12) : groupe 32 QuickDraw et groupe 34 Window Manager**, repris du fork (`archive/bmarty-main-2026-09-19`,
  `toolbox_quickdraw.cpp`, `toolbox_windows.cpp`, `config/toolbox/group32_quickdraw.inc`, `group34_windows.inc`, ADR-01),
  mode 0 d'abord (décision bmarty). Adaptations : pas de pages de banques (`$A0+`) ni de sources blitter 2 bits / doublées
  (`12,3` amont : `BLTGetRealAddress`, `BLTLoadArea`, `BLTCopyArea` exposés par `blitter.cpp`) ; `CONGlyph` (police 6×8
  système, `$C0-$FF` UDG) dans `console.cpp` ; les groupes ≥ 32 sont dispatchés par `DSPToolbox()` en flash
  (`dispatch_toolbox.h`, `makedispatch.py` R22 du fork) pour ne pas grossir `DSPHandler` copié en RAM ; `QDInitGraf` et
  `WMReset` au reset. **Pas encore repris** : Event Manager (33) — les événements update/activate des fenêtres sont
  ignorés (crochets no-op dans `toolbox_windows.cpp`), le programme redessine après chaque appel — et Control Manager (36).
  Tests : `tests/toolbox/quickdraw.asm`, `quickdraw2.asm`, `wm.asm` (du fork, journal console recopié en RAM `$2000`,
  fin `jmp $FFFF` sous `neo`), `make test-toolbox` → sorties **identiques à celles du fork** (hors lignes `EV`).
  RAM : +428 o (`windows[8]`, ordre Z), 36 696 o libres ; UF2 375 808 o (+14 336 o de flash).

- **0.4.1** (2026-09-20, **validé sur carte** le soir même : bannière `v0.4.0 dirty` = ce code avant le tag ; `boot/neobasic.bin`
  lancé par `auto.txt`, `vmode 1` → curseur visible, `list` lisible, numéros de ligne soulignés comme l'encre rouge du mode 0,
  `vmode 0` → retour ; UF2 reconstruit avec la bannière `v0.4.1-1-g10724fe` : `~/neo-carte/trinity-0.4.1-mda-USB.uf2`)
  — **NeoBASIC en mode 1 (T-16)**. La 0.4.0 (NeoDOS résident, menu `boot/`) est validée par la même séance.
  Exploration du « problème BASIC / MDA » dans `neo` (captures d'écran de la fenêtre SDL) : (1) `cls` remplissait la
  mémoire console avec l'encre 7 codée en dur → en mode 1 (F-52 : 7 = allumé + souligné + gras) chaque défilement
  repeignait les cellules vides **soulignées sur toute la ligne** ; (2) le codage F-52 des attributs sur les bits de
  l'encre rendait les encres de NeoBASIC (`02colours.inc`) : 7 blanc → gras souligné, 6 cyan (mots-clés de `list`) →
  gras souligné, 2 vert (prompt) et 3 jaune → soulignés, **11 orange (constantes de `list`, encre conservée ensuite) →
  clignotant** (le `1` de `vmode 1` et le texte suivant disparaissaient une demi-seconde sur deux). Décision bmarty :
  **codage fidèle à l'IBM MDA** — l'octet d'attribut est `papier << 4 | encre` : encre 0 = éteint, 1 = souligné, 2-7 =
  normal, 8-15 = gras ; papier 1-7 = inverse, papier 8-15 = clignotant (`CONMDADecode`, `console.cpp`). Avec NeoBASIC :
  texte et mots-clés normaux, constantes en gras, numéros de ligne soulignés, rien ne clignote ; le curseur inverse
  toujours la cellule en 1 bpp (avant : XOR avec l'encre, invisible pour une encre paire). L'encre par défaut de la console
  est 7 dans tous les modes. Vérifié dans `neo` (`print`, défilement, `cls`, `cursor`, `input`, `list`, attributs, 1 → 0).
  **Écran noir sur carte expliqué** : bmarty observait après `vmode 1` depuis NeoBASIC (0.3.0) un écran noir, signal
  présent, sans activité, alors que `mda.neo6502` fonctionnait ; protocole carte : `vmode 0` tapé à l'aveugle ramène l'image
  (BASIC vivant), idem en 0.3.0, rien ne change en attendant. Reproduit dans `neo` avec la console 0.3.0 : `vmode 1` au prompt
  → **0 pixel allumé**. Cause : après `vmode`, NeoBASIC n'imprime rien (pas de « Ready ») et n'affiche que le curseur,
  que la console dessinait par XOR avec l'encre courante — l'encre du prompt est le vert (2), bit 0 nul, donc **aucun pixel
  inversé en 1 bpp** : écran vide, curseur invisible, jusqu'à ce qu'on tape quelque chose. `mda.neo6502` n'a pas de curseur.
  Corrigé par le curseur toujours inversé (ci-dessus) ; vérifié dans `neo` (curseur visible en haut à gauche).

- **0.4.0** (2026-09-20, **à valider sur carte** : `~/neo-carte/trinity-0.4.0-neodos-USB.uf2`) — **NeoDOS remplace
  NeoBASIC comme environnement résident (T-15)**, décision bmarty du jour. Le firmware embarque `neodos_binary.h`
  (NeoDOS 0.12.0, dépôt Neo6502Msdos, image brute `$C000-$E854`, 10 325 o) au lieu de `basic_binary.h` ;
  `MEMInitialiseMemory` et `1,3` (« Load BASIC ») chargent NeoDOS depuis la flash et pointent `jmp (0)` sur `$C000` ;
  le premier `1,3` du noyau applique toujours le choix du menu `boot/` (entrée par défaut affichée : `1 NeoDOS`).
  La copie racine `neobasic.bin` de la 0.2.0 est retirée : **NeoBASIC se lance par `boot/neobasic.bin`** (= `bin/basic.bin`
  du dépôt Neo6502Basic, image `$800`) et `boot/auto.txt` ; `boot/neodos.neo` devient inutile. Vérifié dans `neo` :
  sans `boot/`, NeoDOS exécute `AUTOEXEC.BAT` (`ECHO … > RESULT.TXT`, `VER` = NeoDOS 0.12.0) ; avec `boot/neobasic.bin`
  + `auto.txt`, NeoBASIC démarre. UF2 360 960 o (−18 944 o par rapport à 0.3.1), RAM inchangée (225 020 o).
  Conséquence pour NeoDOS : `EXIT` (1,3 + `jmp (0)`) relance NeoDOS ; le stub de survie `$0100` n'est plus nécessaire
  sur Trinity (T-11 réduit).

- **0.3.1** (2026-09-20, **à valider sur carte** : modes 0 et 1, lignes noires des bordures verticales, chargement de fichiers)
  — **budget mémoire (T-13)** : −5 580 o de RAM, −33,8 Ko d'UF2, sans changement fonctionnel. (1) `std::string` de l'API
  fichiers tirait `functexcept.o` de libstdc++ (précompilé **avec** exceptions) → `__cxa_throw`, pool d'urgence des
  exceptions (constructeur statique + `malloc` au démarrage) et dérouleur libgcc, que le script de lien du SDK place en
  RAM (3,8 Ko) ; le SDK compile déjà en `-fno-exceptions`, ce n'était donc pas une option de compilation mais un effet
  de lien. Remède : `firmware/sources/hardware/cxxthrow.cpp` (les `std::__throw_*` → `panic`, `new (nothrow)` → `malloc`)
  et `firmware/include/cxx_no_extern_string.h` forcé par `-include` (instancie `std::string` dans nos objets, sans
  exceptions, au lieu de `string-inst.o`) : libstdc++ ne fournit plus que `new_handler.o` et `hashtable_c++0x.o`, plus
  aucun constructeur statique. (2) `blackChannel[360]` (1 440 o) supprimé : les lignes noires sont remplies par le mot
  constant `TMDS_BLACK_WORD` (stores seuls, pas de lecture — ni RAM ni flash — dans le callback de ligne de core 1).
  (3) `make -C firmware size` + `RAM_LIMIT` (ci-dessus). Le code commun (`firmware/common`) est inchangé : `neo` et
  Phosphoneo ne sont pas concernés. Correction : les « 41 Ko libres » notés le matin omettaient `.data` ; le vrai chiffre
  en 0.3.0 était 31,5 Ko.

- **0.3.0** (2026-09-20, **validé sur carte** : `mda.neo6502` en Hercules, bascules 0 → 1 et 1 → 0 à chaud depuis le BASIC) — **mode vidéo 1
  Hercules** 720×350 × 1 bpp, console 80×25 en cellules 9×14 (police MDA 8×14), attributs MDA (bits de l'encre :
  1 allumé, 2 souligné, 4 gras, 8 clignotant ; papier bit 0 = inverse — **codage remplacé en 0.4.1**, T-16), 2 pages écran, sprites/tilemaps/images en
  1 bpp (XOR) — F-51/52/53/55 du fork, **sans le mode 2** (320×256, retiré sur décision bmarty). `5,9 Set Graphics
  Mode` 0/1, `5,10 Get Graphics Mode`, encre = entrée 1 de la palette (blanc ; ambre/vert via `5,32`). Quatre
  corrections trouvées sur carte (le fork n'avait jamais tourné) : (1) division dans l'IRQ DMA du correctif PicoDVI →
  masque ; (2) encodage 1 bpp une seule fois par ligne, voies TMDS partagées (décalages par voie, extension du
  correctif PicoDVI) au lieu de trois copies (lignes en retard) ; (3) core 1 garé coopérativement, jamais réinitialisé
  ; (4) chaînage DMA coupé avant l'abandon des six canaux (un canal abandonné était relancé par son partenaire :
  signal sans image). Connu : les codes couleur 2/3/6 des messages de démarrage donnent « souligné » en mode 1
  (par conception, ils ne s'affichent qu'en mode 0).

- **0.2.1** (2026-09-19, **à valider sur carte**) — **lecture UART par blocs (10,13) via le modem USB corrigée** :
  `UARTRReadBlock` attendait les octets dans une boucle sans jamais servir l'hôte USB (`tuh_task`, appelé
  seulement par `KBDSync` entre deux appels API) : seuls les octets déjà dans le FIFO CDC (≤ 1 Ko) étaient
  lus, le reste n'arrivait jamais → timeout 5 s, erreur. Vu sur carte avec ProphetGui : `/cat` (30 octets,
  lu octet par octet) passait, `/list` (≈ 1 Ko lu en bloc) donnait « liste indisponible ». Invisible dans les
  émulateurs (pty lu directement). Correctif : `KBDSync()` dans la boucle d'attente, comme `serialmanager.cpp`.

- **0.2.1** (2026-09-19, **validé sur carte** : ProphetGui affiche le catalogue de 3617.fr) — lecture par blocs CDC : l'hôte USB est servi pendant l'attente (`cdcserial.cpp`, R16).
- **0.2.0** (2026-09-19, **validé sur carte** : `boot/auto.txt` → netinfo.neo démarre seul et obtient l'IP) — BASIC découplé et menu de démarrage.
  **BASIC découplé** (projet Neo6502Basic) : si `neobasic.bin` (= `bin/basic.bin` d'un build de `basic/`) est à la
  racine du stockage, il est chargé en `$800` au démarrage et par `1,3` à la place de la copie embarquée
  (message `NeoBASIC from storage (neobasic.bin)`) ; sinon le BASIC embarqué sert. Une nouvelle version du BASIC
  se teste donc en copiant un fichier sur la clé, sans reflasher.
  **Menu de démarrage** : si la clé a un répertoire `boot/`, ses fichiers `.neo` (programmes, exécutés à leur
  adresse d'exécution) et `.bin` (images pour `$800`, comme BASIC) sont listés au démarrage avec NeoBASIC :
  `Boot : 1 NeoBASIC  2 telemon.neo  3 …` ; une touche 1-9 choisit, Entrée ou 3 s = NeoBASIC (ou `neobasic.bin`).
  Le choix est appliqué au premier `1,3` du kernel (`jmp (0)`) ; les `1,3` suivants reviennent au BASIC.
  **Démarrage automatique** : `boot/auto.txt` contient le nom d'un fichier de `boot/` → `Boot : auto nom (Esc = menu)`,
  lancé après 1 s sauf Échap (→ menu). Un `.neo` sans adresse d'exécution ou illisible est signalé et NeoBASIC
  démarre. Complémentaire de l'`autoexec` de NeoBASIC (programme BASIC lancé par NeoBASIC lui-même).

- **0.1.0** (2026-09-19) — **TinyUSB 0.21.0** au lieu de 0.16.0 (amont) : le pilote hôte RP2040 de 0.16 lisait
  **un secteur en 2,5 s** derrière le hub 4 ports (transferts bulk sur les « interrupt endpoints », latches
  partagés — tinyusb #3533/#1261) ; avec la refonte HCD de 0.21 (EPX, double tampon) : **1 ms**. Mesuré sur
  carte (clé 8 Go FAT32, clavier, modem). Conséquences : FatFs (R0.15) copié dans `firmware/lib/fatfs`
  (TinyUSB 0.21 ne le livre plus), `tinyusb_board` retiré (BSP pour SDK 2.x, inutilisé). Messages console
  `MSC SCSI inquiry failed` / `MSC filesystem mount failed, FatFs error NN` quand une clé n'est pas montée.
  Constats carte du 2026-09-18 : alimenter la carte par un bloc secteur (le port USB-C d'un PC ne suffit
  pas : clavier/hub décrochent) ; une clé 60 Go USB 3 n'énumère pas, une 8 Go USB 2 oui.
- **0.0.1** (2026-09-18) — amont `dc70908` + modem USB CDC (F-90/F-93), message `USB serial modem found`.

## Écarté
Le mode 2 (320×256 × 16 couleurs) du fork : retiré de Trinity (décision bmarty 2026-09-19).
