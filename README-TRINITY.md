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
`firmware/common/include/data/neodos_binary.h` (`kernel/scripts/hconvert.py … neodos B800` — `C000` jusqu'à la 0.6.1, NeoDOS 0.14.0 a descendu sa base, ADR-004 de Neo6502Msdos) ; même chose pour
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

- **0.10.12** (2026-09-23) — ⚠️ **la bissection de cette nuit n'a PAS trouvé la cause : elle a mesuré un effet de
  placement du code.** `bisect-I` et 0.10.12 sont fonctionnellement identiques (même taille d'UF2, même RAM, seules
  quelques instructions déplacées) et pourtant la première fait tourner `legdiag` et le jeu de façon stable, la seconde
  non. Les deux corrections ci-dessous restent justes en elles-mêmes — elles suppriment de vraies fautes — mais **rien
  ne prouve qu'elles corrigent la panne**, et les verdicts « OK / KO » des neuf flashs sont à relire comme des tirages
  dépendant du binaire, pas comme des causes. La vraie piste devient un défaut marginal en temps (boucle du bus du
  6502, IRQ USB sur core 0, cache XIP), que le moindre déplacement de code fait basculer. Ce qui a quand même été
  corrigé — depuis la 0.10.3, **tout programme mourait peu après son lancement sur la carte** (`legdiag` de NeoLegacy, le jeu lui-même), sans
  que `neo` ni Phosphoneo n'en montrent rien. Neuf flashs ont isolé **deux causes indépendantes** :
  **(1) T-46 — le sondage du curseur dans `DSPSync`.** 0.10.3 y avait ajouté `RNDCursorUpdate()`, qui vit en flash et
  s'exécute en entier ~95 fois par seconde, alors que `DSPSync` est `TIMECRITICAL` — placée en RAM — et appelée depuis la
  boucle qui sert le bus du 6502. Les deux autres fonctions qu'elle appelle respectaient la règle : `KBDSync` est
  `__time_critical_func`, `CONBlinkSync` sort immédiatement sauf deux fois par seconde. Le sondage disparaît : l'état du
  curseur est désormais **publié là où il change** (`MSESetPosition`, `MSEOffsetPosition`, `MSESetVisible`,
  `MSEInitialise`, `CURSetCurrent`, `GFXSetMode`). Le bénéfice visé par 0.10.3 est conservé — avec T-44, le callback de
  ligne ne touche ni code ni données en flash.
  **(2) T-40 retirée — les LED des touches de verrouillage.** Les allumer suppose d'envoyer un rapport de sortie HID,
  c'est-à-dire un transfert de contrôle ; émis depuis `tuh_hid_mount_cb` et depuis le callback de rapport — donc depuis
  `tuh_task` — il détruisait la pile USB hôte. C'est le terrain de T-31 : sur RP2040, TinyUSB partage l'endpoint EPX
  entre les transferts bulk du MSC et les interrupt du HID. L'état des verrous et `2,23 Get Lock Keys` **restent**
  (T-41 en dépend pour lire les touches) ; seules les diodes s'en vont, à refaire avec une demande mise en file et
  émise depuis la boucle principale, hors de tout callback.
  `make test-api` 15/15, `make test-toolbox` 10/10 ; RAM 32 544 o libres ; `~/neo-carte/trinity-0.10.12-USB.uf2`
  (**ne fonctionne pas sur carte** ; `~/neo-carte/bisect-I-sans-led-USB.uf2`, de contenu équivalent, fonctionne).

- **0.10.10** (2026-09-23, retour carte bmarty : « late plante à nouveau ») — **outil `LATE` réellement reconstruit**.
  La 0.10.5 avait retiré de `late.asm` son journal de débogage en RAM **et, par mégarde, la routine `cr`** qu'il utilise
  encore ligne 38 : l'assemblage échouait, `LATE.NEO` n'a donc jamais été régénéré et la clé a gardé la version qui se
  plante. `cr` est rétablie, et les outils carte ont désormais une cible de construction, **`make outils`**
  (`tests/api/outils/*.asm` → `~/neo-carte/cle-usb/*.NEO` via `mkneo.py` de Neo6502Msdos), pour qu'un binaire périmé ne
  puisse plus se faire passer pour une correction. `LATE.NEO` 198 o, `TZPARIS.NEO` inchangé. Firmware inchangé.

- **0.10.9** (2026-09-22, piste pour la régression de NeoLegacy sur carte) — **l'image du curseur ne sort plus de la flash
  dans le callback DVI (T-44)**. 0.10.3 a sorti du callback de ligne les *appels de fonction* en flash, mais **pas les
  données** : `CURGetCurrent` renvoie un pointeur dans `cursor_data`, tableau `const` donc en flash, et le callback y
  lisait un octet par pixel de curseur — jusqu'à 256 accès XIP par trame, sur le cœur dont la ligne doit être encodée à
  l'heure, pendant que core 0 martèle la flash. C'est la contention que T-38 traque. Elle ne se voyait pas tant que le
  curseur restait caché (il l'est au reset depuis 0.9.3) ; **NeoLegacy 0.29.1 est le premier programme à l'afficher**
  (`11,2`). `RNDCursorUpdate` copie désormais l'image en RAM (256 o, seulement quand le curseur change) et le callback ne
  lit plus que de la RAM. Mesurable sur carte : à curseur visible, `5,40 Get Late Scanlines` (outil `LATE.NEO`) doit
  donner moins de lignes en retard qu'en 0.10.8. **C'est une hypothèse sur la panne de NeoLegacy, pas une preuve** :
  seule la carte tranchera. RAM 32 508 o libres (−256 o) ; `~/neo-carte/trinity-0.10.9-cursor-ram-USB.uf2`.

- **0.10.8** (2026-09-22, revue du code des 0.10.x en cherchant la régression de NeoLegacy) — **état du curseur publié
  d'un bloc (T-43)**. `RNDCursorUpdate` (0.10.3) écrivait ses sept champs un par un dans des variables `volatile` que
  core 1 recopiait au début de trame : core 1 pouvait lire un **mélange de deux états** — une position neuve avec une
  largeur ancienne, ou `enabled` alors que le pointeur d'image n'était pas encore écrit — là où l'ancien code (0.10.2 et
  avant) calculait tout d'un bloc dans le callback. Deux emplacements désormais (`cursorSlot[2]`) : core 0 remplit celui
  que core 1 ne lit pas, puis publie l'index par une écriture d'un octet, atomique sur le M0+ et précédée d'un `__dmb()`.
  Le curseur n'est plus jamais dessiné depuis une image nulle. Même réserve que 0.10.7 : fichier compilé pour la carte
  seulement, aucun test automatique, à valider carte.
  `~/neo-carte/trinity-0.10.8-cursor-slot-USB.uf2`, RAM 32 760 o libres.

- **0.10.7** (2026-09-22, trouvé en cherchant la régression de NeoLegacy sur carte) — **curseur souris aux bords de l'écran
  (T-42)**. `RNDCursorUpdate`, introduite en 0.10.3 pour sortir le calcul du curseur du callback de ligne, faisait ses
  soustractions en **non signé** : `x -= xHit` (le point chaud) passe à ~65530 dès que le pointeur approche le bord gauche
  ou haut, et `w = xGSize - x` donnait 1 726 au lieu d'une largeur tronquée. Les gardes du callback rejetaient ces valeurs,
  donc le curseur **disparaissait** au lieu d'être découpé (aucune corruption mémoire : les tampons de ligne ont assez de
  marge). Le calcul est désormais signé et découpe les quatre bords : `skipX`/`skipY` disent combien de colonnes et de
  lignes de l'image du curseur sauter, `w`/`h` ce qui reste, et le curseur est désactivé s'il sort complètement — modes 0
  et 1. Ce fichier n'est compilé que pour la carte (ni `neo` ni Phosphoneo ne l'ont), donc **aucun test automatique ne le
  couvre** : vérifié par compilation, à valider carte.
  `make test-api` 15/15, `make test-toolbox` 10/10 (inchangés : aucun ne voit ce fichier).
  `~/neo-carte/trinity-0.10.7-cursor-clip-USB.uf2` (435 712 o), RAM 32 764 o libres.

- **0.10.6** (2026-09-22, retour carte bmarty : « num lock ou caps lock ne rendent pas leur service ») — **les touches de
  verrouillage agissent enfin (T-41)**. La 0.10.5 n'allumait que les diodes : l'amont ne consulte jamais les verrous
  (`KBDMapToASCII` ne regarde que Shift) et remappait le pavé numérique sur les chiffres **sans condition**, en laissant
  `/ * - + Entrée .` sans correspondance — touches mortes. Désormais :
  **Caps Lock** inverse la casse des **lettres seulement** (`a-z` ↔ `A-Z`, appliqué après la table de locale : les chiffres,
  les symboles et les caractères accentués du clavier FR ne changent pas — décision bmarty) ;
  **Num Lock** commande le pavé : allumé, les chiffres et `.` comme avant ; éteint, la navigation imprimée sur les touches
  (KP1 Fin, KP2 ↓, KP3 PgSuiv, KP4 ←, KP5 rien, KP6 →, KP7 Origine, KP8 ↑, KP9 PgPréc, KP0 Inser, KP. Suppr) ;
  `/ * - +` du pavé donnent leur caractère quels que soient Shift et la locale, et son Entrée vaut celle du clavier.
  **Le firmware démarre Num Lock éteint** (décision bmarty) ; Scroll Lock reste un simple état avec sa diode.
  L'état des verrous a quitté `usbdriver.cpp` pour `keyboard.cpp` (commun au firmware et à `neo`, donc testable) ; le pilote
  USB ne transmet plus que les scancodes bruts et porte l'état aux diodes (`KBDLockLEDUpdate`). Le mappage choisi à l'appui
  est rejoué au relâchement, pour qu'une bascule de Num Lock touche enfoncée ne colle pas une touche.
  `2,23 Get Lock Keys` est inchangée mais sa documentation dit maintenant ce que les verrous commandent.
  Côté `neo` : nouveau crochet de test **`hid:C:LISTE`** (scancodes HID bruts, pour les touches sans ASCII) et les touches
  de verrouillage et du pavé ajoutées à la table SDL→HID (`emulator/scripts/mapper.py`). Test `tests/api/locks.asm`
  (`KEYS 61 41 17 35 2B 2F 0D 61 / LOCK 01`). `make test-api` 15/15, `make test-toolbox` 10/10.
  RAM 32 816 o libres (`padDown` : 16 o) ; UF2 435 200 o, `~/neo-carte/trinity-0.10.6-locks-USB.uf2`.

- **0.10.5** (2026-09-22, retours carte bmarty : `LATE` planté après 20 s, Num/Caps Lock n'allument rien) — **LED des touches
  de verrouillage (T-40)** : le firmware amont ne renvoyait jamais le rapport de sortie HID, donc les diodes Num/Caps/Scroll
  Lock restaient éteintes ; l'état est tenu par le firmware (basculé à chaque appui), envoyé au clavier
  (`tuh_hid_set_report`) et lisible par **`2,23 Get Lock Keys`** (bit 0 Num, 1 Caps, 2 Scroll ; à ce stade la lecture des
  caractères n'en dépendait pas — c'est la 0.10.6 qui l'a corrigé). **Outil `LATE` corrigé** : sa temporisation écrasait le registre X (boucle à vide) et son journal de
  débogage en RAM — utile seulement sous `neo` — finissait par écraser la mémoire ; il attend maintenant le timer `1,1`
  et n'écrit plus rien. `~/neo-carte/trinity-0.10.5-leds-USB.uf2`. (La note disait « `cle-usb/LATE.NEO` régénéré » : c'était faux —
  l'assemblage était cassé et le binaire est resté périmé jusqu'à la 0.10.10.)

- **0.10.4** (2026-09-22, demande bmarty : « prendre en charge le driver pour 1A2C 0B2A », message `No driver found` au
  démarrage) — **touches multimédia du clavier USB (T-39)**. Relevé sur le PC : ce clavier (China Resource Semico) expose
  deux interfaces HID — **0** boot keyboard (déjà gérée) et **1** sans protocole boot, portant une collection *Consumer
  Control* (page d'usage `$0C`, report ID 1, usage 16 bits) et une *System Control* (report ID 2 : sleep, power, wake).
  Faute de protocole, elle tombait dans le gestionnaire de manettes, qui n'avait que « No driver found » à en dire.
  Le clavier la revendique désormais d'après son descripteur (`KBDMediaClaim`), consomme ses rapports, et la dernière
  touche est lisible par **`2,22 Get Media Key`** (P0-1 = usage Consumer — `$00E9` volume +, `$00EA` volume −, `$00E2`
  sourdine, `$00CD` lecture/pause… —, P2 = bits système ; effacés à la lecture). Message `USB media keys found`.
  `~/neo-carte/trinity-0.10.4-media-USB.uf2`.

- **0.10.3** (2026-09-22, les traits rouges de 0.10.2 ont « un peu diminué, pas assez ») — deux mesures : (1) le callback
  de ligne DVI (core 1) n'appelle plus **aucun code en flash** : les informations du curseur souris (`MSEGetCursorDrawInformation`,
  `CURGetCurrent`, en flash) sont préparées par core 0 dans `DSPSync` et publiées en RAM (`RNDCursorUpdate`, T-32c) — un accès
  XIP depuis core 1 s'allonge fortement quand core 0 martèle la flash (USB, FatFs, console), d'où des lignes non prêtes ;
  (2) **`5,40 Get Late Scanlines`** expose le compteur de lignes en retard de PicoDVI, pour mesurer au lieu d'estimer
  (outil `LATE.NEO` / `late.neo6502`, boucle d'affichage, Échap pour sortir). `~/neo-carte/trinity-0.10.3-scanline-USB.uf2`.

- **0.10.2** (2026-09-22, retour carte bmarty : traits rouges nombreux à la frappe et à l'affichage) — le **lockout
  multicore du SDK** (0.9.10, pour les écritures flash) installait sur core 1 un gestionnaire d'IRQ (FIFO inter-cœurs)
  **situé en flash** : chaque interruption volait des cycles à l'encodeur TMDS, d'où des lignes non prêtes à temps (traits
  rouges). Remplacé par un **parking coopératif** : core 1 se gare lui-même dans sa boucle (code en RAM), IRQ DMA coupée,
  le temps de l'écriture (`RNDFlashPause`/`RNDFlashResume`) — plus aucune IRQ supplémentaire sur core 1, le DVI n'est pas
  démonté (image figée puis reprise, sans re-verrouillage du moniteur). `~/neo-carte/trinity-0.10.2-core1-USB.uf2`.

- **0.10.1** (2026-09-22, retour carte bmarty sur 0.10.0 : `USB settled (500 ms)` puis `USB Key found` et `Volume 0:`
  **après** NeoDOS, Échap inopérant, clavier muet ~20 s) — la barrière attendait « 300 ms sans événement », condition déjà
  vraie **avant** le début de l'énumération : elle sortait à son plancher. Elle attend maintenant un **premier événement**
  (ou le plafond, ramené à 4 s), puis le calme ; message `USB settled (N ms, D dev)`.

- **0.10.0** (2026-09-22, **à valider sur carte** : `~/neo-carte/trinity-0.10.0-boot-USB.uf2`) — **boot déterministe, étape (a)
  de l'ADR-0001** (T-32) : `DSPReset` est découpé en phases — **P0** matériel et état du firmware (aucune E/S externe),
  **P1** découverte USB jusqu'à la **barrière de calme** (`usbsettle.cpp` : chaque `mount`/`umount` HID, MSC et CDC horodate
  un événement ; P1 se termine après 300 ms sans événement, avec un plancher de 500 ms et un plafond de 5 s, message
  `USB settled (N ms)`), **P2** politique (fuseau, catalogue `boot/`, menu), **P3** UEXT puis boucle bus. L'attente de 2 s
  sur la première clé (`STOSynchronise`, heuristique de l'amont) est supprimée : le démarrage ne dépend plus du nombre de
  périphériques ni de leur ordre d'énumération — ce qui rend structurels les correctifs ponctuels T-24 (volume), T-28
  (clavier avant Échap) et le gel de 0.9.5-0.9.7. `make test-api` 14/14, `make test-toolbox` 10/10. RAM 32 840 o libres.

- **0.9.11** (2026-09-22, **validée sur carte** : `TZPARIS.NEO` règle le fuseau et synchronise l'heure, écran conservé, clavier et Échap immédiats ; retour carte bmarty : après `TZPARIS` sur 0.9.10, écran conservé mais **clavier muet pendant ~10 s**,
  périphériques toujours détectés) — pendant l'écriture flash les interruptions sont coupées sur les deux cores : l'hôte USB
  perd les siennes et TinyUSB ne se resynchronise qu'après une dizaine de secondes. `HWUSBRecover()` (T-30) pompe
  `tuh_task` pendant 300 ms juste après l'écriture (et `KBDSync()` juste avant) : la reprise est immédiate.
  `~/neo-carte/trinity-0.9.11-USB.uf2`.

- **0.9.10** (2026-09-22, retour carte bmarty : après `TZPARIS` (écriture du secteur de réglages) l'écran reste noir) — les
  écritures flash (`1,22 Write Bank`, `1,24`) ne démontent plus le DVI (`RNDSuspend`/`RNDResume` : pour un même mode, le
  moniteur ne re-verrouillait pas) : **lockout multicore du SDK** (`multicore_lockout_victim_init` sur core 1 au démarrage,
  `multicore_lockout_start/end_blocking` autour de l'effacement/programmation, IRQ coupées sur les deux cores ≈ 50-150 ms) —
  l'image se fige quelques trames, l'horloge DVI continue. `RNDSuspend`/`RNDResume` restent disponibles mais inutilisés.
  `~/neo-carte/trinity-0.9.10-USB.uf2`.

- **0.9.9** (2026-09-22, demande bmarty : « No mouse cursor overlay in monochrome ») — **curseur souris en mode 1 Hercules**
  (T-29) : superposition du curseur 16×16 sur la ligne 1 bpp dans le callback DVI (couleur 0 = pixel éteint, autre couleur =
  allumé, `$FF` transparent), même contrat qu'en mode 0 (`11,2` pour l'afficher). `neo` le faisait déjà.
  `~/neo-carte/trinity-0.9.9-USB.uf2`.

- **0.9.8** (2026-09-22, bissection carte : la variante sans synchro automatique démarre avec la clé, 0.9.7 non) — **plus de
  synchro automatique de l'heure** : `1,20` ne consulte jamais le modem ; seul `1,23 Sync Clock From Modem` le fait, à la
  demande du programme (remarque bmarty : « tant que l'on ne lit pas la date, pourquoi l'initialiser ? »). La cause du gel
  (échange AT depuis la sonde `1,20` de NeoDOS au démarrage, clé présente) reste **ouverte** : à reproduire hors démarrage
  avec `cle-usb/clocksync.neo6502` (`1,23` puis `1,20` puis `14,4`). Proposition NeoDOS : `DATE`/`TIME` appellent `1,23`
  quand la source est 0. `~/neo-carte/trinity-0.9.8-USB.uf2`.

- **0.9.7** (2026-09-22, retour carte bmarty : 0.9.6 bloquée sur `USB Storage` clé insérée, OK sans clé) — la sonde `1,20` de
  NeoDOS au démarrage lançait l'échange AT pendant l'énumération USB ; le montage de la clé (imbriqué dans l'attente de
  l'échange, qui sert l'hôte USB) figeait la carte. La synchro automatique n'est plus tentée dans les **10 premières
  secondes** après le reset ; `1,23` explicite inchangé. `~/neo-carte/trinity-0.9.7-USB.uf2`.

- **0.9.6** (2026-09-22, remarque bmarty : « ne pas jeter les messages ») — **tampon de report CDC** (`cdcserial.cpp`,
  256 o, périphérique 0) : ce que le firmware lit du modem pour ses propres échanges (synchro de l'horloge) et qui n'est
  pas sa réponse (`ready`, `WIFI GOT IP`, `+IPD…`) est rendu au programme : les lectures du groupe 14 et du routage UART
  servent d'abord ce tampon, puis le FIFO (`CDCRead`, `CDCReadAvailable`). L'échange AT lit le FIFO en direct (sinon il
  relirait son propre report). La synchro ne part toujours que sur une **lecture** de la date (`1,20`) ou `1,23`, jamais
  d'elle-même ; ce qu'on voit au démarrage est la sonde `1,20` de NeoDOS. Test `clocksync` : le modem factice émet
  `WIFI GOT IP` avant sa réponse, relu intact par `14,4`. `make test-api` 14/14. `~/neo-carte/trinity-0.9.6-pushback-USB.uf2`.

- **0.9.5** (2026-09-21, retour carte bmarty : « la date est toujours en 1970 ») — synchro modem rendue robuste : (1) la
  tentative automatique exigeait une liaison CDC inactive, or le Pico W émet des messages non sollicités au démarrage qui
  restent dans le FIFO → condition retirée tant que l'horloge n'est pas réglée (l'entrée en attente est jetée) ; (2) le SNTP
  du modem est **désactivé d'usine** : si `AT+CIPSNTPCFG?` répond `0`, le firmware l'active lui-même
  (`AT+CIPSNTPCFG=1,0,"pool.ntp.org"`, sauvé par le modem) et l'heure vient à la tentative suivante (≤ 30 s après la
  connexion Wi-Fi). `CLKModemCommand` factorise l'échange AT. Modem factice de test complété (`+CIPSNTPCFG:1,0,…`).
  `~/neo-carte/trinity-0.9.5-sntp-fix-USB.uf2`.

- **0.9.4** (2026-09-21, retour carte bmarty : « le 3 s Esc ne fonctionne pas ») — le clavier USB n'est pas encore énuméré
  quand la fenêtre s'ouvre (le hub monte la clé, le modem, puis le clavier). Le menu attend maintenant qu'un **clavier soit
  monté** (message `USB keyboard found`, `KBDIsPresent`, jusqu'à 8 s) puis ouvre les 3 s pour Échap (T-28).
  `~/neo-carte/trinity-0.9.4-esc-kbd-USB.uf2`.

- **0.9.3** (2026-09-21) — curseur souris : l'automatisme de 0.9.1 (T-27) est **retiré** sur décision bmarty : le curseur reste
  caché au reset et n'apparaît que par `11,2` (c'est au programme de le montrer). `~/neo-carte/trinity-0.9.3-USB.uf2`.

- **0.9.2** (2026-09-21) — menu de démarrage : **3 s** (au lieu de 1 s) pour appuyer sur Échap quand `boot/auto.txt` lance une
  entrée (demande bmarty : le clavier USB s'énumère pendant ce délai derrière le hub) ; message `(Esc = menu, 3 s)`.
  `~/neo-carte/trinity-0.9.2-esc-USB.uf2`.

- **0.9.1** (2026-09-21, **à valider sur carte** : `~/neo-carte/trinity-0.9.1-timezone-USB.uf2`) — **Fuseaux horaires (T-26)**,
  demande bmarty (« Europe/Paris, Europe/Belgrade… pas nécessairement la France ») : l'horloge garde l'**UTC** ; `1,20`,
  `1,21` et les horodatages FAT sont en heure locale du fuseau choisi. **`1,24 Set Time Zone`** : nom de style IANA parmi
  ~120 zones en flash (`timezone.cpp` : Europe, Afrique, Amériques, Asie, Océanie) ou décalage fixe (`UTC+2`, `UTC-3:30`,
  `+0530`) ; règles d'heure d'été Union européenne, Amérique du Nord, Australie, Nouvelle-Zélande (état 2026, sans
  historique) ; **`1,25 Get Time Zone`** (nom, décalage courant en minutes, été en cours). Réglage **persistant sur la carte**
  (pas sur la clé, remarque bmarty) : nouveau **secteur de réglages en flash** (`settings.cpp`, 4 Ko à `0x1BF000` sous les
  banques, enregistrement `NST1`, écrit par `1,24` seulement au changement, DVI suspendu ≈ 100 ms ; `neo` :
  `storage/settings.flash`), appliqué au démarrage (`Time zone Europe/Paris`). `1,23` lit aussi `AT+CIPSNTPCFG?` et retranche
  le `tz` du modem : l'heure devient UTC quel que soit le réglage du Pico W. **Curseur souris automatique (T-27)** : le curseur
  apparaît au premier mouvement de la souris tant qu'un programme n'a pas appelé `11,2` (reset = automatisme). Tests
  `timezone.asm` (Paris été, sync modem 12:34:56 UTC → 14:34:56, réglage local relu, Montréal en décembre −300, zone
  inconnue, `UTC-3:30`), `clocksync` (correctif : initialisation de l'horloge avant écriture). `make test-api` 14/14,
  `make test-toolbox` 10/10. RAM 33 372 o libres (table des zones en flash, enregistrement 256 o) ; UF2 429 568 o.

- **0.9.0** (2026-09-21, **à valider sur carte** : `~/neo-carte/trinity-0.9.0-sntp-USB.uf2`) — **Heure par le modem (T-25)**,
  demande bmarty (`DATE` = 1970 sans RTC). `1,23 Sync Clock From Modem` : `AT+CIPSNTPTIME?` sur la CDC (1 s max, hôte
  USB servi), réponse `+CIPSNTPTIME:Www Mmm dd hh:mm:ss yyyy` analysée (`CLKParseModemTime`), 1970 = pas encore d'heure
  (erreur 2), pas de modem (erreur 1) ; entrée en attente sur la liaison jetée. **Automatique** : `1,20` avec horloge non
  réglée et modem présent interroge le modem (au plus toutes les 30 s, seulement liaison inactive) — NeoDOS appelle `1,20`
  au démarrage, l'heure suit dès que le modem a son SNTP. Source `1,20` P7 = 3. **Fuseau** : réglé dans le modem
  (`AT+CIPSNTPCFG=1,tz,"serveur"`, heures entières, sauvé dans sa flash, pas de règle été/hiver) ; le firmware reçoit
  l'heure locale, cohérent avec les horodatages FAT. Test `tests/api/clocksync.asm` + modem factice sur pty
  (`tests/tools/fake_sntp_modem.py`, `NOM.modem` dans `run_neo.sh`) ; `neo` modélisant un PCF8563, la source y vaut 2.
  `make test-api` 13/13. RAM 34 304 o libres ; UF2 415 744 o.

- **0.8.3** (2026-09-21, **validé sur carte** le soir même : `Volume 0: (A:)`, invite `A:\>`, clavier, NeoBASIC lancé ; `DATE` = 1970 sans RTC, normal. **0.8.2 sur carte : gel** — invite `A:\>` affichée puis clavier mort, Échap au boot inopérant) —
  **correctif du correctif** : le diskio USB posait le drapeau « occupé » sur l'index lecteur (`pdrv`, désormais 0) et la fin
  de transfert TinyUSB l'effaçait sur l'index adresse USB (`dev_addr`) ; égaux jusqu'en 0.8.1, différents depuis le montage
  de la première clé en `0:` → le premier accès disque attendait sans fin dans `tuh_task` (le clavier était lu, jamais
  servi au 6502). Drapeau indexé par `dev_addr` partout (`usb_storage.cpp`). UF2 `~/neo-carte/trinity-0.8.3-diskio-fix-USB.uf2`.

- **0.8.2** (2026-09-21, **carte : 0.8.1 flashée le jour même**, retour bmarty en NeoDOS : invite `!:\>`, `A:` refusé, `B:` →
  `b:1:\>`) — **volumes sur carte corrigés (T-24)** : (1) l'amont montait une clé USB à son **adresse USB** (`1:` derrière
  le hub) → pas de volume 0, `A:` inexistant ; désormais la n-ième clé montée est le lecteur logique `n-1` (`0:` = `A:`,
  table lecteur ↔ adresse dans `usb_storage.cpp`, diskio traduit), message `Volume 0: (A:)` au montage ; (2) `3,26` échouait
  avant le premier montage (`f_getcwd`) et renvoyait une variable **non initialisée** (`'A' + $E0` = `!`) → volume courant
  suivi par le firmware (`FISNoteCurrentVolume`, `FISSelectVolume`), `v = 0` par défaut dans le dispatch ; (3) `3,23`
  renvoyait `1:/…` brut sur carte alors que les émulateurs donnent `/…` relatif au volume → préfixe `n:` retiré.
  **Volume par canal son (T-22, F-12 du fork)** : `8,9 Set Channel Volume` (0-100, rampe en 1/100 s, sans redémarrer
  l'onde), `8,10 Get Channel Volume` ; test `sndvol.asm` du fork (regex sur la valeur de rampe). `make test-api` 12/12.
  RAM 34 340 o libres ; UF2 413 696 o (`~/neo-carte/trinity-0.8.2-volumes-fix-USB.uf2`).

- **0.8.1** (2026-09-21, **à valider sur carte** : `~/neo-carte/trinity-0.8.1-rtos-USB.uf2`) — **Noyau RTOS 6502 (T-14 b, F-61 du
  fork)** : `kernel/rtos.asm` + `rtos_data.asm` (TCB en `$FF10-$FF6D`) compilés dans le noyau (`$FC00-$FEFB`, 5 octets de
  marge avant `$FF00`) : 4 tâches (pile `$0100` en 4 × 64 o, page zéro privée `$E0-$EF`), tourniquet sur le tick 1,12,
  `KTaskInit/Create/Yield/Sleep/Exit/Lock/Unlock/Ticks`, `KSemWait/Signal` (vecteurs `$FFC1-$FFDC`, `neo6502.inc`
  régénéré), `WAI` + lecture de `$FFFF` quand rien n'est prêt ; `$FFFE` → `KIrqHandler` (reset si l'ordonnanceur est
  inactif, comme l'amont). L'API `$FF00` n'est pas réentrante : `KTaskLock`/`KTaskUnlock` autour des appels ; NeoBASIC
  incompatible (page zéro, pile) ; NeoDOS en `$B800` n'est pas concerné. Tests `tests/api/rtos.asm` et `rtos_idle.asm`
  (démos Phosphoneo du fork, coupées par `cycles:`) : sortie **identique à la référence golden du fork**
  (`AAAAAT=0032 ABAAAAT=0064 ABAAAAT=0096 AB…`). **Titres de fenêtre sans limite (T-21)** : demande d'un autre projet
  (`WM_TITLE_MAX` 31) — le Window Manager lit le titre **en place** dans la RAM 6502 (pointeur + longueur, comme les
  menus) au lieu de le copier : jusqu'à 255 caractères, −248 o de RAM ; le programme garde la chaîne intacte tant que
  la fenêtre existe (`34,1`, `34,9`, documenté). `make test-api` 11/11, `make test-toolbox` 10/10. RAM 34 492 o libres ;
  UF2 412 160 o.

- **0.8.0** (2026-09-21, **à valider sur carte avec prudence** : `~/neo-carte/trinity-0.8.0-irq-USB.uf2`) — **Tick d'interruption
  et IRQ de trame (T-14 a, F-60/F-10 du fork)** : `1,12 Set Interrupt Tick` (1-1000 Hz, timer matériel du RP2040 sur
  core 0), `1,13`, `1,16 Set Frame Interrupt` (IRQB au début de chaque trame, posée par le callback de ligne DVI sur core 1),
  `1,17`. IRQB (GPIO 25) est tenue basse jusqu'à la lecture du vecteur `$FFFF` par le 65C02, vue par la boucle bus
  (`processor_pio.cpp` : test sur l'adresse à chaque lecture, **marge de nop réduite de 14 à 11** comme dans le fork — jamais
  mesuré sur carte) ; pas d'acquittement, pas de réentrée, ticks fusionnés sous `SEI`. Reset : tick et IRQ de trame
  arrêtés. `neo` : IRQ par cycles et par trame, relâchée sur `$FFFF`, opcode **`WAI`** (`$CB`) — corrigé par rapport au
  fork : l'IRQ prise pendant un `WAI` reprend à l'instruction suivante, pas sur le `WAI`. Tests `tests/api/` : `frameirq.asm`
  du fork (identique : 60 trames comptées, arrêt vérifié) et `irqtick.asm` nouveau (100 Hz ≈ 100 ticks/s, `WAI`, arrêt,
  2000 Hz refusé ; attendu en regex). `make test-api` 9/9, `make test-toolbox` 10/10. RAM 34 284 o libres ; UF2 410 624 o.
  **Carte** : première IRQ jamais délivrée au 65C02 par ce firmware ; à vérifier d'abord que le mode 1 Hercules et le modem
  CDC fonctionnent encore (timing de la boucle bus), puis `frameirq.neo6502` et `irqtick.neo6502`.

- **0.7.1** (2026-09-21, **à valider sur carte** : `~/neo-carte/trinity-0.7.1-latin1-USB.uf2`) — **Latin-1, locale FR, police
  console, écho de débogage (T-20, F-17/F-95/F-92 du fork)**. Console : caractères `$A0-$BF` = symboles Latin-1 en flash
  (`latin1font.h` généré par `scripts/latin1.py` depuis `font_5x7.h`), `$C0-$FF` = police utilisateur initialisée aux lettres
  accentuées au reset (`2,5` les remplace), `CONGlyph` unique pour la console, Draw Text et QuickDraw ; `fr.locale` = AZERTY
  PC en Latin-1 (l'ancienne disposition Apple devient `fm.locale`), codes 160-255 admis par `keymaps.py`. **`2,21` Set
  Console Font** : glyphes `$20-$7F` lus en place dans la RAM 6502 (8 lignes, et 14 lignes pour les cellules 9×14 du mode 1),
  rétablis par 5,9 et au reset. **`2,20` Console Debug Echo** : le texte console part aussi sur l'UART de débogage
  (stderr dans `neo`). `neo` : tampon d'arguments 512 o, sortie `cycles:` une seule fois. Tests `tests/api/` :
  `latin1.asm` (frappe AZERTY injectée `keys:2790\`#;` → `é è ç à ² 3 m`), `confont.asm`, `confont14.asm` du fork :
  sorties identiques ; `events` : attendu en regex (position souris hôte). `make test-api` 7/7, `make test-toolbox` 10/10.
  RAM 34 556 o libres ; UF2 409 600 o.

- **0.7.0** (2026-09-21, **à valider sur carte** : `~/neo-carte/trinity-0.7.0-banks-xip-USB.uf2`) — **Banques mémoire en
  flash (XIP), T-17** (décision bmarty : « je veux le XIP pour les banques »). Reprise de F-23 du fork avec le stockage en
  flash au lieu de la SRAM : **32 banques de 8 Ko** dans les 256 Ko du haut des 2 Mo (`0x1C0000`), lues en place ;
  `1,18 Select Bank` = copie flash → fenêtre 6502 (page alignée, sous `$FF00`), **sans write-back** (la fenêtre est une
  copie) ; `1,19 Get Bank Info` ; **`1,22 Write Bank`** (nouveau) : programme une banque depuis une fenêtre — sur carte,
  DVI suspendu (`RNDSuspend` : core 1 garé en RAM, DMA/PIO coupés, écran noir ≈ 150 ms), IRQ coupées, `flash_range_erase`
  + `flash_range_program` (`hardware_flash`), vérification, `RNDResume` ; une banque écrite survit aux resets et aux
  reflashages du firmware. Pages blitter `$A0`-`$BF` = banques **en lecture seule** (`12,2` banque → VRAM ; cible banque
  et `3,27` vers une banque refusés). `neo` : flash simulée dans `storage/banks.flash` (256 Ko, persistant). Test
  `tests/api/banks.asm` (`make test-api` 4/4). RAM : −708 o (routines flash résidentes en RAM du SDK), **34 636 o libres** ;
  UF2 406 528 o (le firmware occupe 0x63400, loin de la zone des banques). **Sur carte, à valider avec prudence** :
  première écriture flash sous DVI du projet (le fork l'avait écartée sans carte).

- **0.7.0, même livraison** (2026-09-21 ; préparé comme 0.6.2 mais entraîné dans le commit 0.7.0 par la session banques) — **NeoDOS 0.14.0 embarqué, chargé en `$B800`** (T-15 suite ; demande
  bmarty : NeoDOS a descendu sa base de `$C000` à `$B800` pour retrouver 2,5 Ko de marge, ADR-004 de Neo6502Msdos, et
  externalisé `ATTRIB`). `hconvert.py … neodos B800` dans les trois Makefiles (firmware, common, emulator) : `NEODOS_LOAD = 0xb800`, `NEODOS_SIZE =
  0x27f1` (10 225 o) ; `MEMInitialiseMemory` et `1,3` pointent `jmp (0)` sur `$B800`. Programmes 6502 : `$0800-$B7FF`.
  Sans cette reprise, `EXIT` de NeoDOS 0.14.0 relançait l'ancienne image 0.12.0 en `$C000` à côté de la nouvelle.
  Vérifié dans `neo` : bannière `NeoDOS version 0.14.0`, `MEM` = 45 056 / 17 408, `EXIT` relance la 0.14.0.

- **0.6.1** (2026-09-21, émulateur seulement) — **`neo` : crochets de test** (T-19, repris du fork sans sa partie IRQ) :
  arguments `cycles:N` (sortie + `memory.dump`), `shot:C:FICHIER` (capture PPM), `text:C:FICHIER` (texte console),
  `keys:C:TEXTE` (frappe automatique, `\n` = Entrée), `mouse:C:X,Y,B` (souris et boutons). `tests/toolbox/run_neo.sh`
  lit `NOM.args` ; `events.asm` (groupe 33, souris + « ab ») est automatisé : sortie identique au fork.
  `make test-toolbox` 10/10, `make test-api` 3/3.

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
