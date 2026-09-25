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

Bannière : `Trinity Firmware: v0.0.1` (tag `trinity-v0.0.1` ; entre deux tags : `v0.0.1-N-gXXXXXXX`).
**Ordre à respecter pour une livraison** : commit, **puis tag**, **puis** `make -C firmware build`, **puis** copie de
l'UF2 dans `~/neo-carte/`. La bannière est tirée de `git describe` **au moment de la compilation** : compiler avant de
taguer produit une image qui affiche la version précédente (c'est arrivé aux 0.10.12 et 0.11.3). Et comme T-48 montre
que le comportement carte dépend du placement du code, recompiler après coup donne un **binaire différent** : ce n'est
pas un détail cosmétique. Compilation : comme l'amont
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

- **0.16.5** (2026-09-25, firmware de diagnostic) — **balayage des broches UEXT (T-75)**. Après deux défauts réels
  corrigés (0.16.3 et 0.16.4) et toutes les permutations de câblage, le port restait muet sans qu'on puisse
  départager « le fil n'est pas sur la bonne broche » de « il reste un défaut » — et sans multimètre pour interroger
  la carte. La question est donc renversée : **c'est le firmware qui dit où il parle**. Pendant les 30 premières
  secondes, il déplace la fonction UART TX d'une broche UEXT à la suivante, une par seconde, en annonçant
  `UEXT GPIO=nn`. Le fil ne bouge pas ; l'annonce qui arrive nomme la broche.
  Candidates : 28 et 29 (l'UART), puis 22 à 27 (I2C et SPI du firmware — les piloter une seconde est sans danger,
  rien n'étant branché). L'écriture va droit au matériel avec `uart_tx_wait_blocking`, car le message doit avoir
  quitté le registre à décalage avant que la broche ne change sous lui ; cette attente bloquante est la raison pour
  laquelle le balayage ne tourne **que** pendant ces 30 s. Quatre passes, puis retour au port normal sur 28/29.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 28 548 o libres.

- **0.16.4** (2026-09-25) — **le port de debug n'est plus éteint en P3 (T-74)**. Le port restait muet pour une
  seconde raison, la vraie : `GPIOMapping[]` associe les broches UEXT du 6502 aux GPIO du RP2040, et ses entrées 3 et
  4 sont **28 et 29**, c'est-à-dire l'UART. `IOInitialise()` tourne en **P3**, après `DBGInitialise()` en P0, et remet
  chaque broche de cette table en entrée — or `gpio_init()` réinitialise la fonction du GPIO en SIO et **détruit la
  fonction UART**. Le port était donc coupé à chaque démarrage, et aucun câblage n'y aurait rien changé. La table
  prévoyait le cas (`IOPINDisabled`, « probably in use by SPI, Serial, I2C ») : `DBGOwnsGPIO()` réserve les deux
  broches et `IOInitialise` les saute.
  La trouvaille vient d'un **test de bouclage** de l'adaptateur : `NEOTEST-12345` renvoyé intact prouvait que le
  matériel, le pilote et le débit étaient bons, ce qui obligeait à chercher dans le firmware plutôt que dans le
  câble, le brochage ou le schéma de la carte. Sans lui, la recherche partait pour des heures au mauvais endroit.
  Conséquence : les broches 3 et 4 de l'UEXT ne sont plus offertes au 6502 tant que le port de debug est actif.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 28 584 o libres.

- **0.16.3** (2026-09-25) — **le port de debug recalcule son débit après un changement de mode (T-74)**. Le port
  restait muet, et la cause était dans le firmware, pas dans le câble : `DVIStart` appelle `set_sys_clock_khz` (252
  ou 270 MHz selon le mode vidéo), `clk_peri` suit, et le diviseur UART calculé en P0 cesse d'être juste dès que le
  premier mode démarre. `serial.cpp` connaît ce piège et le règle par `SERClockChanged` — mais **seulement pour un
  port que le 6502 a ouvert** (`currentBaudRate != 0`), ce qui n'est jamais le cas du port de debug, qui
  s'initialise de lui-même. D'où `DBGClockChanged()`, accroché à `HWClockChanged()` aux côtés de `SERClockChanged()`
  et `SNDClockChanged()`.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 28 584 o libres.

- **0.16.2** (2026-09-25, demande bmarty) — **rapports `!u` usb et `!f` stockage (T-74)**. `!u` donne la barrière
  d'énumération, le nombre d'événements vus et la présence du clavier ; `!f` donne, pour chaque lecteur FatFs,
  l'adresse USB qui le sert, l'état occupé et les secteurs déplacés — **un lecteur resté occupé longtemps après un
  transfert est la signature même de T-31**, le clavier muet pendant les accès disque.
  Le point de conception vaut d'être noté : ces rapports partent de `DSPSync`, donc ils ne lisent **que ce que le
  firmware garde déjà en RAM**. Interroger TinyUSB ou FatFs aurait été plus riche, mais tout TinyUSB vit en flash
  (`hcd_rp2040_irq` à `0x1001b38c`) et appeler du code flash depuis `DSPSync` est exactement la faute qui tuait tous
  les programmes en 0.10.x. D'où trois accesseurs placés en RAM plutôt qu'un appel commode.
  Jeu complet : `!s` famines, `!v` vidéo, `!k` clavier, `!u` usb, `!f` stockage, `!m` mémoire, `!p` placement,
  `!a` tout, `!z` remise à zéro, `!!` un vrai `!`.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 28 584 o libres, 940 o de marge.

- **0.16.1** (2026-09-25, question bmarty : « y a-t-il d'autres commandes intéressantes pour le debug ? ») — **trois
  rapports de plus, un par ticket ouvert (T-74)**. **`!v`** donne le mode, les dimensions, les lignes en retard et
  surtout le **compteur de trames** : core 1 l'incrémente au début de chaque trame, donc s'il monte pendant l'écran
  noir de T-71 le signal vit et l'image est seulement fausse — s'il gèle, l'encodeur s'est arrêté. C'est le
  discriminant que ni l'écran ni la télémétrie seule ne donnaient. **`!k`** dit si le clavier est monté, si Échap a
  été vu et si la file contient quelque chose : la question de T-68, posée hors de l'écran. **`!p`** affiche les
  adresses de `DSPSync`, `DBGFlush` et `DBGInitialise` — `2xxxxxxx` en RAM, `10xxxxxx` en flash — ce qui vérifie que
  le port de debug **n'a pas lui-même enfreint la règle T-46** qu'il sert à surveiller, et donne un repère pour la
  sensibilité au placement de T-48. **`!a`** enchaîne les cinq.
  Coût : ~340 o de code en RAM, d'où **`RAM_LIMIT` porté de 233 500 à 234 500** ; 28,9 Ko de tas restants.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 28 872 o libres.

- **0.16.0** (2026-09-25, demandes bmarty : une voie de debug, puis « un terminal permettant d'exécuter des
  commandes », puis « vérifier l'état des famines ou l'état de la RAM ») — **terminal de debug sur UART0 (T-73,
  T-74)**. Plusieurs défauts résistent parce qu'ils ne s'observent que sur carte et que le seul canal de mesure est
  **l'écran** — précisément ce qui tombe en panne dans T-71.
  **Pas par l'USB-C**, contrairement à l'idée de départ : le RP2040 n'a **qu'un seul contrôleur USB**, déjà en mode
  hôte pour le clavier, le hub et la clé, et le connecteur de programmation *est* ce contrôleur. Hôte et CDC device
  s'excluent, et basculer coûterait les périphériques dont on veut observer l'activité. UART0 (GPIO 28/29, **UEXT
  broches 3 et 4**, 3,3 V) est libre de tout cela. Côté PC : un CP2104 à 115 200 bauds.
  Le terminal marche dans les deux sens. Les caractères reçus entrent dans la **file clavier** — le Neo se pilote
  depuis le PC comme depuis son propre clavier, **écran noir compris** ; le texte de la console revient par l'écho
  `2,20`, qui existait déjà et dont seule la destination change. Une ligne par seconde donne `L=` lignes DVI en
  retard, `S=` secteurs lus, `M=` mode. Commandes préfixées par `!` : **`!s`** famines, **`!m`** mémoire, **`!z`**
  remise à zéro, **`!!`** un vrai `!`.
  **La règle T-46 s'applique ici plus qu'ailleurs** : `DSPSync` est en RAM, donc tout ce que le port y expose est
  `__not_in_flash_func` et n'utilise **pas** `snprintf` (qui vit en flash) — les nombres sortent en hexadécimal par
  une conversion maison. Une trace qui calerait le bus fausserait la mesure qu'elle sert à prendre, et c'est
  exactement la faute que 0.10.3 avait commise.
  Coût : anneau de 2 Ko (un `DIR` produit ~1,5 Ko d'un coup, le port n'en sort que ~3 Ko/s), d'où **`RAM_LIMIT`
  porté de 231 600 à 233 500** ; marge vérifiée, 29,2 Ko de tas restants.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 29 208 o libres.

- **0.15.1** (2026-09-24, retour carte bmarty : « toujours pas ») — **mesure : le firmware dit s'il a vu Échap
  (T-68b)**. La correction T-68 n'a pas suffi sur carte, et deviner a assez duré. La ligne `USB settled` porte
  maintenant deux témoins, relevés **avant** que le menu ne décide : **`KEY`** si le clavier a produit au moins un
  caractère pendant la phase silencieuse — donc s'il était énuméré et si la frappe est arrivée — et **`ESC`** si le
  drapeau que lit `BOOTSelect` est posé. Les trois issues se lisent directement : `ESC` sans menu accuse
  `BOOTSelect` ; `KEY` sans `ESC` accuse le mappage de la touche ; **aucun des deux** dit que le clavier n'était pas
  encore énuméré quand la touche a été pressée — et dans ce cas aucun drapeau ne rattrapera jamais une frappe que le
  matériel n'a pas vue, il faudra afficher l'invite **pendant** les logos.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 32 100 o libres.

- **0.15.0** (2026-09-24, constat carte bmarty : « je n'ai pas vmode sous dos ») — **NeoDOS 0.24.0 embarqué, avec la
  commande `MODE` (T-69)**. Aucune des 35 commandes de NeoDOS ne touchait au mode vidéo : le **mode 1 de Trinity
  n'était joignable que depuis NeoBASIC**, par `VMODE`. Or on démarre sous NeoDOS — le mode Hercules était donc, en
  pratique, hors d'atteinte. `MODE` affiche le mode courant et ses dimensions (`5,10 Get Mode`), `MODE n` en change
  (`5,9 Set Mode`) ; un numéro que le firmware refuse donne `Invalid video mode` et `ERRORLEVEL 1`.
  Côté firmware, seule l'image résidente change : `neodos_binary.h` régénérée depuis `~/Neo6502Msdos/build/neodos.bin`
  (`make -C firmware kernel`). **À retenir** : `EXIT` recharge l'image **embarquée dans le firmware**, pas le binaire
  local — c'est ce qui faisait échouer le test `09_exit` de NeoDOS depuis sa 0.23.0, sa référence étant restée à
  `0.22.0`. Régénérer l'image fait donc partie de toute livraison de NeoDOS.
  `make test-api` 16/16, `make test-toolbox` 10/10, NeoDOS 49/49, Phosphoneo 34/34 ; RAM 32 100 o libres.

- **0.14.1** (2026-09-24, retour carte bmarty : « le Esc au démarrage n'agit plus ») — **régression de la 0.13.0
  corrigée (T-68)**. En donnant aux logos un plancher de 3 s (`USB_FLOOR`, T-66) pendant lequel la console est muette,
  la 0.13.0 a repoussé de 3 à 4 s le message `Boot : auto … (Esc = menu, 3 s)` — **et avec lui la fenêtre où Échap
  était scruté**. Or on appuie pendant les logos, ce que l'ancienne séquence permettait puisque le message venait
  aussitôt : la frappe arrivait désormais **trop tôt** et était ignorée. Le gestionnaire clavier **retient**
  maintenant l'appui (`escapeSeen`, au même endroit que le drapeau `$80` du port de contrôle, remis à zéro par le
  reset clavier de P1), et `BOOTSelect` le consulte **avant** ses deux attentes. Échap compte donc dès que le clavier
  est énuméré, logos compris ; l'appui consommé par le menu est retiré du port de contrôle pour que le 6502 ne le
  voie pas une seconde fois.
  À savoir : ce chemin **n'est pas testable automatiquement** — `BOOTSelect` tourne dans `DSPReset`, avant le premier
  cycle 6502, donc ni `tests/api` ni le `--type-keys` de Phosphoneo (programmé en cycles) ne peuvent l'atteindre ;
  vérifié en essayant sept instants de frappe, aucun n'ouvre le menu, y compris avant la régression. **Validation sur
  carte uniquement.**
  `make test-api` 16/16, `make test-toolbox` 10/10, Phosphoneo 34/34 ; RAM 32 100 o libres.

- **0.14.0** (2026-09-24, demande bmarty : « tu peux faire un dump spécifique au Hercules, pas MDA ? ») — **la police
  du mode 1 est celle du vrai générateur de caractères MDA (T-67)**. Réponse à la question posée : **il n'existe pas
  de police Hercules distincte**. En mode texte, la carte Hercules reprend tel quel le générateur de caractères
  **MDA** d'IBM — sa nouveauté est le mode graphique, pas les glyphes. La police précédente venait de
  `Lat15-VGA14.psf` (console Linux) : la bonne hauteur, mais un dessin **VGA**, plus épais et plus large, pas celui
  d'un MDA. Elle est remplacée par le relevé du générateur MDA publié dans l'**Ultimate Oldschool PC Font Pack v2.2**
  de VileR (int10h.org), fichier `Bm437_IBM_MDA.otb`, sous **CC BY-SA 4.0** (source et licence versionnées dans
  `firmware/scripts/assets/`, attribution portée en tête du header généré).
  Le MDA dessine dans une cellule **9 × 14** dont la neuvième colonne reste vide, sauf pour les semi-graphiques
  `$C0-$DF` où elle réplique la huitième afin que les traits se joignent. Trinity n'exportant que `$20-$7F`,
  **aucun glyphe n'est tronqué** par notre cellule de 8 colonnes, et l'espacement d'origine est conservé puisque notre
  cellule 9 × 14 a elle aussi sa neuvième colonne vide. 87 des 96 glyphes changent ; le coût mémoire est **nul**
  (la table est en flash). Génération : `firmware/scripts/mda14.py` (rendu FreeType à la taille native de la fonte
  bitmap) ; `vga14.py` est conservé pour qui préfère la variante VGA.
  `make test-api` 16/16, `make test-toolbox` 10/10, Phosphoneo 34/34 ; RAM 32 100 o libres.

- **0.13.0** (2026-09-24, idée bmarty : « bloquer le DVI après les logos, attendre 3 secondes, puis afficher tout le
  reste ») — **les logos restent visibles au démarrage (T-66)**. Ils étaient balayés par les messages d'énumération :
  la console écrit dans la même mémoire, et passé la ligne 29 l'écran **défile** — plus il y avait de périphériques,
  plus vite ils disparaissaient. Les phases sont réorganisées : **P0** dessine les logos puis fait taire la console
  (`CONSetQuiet`) ; **P1** découvre l'USB **en silence**, avec un **plancher de 3 s** (`USB_FLOOR`) ; **P2** rétablit
  la console et affiche tout d'un coup — bannière, bilan de la barrière, stockage, fuseau, menu `boot/`.
  L'idée a un mérite que mes propres propositions n'avaient pas : **l'attente ne coûte rien**. La barrière de calme
  de T-32 patientait déjà à cet endroit, et l'énumération se déroule pendant que les logos sont à l'écran.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 32 100 o libres.
  **Validé carte le 2026-09-24** : les logos tiennent leurs trois secondes, et **plus aucun trait rouge au
  démarrage** — ce qui achève T-58.

- **0.12.5** (2026-09-24, **les traits rouges sont réglés**) — **conversion de palette par mots de 32 bits (T-58)**.
  Après trois essais infructueux — cinq tampons TMDS, priorité du DMA (qui a **empiré** les choses), quatre tampons de
  ligne — la mesure avait fini par dire l'essentiel : core 1 ne manque pas de cycles ni d'avance, il manque de **bande
  passante mémoire**. Or le callback de ligne, exécuté **en interruption sur le cœur même de l'encodeur**, faisait pour
  chaque ligne de 320 pixels **320 lectures d'octets, 320 lectures de table et 320 écritures de 16 bits**. En traitant
  quatre pixels par mot de 32 bits, on tombe à **80 lectures et 160 écritures** : moitié moins de transactions sur le
  bus, précisément la ressource qui manquait. L'alignement est garanti à la déclaration des tampons, donc rien à
  vérifier à l'exécution.
  **Carte : plus aucun trait rouge.** Le résidu qui subsistait au démarrage a disparu avec la 0.13.0 (T-66), qui
  sépare l'énumération silencieuse de la phase d'affichage du texte.
  Coût : environ 120 octets de code en RAM, d'où **`RAM_LIMIT` porté de 230 000 à 231 000** (marge vérifiée : 31 Ko de
  tas restants, dont 11,5 Ko de tampons TMDS ; la pile de core 0 vit dans le scratch, hors `.bss`).
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 32 108 o libres.

- **0.12.4** (2026-09-24) — **le clavier de bmarty a un pavé numérique intégré, et il lui fallait le rapport HID (T-65)**.
  « Le Num Lock ne fonctionne pas », « la touche **l** devrait afficher le chiffre **3** » : cette seconde phrase est la
  signature d'un pavé **intégré**, celui des claviers compacts où `j k l` donnent `1 2 3`. Le relevé du clavier sur le
  PC le confirme — un **1A2C:0B2A**, deux interfaces HID. Or ce basculement est fait **par le clavier lui-même**, et il
  ne le fait que si l'hôte lui envoie le **rapport de sortie HID** annonçant Num Lock : exactement celui qui allume les
  diodes. En retirant T-40 en 0.10.12, parce qu'elle détruisait la pile USB, j'avais donc aussi privé ce clavier du
  seul moyen d'apprendre l'état du verrou. Refait comme le backlog le prévoyait : la demande est **enregistrée** par
  `KBDLockLEDUpdate` et **émise par `KBDSync`, entre deux `tuh_task()`** — jamais depuis un callback, ce qui était la
  faute de T-40.
  **T-64** : le pavé donne désormais **toujours** ses chiffres (décision bmarty, revient sur T-41). Num Lock garde son
  état et sa lecture par `2,23`, mais ne change plus ce que produisent les touches.
  **T-58** : passer de 3 à 5 tampons TMDS a été essayé sur carte et n'a **rien changé** — l'encodeur ne manque pas
  d'avance mais de débit. Revenu au défaut, l'essai est consigné.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 32 172 o libres.
  **Validé carte le 2026-09-24** : le pavé intégré bascule (`j k l` donnent `1 2 3`) **et le jeu tourne** — l'émission
  du rapport hors callback ne reproduit donc pas le désastre de T-40, qui tuait tout programme en quelques secondes.
  Sur T-48, cela ne prouve rien à soi seul : c'est un binaire de plus qui tombe du bon côté, et seuls des essais
  répétés diront si la relecture (0.12.0 à 0.12.3) a supprimé la cause.

- **0.12.3** (2026-09-24, fin de la relecture) — dernier défaut trouvé dans les tables API :
  `3,23 Get Current Working Directory` écrivait à une adresse **et** sur une longueur toutes deux données par le
  programme, sans vérifier leur somme ; près du haut de la mémoire, l'écriture la dépassait. Relus **sans défaut** :
  les sept tables de la toolbox, les groupes 4, 6, 7, 8, 9, 11 et 13, ainsi que `QDSetPattern`, les chaînes de
  QuickDraw et les blocs du modem CDC, qui bornent tous correctement.
  **Bilan de la relecture (0.12.0 à 0.12.3) : 19 défauts corrigés** sur environ 15 000 lignes — dont une faute
  matérielle atteignable en une commande (`2,10` sur la ligne 0), un gel complet de la machine (lecture série sans
  octet disponible) et une écriture sans aucune limite (blitter `12,3`). Tous du même profil : ce qu'ils écrasent
  dépend de la disposition des variables, donc du binaire — la signature de T-48.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 32 240 o libres.

- **0.12.2** (2026-09-24, relecture lot 3) — **le blitter complexe pouvait écrire sans aucune limite (T-61)**.
  `12,3` ne vérifiait que la **première** adresse de chaque zone : `width` étant sur 16 bits, une seule ligne pouvait
  copier 64 Ko, et `height` (jusqu'à 255) fois `stride` (jusqu'à 65535) faisait avancer le pointeur de près de
  **16 Mo**. Tous ces champs sont lus dans une structure que le programme 6502 écrit lui-même — c'était donc une
  écriture illimitée dans ce qui suit la zone. `BLTSimpleCopy`, dans le même fichier, vérifie pourtant début **et**
  fin. Désormais chaque ligne est rejetée si elle ne tient pas entièrement dans sa zone. C'est vraisemblablement le
  défaut le plus grave de toute la relecture, et le blitter est très sollicité — NeoLegacy s'en sert pour tous ses
  décors. Également : `mos.cpp` débordait de sept octets son tampon de paramètres sur une commande de 255 caractères.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 32 272 o libres.

- **0.12.1** (2026-09-24, relecture lot 2) — **huit défauts de plus**, du même profil que la 0.12.0 : graphismes,
  console, éditeur, lien série et tables API.
  **Images** : `GFXDrawImage` ne testait pas le `-1` que `GFXFindImage` peut renvoyer et lisait **avant** la mémoire
  graphique ; et cette adresse est calculée à partir de compteurs lus **dans le fichier de graphismes**, donc fournis
  par le programme — au maximum elle atteignait 97 Ko pour une zone de 32 Ko.
  **Tuiles** : avec 255 tuiles, l'adresse de base vaut déjà 32 768 pour 32 768 octets ; et la tilemap, lue dans la RAM
  6502 avec des dimensions tirées d'elle-même, sortait des 64 Ko si le programme la plaçait haut.
  **`CONInsertLine`** était doublement cassée, à la différence de sa symétrique `CONDeleteLine` : la boucle faisait
  varier `y1` mais copiait toujours `y-1` vers `y`, si bien que **rien ne se décalait** ; et `CONCopy` prenant des
  `uint16_t`, insérer à la **ligne 0** donnait `y-1` = 65535, soit une lecture plusieurs mégaoctets plus loin — une
  adresse invalide sur RP2040, donc une **faute matérielle**. Atteignable par `2,10`.
  **Éditeur** : ligne lue et réécrite à une adresse donnée par le 6502, jusqu'à 255 octets au-delà de la mémoire.
  **Lien série** : quatre débordements — `sBuffer[256]` écrit jusqu'à l'indice 256, `fileName[32]` rempli avec une
  longueur pouvant aller à 255, une adresse 16 bits pointant hors d'une zone graphique de 32 Ko, et un transfert sans
  borne de fin. Tout cela alimenté par ce qui arrive sur la ligne série.
  **Groupe 10** : les six transferts par blocs (I2C, SPI, UART) recevaient adresse **et** longueur sur 16 bits sans
  jamais vérifier leur somme.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 32 272 o libres.

- **0.12.0** (2026-09-23, relecture du code demandée par bmarty) — **huit écritures ou lectures hors limites**,
  toutes déclenchables depuis un programme 6502 et toutes du profil de T-48 : ce qu'elles écrasent dépend de la
  disposition des variables, donc du binaire.
  **Chargement de fichier** (`FIOReadFile`) : `cpuMemory[loadAddress+i]` sans borne — `loadAddress` et `loadSize`
  faisant 16 bits chacun, l'écriture allait jusqu'à **64 Ko au-delà** de la mémoire 6502 ; idem vers la RAM graphique.
  **Bloc de commentaire** du même fichier : garde **inversée**, le pointeur ne commençait à avancer qu'après le 32ᵉ
  caractère et ne s'arrêtait plus — un commentaire `.NEO` de plus de 64 caractères écrivait sans limite.
  **`maths.cpp`** : `sprintf` d'un `%f`, une quarantaine de caractères, à une adresse 6502 quelconque.
  **Lecture de ligne console** : jusqu'à `addr+256` sans vérification.
  **`fileimplementation.cpp`** : lecture vers `gfxObjectMemory` non bornée (32 Ko de débordement), et
  `uint16_t(0x10000-address)` qui **vaut 0 à l'adresse 0** — un transfert à l'adresse 0 ne copiait rien, en silence.
  **`sndmanager.cpp`** : `>` au lieu de `>=`, `channel[4]` lu hors tableau par `8,3`.
  **`serial.cpp`** : `SERReadByte` attendait **indéfiniment** ; un programme lisant le port série sans octet
  disponible **figeait toute la machine**, boucle du bus comprise — alors que l'historique du fichier affirme qu'un
  délai avait été ajouté en 2024. Borné à 100 ms (T-34).
  **Émulateurs** : `neo` et Phosphoneo avaient les mêmes lectures de fichier sans aucune borne. Un modèle de
  référence qui se corrompt masque sur PC ce qui casse sur carte — corrigés tous les deux.
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 32 372 o libres.

- **0.11.6** (2026-09-23) — **la priorité DMA est annulée, et `5,40` mesure enfin quelque chose (T-57)**. Sur carte, la
  0.11.5 a rendu les traits **bien pires** : donner au DMA la priorité sur les cœurs prend de la bande passante à
  **core 1, l'encodeur**. L'essai n'est pas perdu — il désigne l'affamé. Surtout, il a fallu constater que
  `late_scanline_ctr` de PicoDVI est un **état**, pas un cumul : `dvi.c` l'incrémente quand une ligne manque et le
  **décrémente dès que le pipeline rattrape**. Le lire depuis `DSPSync`, 95 fois par seconde, le retrouvait presque
  toujours à zéro — d'où un « `DVI` = 0 » qui a blanchi l'affichage toute la journée **à tort**, et fait écarter la
  piste des lignes en retard. Désormais le **callback de ligne** (core 1, toutes les 31 µs) échantillonne et cumule
  dans `lateTotal`, que `5,40` renvoie. La leçon vaut pour la suite : vérifier ce que compte un compteur avant de
  conclure de son zéro. RAM 32 372 o libres.

- **0.11.5** (2026-09-23) — **traits rouges : le DMA passe devant les cœurs (T-56)**. L'outil `VRAMCHK` a tranché : la
  **mémoire vidéo reste saine** pendant que les traits défilent. Le défaut est donc **après la mémoire**, dans le chemin
  vers l'écran — et sans rapport avec la panne des programmes, qui reste ouverte (T-48). Les trois canaux TMDS sont
  servis par **trois canaux DMA séparés**, dont PicoDVI ne demande jamais la priorité, alors que son propre commentaire
  avertit : « we really don't want the FIFOs to bottom out ». Pendant la lecture d'un secteur sur la clé, core 0 copie
  à pleine vitesse et prive ces canaux d'accès mémoire : une lane se vide et peint un trait — le rouge étant le
  troisième. Une ligne dans `DVIStart` suffit :
  `bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_R_BITS | BUSCTRL_BUS_PRIORITY_DMA_W_BITS`, ce que la fiche
  technique recommande pour du DMA temps réel, sans coût mémoire. Cela explique aussi que le mode 1 soit épargné : il
  n'a qu'un encodage par ligne et ses lanes éteintes lisent un canal noir constant, insensible à une famine.
  RAM inchangée (32 408 o libres).

- **0.11.4** (2026-09-23) — **débordement des tableaux MSC (T-54)** : une case au-delà, à chaque accès disque. En
  suivant les traits rouges jusqu'à leur source, on arrive à `usb_storage.cpp`, qui indexait `msc_fatfs_volumes[]` et
  `msc_volume_busy[]` — tous deux de taille `CFG_TUH_DEVICE_MAX`, soit **5** avec le hub — **par l'adresse USB
  elle-même**. Or TinyUSB distribue les adresses **1 à 5** : l'adresse 0 est celle de l'énumération. Une clé qui
  obtient la dernière adresse écrit donc **hors des tableaux** : un octet parasite pour le drapeau d'occupation, et
  **une structure `FATFS` entière** — des centaines d'octets — par-dessus ce que l'éditeur de liens a placé juste
  après. À chaque lecture de secteur. Avec un hub, un clavier, une souris, le modem et la clé, cette adresse est
  atteinte en pratique.
  Ce que le débordement écrase **dépend du binaire**, ce qui explique enfin pourquoi la carte semblait se comporter au
  hasard (**T-48**) ; et quand il tombe dans `graphicsMemory`, on voit des octets parasites — la couleur 1 de la
  palette par défaut étant `255,0,77`, ils s'affichent en **rouge** (**T-38**). Cohérent avec toutes les mesures de la
  journée : `UN`, `OV` et `DVI` obstinément à 0, car il n'y a aucune anomalie de bus ni d'affichage — seulement de la
  mémoire écrasée. Correction : emplacement = adresse − 1, et toutes les indexations bornées (`mscSlot`).
  RAM inchangée (32 408 o libres). `make test-api` 16/16, `make test-toolbox` 10/10.

- **0.11.3** (2026-09-23, reproducteur bmarty : **Tab dans NeoDOS déclenche les traits rouges à coup sûr**) —
  **quatre tampons de ligne au lieu de deux (T-53)**. Tab, c'est la complétion : elle parcourt le répertoire, soit la
  rafale FatFs la plus dense que le clavier sache produire. Avec ce reproducteur et les mesures (`5,40` obstinément à
  0, traits **en mode 0 seulement**), le mécanisme se laisse enfin nommer : en mode 0, l'encodeur lit **trois fois** le
  même tampon de ligne — un passage par canal TMDS, **le rouge en dernier** — pendant que le callback de ligne, qui
  tourne en interruption **sur le même cœur**, n'avait que **deux** tampons à alterner. Quand core 0 sature la mémoire
  pour FatFs, l'encodeur décroche, le callback revient sur le tampon qu'il est en train de lire, et seule la troisième
  passe voit les données neuves : bleu et vert justes, rouge faux. `late_scanline_ctr` reste à 0, car le tampon TMDS
  est bien publié à l'heure — c'est sa **source** qui a changé sous lui. Le mode 1 ne fait qu'un encodage par ligne,
  d'où l'absence de traits. Désormais **4 tampons**, dimensionnés pour le plus large mode **couleur** (320 pixels,
  mode 0) au lieu du plus large mode tout court (720, monochrome, qui passe par `monoLine`) : quatre tampons coûtent
  ainsi **moins** que les deux anciens. RAM 32 408 o libres (−208 o de tampons, marge sous `RAM_LIMIT` portée de 80 à
  264 o). `make test-api` 16/16, `make test-toolbox` 10/10.

- **0.11.2** (2026-09-23) — **`RXUNDER` et `TXOVER` comptés (T-52)**, et avec eux une hypothèse forte pour T-48. La
  mesure carte a montré que, l'écran perdu, **Ctrl+Alt+AltGr redémarre encore la carte** : le firmware tourne, ce n'est
  donc pas lui qui se bloque. En relisant `processor_pio.cpp` : la boucle du bus appelle `pio_sm_get(pio1,0)` pour
  l'adresse et pour la donnée d'une écriture, or cette fonction du SDK est `return pio->rxf[sm];` — elle **ne vérifie
  pas que la file contient quelque chose**. Un firmware qui arrive avant la machine PIO lit un mot périmé, puis écrit
  `cpuMemory[adresse fantôme] = donnée fantôme` : **le programme 6502 est corrompu pendant que le firmware continue**,
  exactement le tableau observé. Et comme tout dépend de la vitesse relative des deux, un binaire dont le code chaud
  tombe mieux en cache lit plus souvent dans le vide — **le mécanisme par lequel la panne devient sensible au
  placement**. `5,41` prend désormais un index : 0 TXSTALL, 1 RXSTALL (tous deux normaux pendant une commande API),
  **2 RXUNDER** et **3 TXOVER** (fautifs). `BUS.NEO` affiche `UN` et `OV` en tête de ligne.
  ⚠️ **Mesuré le jour même : `UN` reste à 0**, au repos comme à la frappe. L'hypothèse est donc **infirmée** — la boucle
  ne lit jamais de file vide et le commentaire du code dit vrai. Les compteurs restent, ils écartent définitivement
  cette famille de causes. `make test-api` 16/16.

- **0.11.1** (2026-09-23, premières mesures carte avec `BUS`) — **durée passée loin du bus (T-50)** et **outils
  utilisables (T-51)**. La mesure au repos a montré que TX **et** RX montent, ce qui était prévisible : servir une
  commande API cale forcément le PIO, puisque le 6502 boucle sur le port de contrôle pendant que le firmware
  travaille. Compter les calages ne suffit donc pas — c'est leur **durée** qui sépare le sain du pathologique.
  **`5,42 Get Bus Timing`** rend la plus longue `DSPSync` (P0 = 0 : clavier, tâche USB, clignotement) et la plus
  longue commande API (P0 = 1), en microsecondes, chronométrées dans la boucle du bus par le timer matériel ; `BUS.NEO`
  les affiche (`SY` et `CM`). **Enseignement immédiat : `DVI` reste à 0** — l'encodeur n'est jamais en retard, core 1
  va bien. Cela écarte T-38 pour cette panne et confirme que les glitches de la barre de menus ne sont pas des lignes
  en retard. **T-51** : `LATE` et `BUS` sortaient sur n'importe quelle touche, alors qu'il faut pouvoir taper pendant
  la mesure — seul Échap sort désormais. `make test-api` 16/16 ; RAM 32 224 o libres (marge de 80 o sous `RAM_LIMIT`,
  le prix de l'instrumentation).

- **0.11.0** (2026-09-23, demande bmarty : instrumenter la carte sans sonde, tout doit passer par l'écran) —
  **compteurs de décrochage du bus 6502 (T-49)**, prérequis de T-48. La machine PIO génère l'horloge PHI2 du 6502
  (side-set GPIO 21) : quand le firmware est en retard, elle cale et l'horloge s'arrête — le processeur est **étiré**,
  jamais nourri d'un octet faux. C'est la bonne nouvelle du programme PIO, et ça oriente T-48 : la panne ne vient pas
  d'un octet corrompu sur le bus. Ces calages, le matériel les enregistre gratuitement dans `PIO_FDEBUG`, dont les bits
  sont collants jusqu'à réécriture : `TXSTALL` quand la donnée d'une lecture n'était pas prête, `RXSTALL` quand la file
  d'adresses n'a pas été vidée à temps. `HWBusProbe()` — **en RAM**, la leçon de T-46 — les échantillonne depuis
  `DSPSync` (~95 Hz) et compte les fenêtres contenant au moins un calage ; la boucle du bus, elle, ne paie rien.
  **`5,41 Get Bus Stalls`** rend les deux compteurs au 6502 (P0-3 TX, P4-7 RX, `$FF` en P0 pour remettre à zéro) et
  l'outil **`BUS.NEO`** les affiche à l'écran avec `5,40` (lignes DVI en retard), pour voir laquelle des deux famines
  se produit. Coût : 8 octets de RAM. Test `tests/api/busstall.asm` (0 sous `neo`, qui n'a pas de PIO).
  `make test-api` 16/16, `make test-toolbox` 10/10 ; RAM 32 376 o libres.

- **0.10.12** (2026-09-23) — ⚠️ **la bissection de cette nuit n'a PAS trouvé la cause : elle a mesuré un effet de
  placement du code, et c'est désormais prouvé.** `trinity-0.10.12-USB.uf2` et `trinity-0.10.12b-banniere-USB.uf2` sont
  compilées du **même code source** et ne diffèrent que par la chaîne de version affichée au démarrage : la première
  échoue systématiquement, la seconde fait tourner `legdiag` et le jeu (carte, 2026-09-23). Voir T-48. `bisect-I` et 0.10.12 sont fonctionnellement identiques (même taille d'UF2, même RAM, seules
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
