# Trinity — firmware Neo6502 de référence (bmarty)

**Firmware de référence unique depuis le 2026-09-19** (le fork `bmarty/main` est archivé). Backlog : `docs/BACKLOG.md`.

Branche `trinity` (bmarty, 2026-09-18) : le firmware **amont** (`v1.0.0-14-gdc70908`, Paul Robson, MIT)
plus **une seule chose** : la reconnaissance d'un modem série USB (Pico W « picowifiusb », CDC-ACM) sur
un port USB-A de la carte — F-90 (groupe 14, `cdc.cpp`, `cdcserial.cpp`, TinyUSB `cdc_host`) et F-93
(routage des fonctions UART 10,13-10,18 vers le modem, 10,19, AUTO par défaut). Rien d'autre du fork
`bmarty/main` (toolbox, banques, modes vidéo, R22…).

Bannière : `Trinity Firmware: v0.0.1` (tag `trinity-v0.0.1` ; entre deux tags : `v0.0.1-N-gXXXXXXX`). Compilation : comme l'amont
(`make -C firmware build STORAGE=USB`, SDK 1.5.1, TinyUSB 0.16.0, PicoDVI amont non modifié).

## Versions

- **0.2.1** (2026-09-19, **à valider sur carte**) — **lecture UART par blocs (10,13) via le modem USB corrigée** :
  `UARTRReadBlock` attendait les octets dans une boucle sans jamais servir l'hôte USB (`tuh_task`, appelé
  seulement par `KBDSync` entre deux appels API) : seuls les octets déjà dans le FIFO CDC (≤ 1 Ko) étaient
  lus, le reste n'arrivait jamais → timeout 5 s, erreur. Vu sur carte avec ProphetGui : `/cat` (30 octets,
  lu octet par octet) passait, `/list` (≈ 1 Ko lu en bloc) donnait « liste indisponible ». Invisible dans les
  émulateurs (pty lu directement). Correctif : `KBDSync()` dans la boucle d'attente, comme `serialmanager.cpp`.

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

## Écarté (branche `trinity-video`)
Les modes vidéo du fork (Hercules 720×350, 320×256, pages écran — F-51/52/53/55, correctif PicoDVI) ont été
essayés sur carte le 2026-09-19 : **écran noir dès le démarrage, même en mode 0**. Conservés sur la branche
`trinity-video` pour investigation (renderer DVI multi-modes, changement de timing, `vertical_repeat`) ; pas dans Trinity.
