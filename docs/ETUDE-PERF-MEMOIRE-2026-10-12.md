# Étude performances et mémoire du firmware — Trinity 0.16.86 (T-132, 2026-10-12)

Demande de bmarty : « l'étude de la totalité du firmware pour voir où gagner des perfs et de la mémoire ».
Étude en lecture et sur PC. Aucun code de `trinity` n'a été modifié.

Sources :
- l'ELF livré, `~/neo-carte/trinity-0.16.86-noms-utf8-USB.elf` (nm, objdump, size) et sa carte de liens ;
- la lecture du code et de l'historique (`docs/BACKLOG.md`, `README-TRINITY.md`) ;
- trois revues : mémoire, cœur 0, cœur 1 ;
- une compilation de mesure, faite dans le worktree d'essai puis annulée.

Les cycles sont **comptés** sur le désassemblage avec le minutage Cortex-M0+ (sans attente mémoire), sauf mention « mesuré ». Rien de ce document n'a été essayé sur carte.

## 1. État des lieux

| Ressource | Occupation | Remarque |
|---|---|---|
| RAM statique (.data+.bss) | 233 140 o, `RAM_LIMIT` 237 060 | marge **3 920 o** |
| dont code en RAM | 21 658 o, 367 fonctions | dont **239 veneers** (relais vers la flash) = **3 824 o** |
| Tas | ~29 000 o | permanent : tampons TMDS ≈ 11,7 Ko (modes 0/2), 13,1 Ko (mode 1) ; pic mesuré 15 380 o (T-79, 0.16.42, ancien) |
| scratch_x (4 Ko) | encodeurs 0x24c + pile du cœur 1 (0x20040800-0x20041000) | 1 460 o libres **si** la pile du cœur 1 ne descend pas sous 0x20040800 (sans garde, profondeur inconnue) |
| scratch_y (4 Ko) | pile du cœur 0, depuis 0x20042000 | 0x20041000-0x20041800 **n'est pas libre** : c'est la marge de la pile du cœur 0, sans garde, profondeur inconnue (correction de ce que j'avais écrit pour T-131) |
| Flash | binaire ≈ 237 Ko ; banques à 0x1C0000 | ≈ 1,5 Mo libres : la flash n'est pas une contrainte |

Charge du cœur 1 par ligne logique, comptée, sans l'IRQ DMA :
- mode 0 : ≈ 9 150 cycles sur 16 000 (57 %) ;
- mode 2 : ≈ 9 000 sur 16 000 (56 %) ;
- mode 1 : ≈ 2 300 sur 8 580 (27 %).

Cohérent avec T-106 (encodage max mesuré 30 µs). D'après T-58, le facteur limitant sur carte est la bande passante de la SRAM, pas les cycles.

## 2. Mémoire : pistes

| # | Piste | Gain | Risque | Preuve |
|---|---|---|---|---|
| M1 | Compiler `dispatch.cpp` en `-mlong-calls` : `DSPHandler` (en RAM) n'appelle plus la flash par 206 veneers | **−2 352 o, MESURÉ** (compilation : 239 → 32 veneers en RAM, RAM 234 028 → 231 676 sur l'essai) | faible : même code ; un appel API coûte ~1 cycle de plus par appel en flash ; le placement change (protocole T-48) | objdump |
| M2 | Groupes rares de `DSPHandler` (3, 9, 10, 13, 14, horloge du 1) en flash, comme `DSPToolbox` | estimé 2 à 4 Ko, recouvre M1 | moyen : vitesse de l'API (`cmdMaxUs`, T-50) à mesurer ; T-104 disait « pas sans mesure » | `dispatch.cpp:35` |
| M3 | `currentPalette` (768 o RGB) déduit de `palette` (RGB565) + 1 octet de bits bas | −512 o | faible ; touche `common/` (neo, Phosphoneo) | `graphics.cpp:18-21` |
| M4 | `TMPDrawTileMap::lineBuffer` sur le tas, par appel | −720 o statiques, +720 o transitoires | faible | `tilemap.cpp:114` |
| M5 | `LfnBuf` : `FF_USE_LFN` 1 → 3 (tas) | −512 o statiques, +512 o transitoires | faible à moyen | `ffconf.h:116` |
| M6 | `atexit()` neutre (le firmware ne sort jamais) | −400 o, à confirmer par compilation | faible | newlib `register_fini` → `__atexit0` |
| M7 | `DBGCommand`/`DBGReport*` en flash, `DBGPoll` reste en RAM | ≈ −1,2 Ko | moyen : contraire à la lettre de T-46 (précédent : `DBGPoll` → `FISDebugUploadPoll`) | `debugport.cpp:302-341` |
| M8 | Remplacer `strtod` (`maths.cpp:189`) pour retirer `__global_locale` | −364 o, à confirmer | moyen (arrondis) | carte de liens |

**Déjà refusé ou démenti, à ne pas reproposer :**
- T-104 et T-124 (a-c), refusés le 2026-10-08 : réduire le tampon CDC, le nombre de volumes FatFs et `rxBuffer` ;
- T-71 : 4 tampons 1 bpp ;
- T-58 : 5 tampons TMDS ;
- `FF_FS_TINY` : fait (T-104).

**À garder tel quel :**
- les données newlib (`_impure_data`, `__sf`, `__malloc_av_`) : c'est l'état de malloc, pas des restes de printf ;
- `noisePattern`, lu par l'IRQ du son ;
- `consoleMemory` (contrat de l'API, T-13) et `sprites` (`static_assert`, T-113) ;
- les mémoires du cœur 1.

## 3. Performances du cœur 0 : pistes

| # | Piste | Gain | Risque | Preuve |
|---|---|---|---|---|
| P1 | **Charger les `.neo` par blocs** : `FIOReadBlock` lit chaque octet par un `f_read` (`fileinterface.cpp:127,147`) | mesuré en 0.16.41 : ≈ 34 µs/octet. Un `f_read` par bloc vise ≈ 900 Ko/s : un programme de 30 Ko passerait d'≈ 1 s à ≈ 40 ms (estimation à remesurer) | faible si l'on garde les bornes T-59 | code vérifié |
| P2 | Modes 1 et 2 : remplissages, lignes horizontales et verticales par octets ou mots au lieu de `GFXPlotPixel` par pixel (≈ 75 cycles/pixel) | rectangle plein plein écran : mode 2 ≈ 23 ms, mode 1 ≈ 70 ms → < 0,1 ms (×50 à ×100) | faible (bords de quartet ou de bit) | `gfxcommands.cpp`, `graphics.cpp:89-112` |
| P3 | Glyphes de la console en mode condensé posés par octets de ligne | ≈ 35 µs → ≈ 1 µs par caractère (mode 1) | faible à moyen (attributs MDA) | `console.cpp:146-162` |
| P4 | Défilement de la console par `memmove` au lieu de repeindre toutes les cellules | mode 1 ≈ 70 ms → ≈ 0,3 ms par ligne défilée | **moyen** : comportement visible (les graphismes posés sous le texte défilent avec lui) | `console.cpp:344-357` |
| P5 | Sprites XOR et tilemaps en mode 2 : travail direct sur les quartets | ×5 à ×8 | faible | `sprites_xor.cpp:118`, `tilemap*.cpp` |
| P6 | Lignes et ellipses en entiers (Bresenham) au lieu de `double` | ×5 à ×10 estimé | moyen : pixels différents possibles, comparaison exhaustive sur PC | `efla.cpp:43-60`, `ellipse.cpp` |
| P7 | 3,20 Copy File : tampon de 1 → 4 Ko | inconnu (40 Ko/s mesuré, l'écriture USB domine) | faible | `fileimplementation.cpp:21` |
| P8 | IRQ du son à 30 882 Hz même en silence | ≈ 1 % du cœur 0 | faible à moyen | `sound.cpp:54` |

**Rien à gagner :**
- la boucle de bus et la répartition (tables de saut, `DSPHandler` en RAM) ;
- `DSPSync`, sauf le clignotement en mode 1 (T-46) ;
- la lecture USB (893 Ko/s, 73 % du maximum en bulk full speed) ;
- `DSPGetStdString` (µs contre ms d'E/S).

**À ne pas faire :**
- LTO : `--wrap`, sections RAM, identité de `CPUExecute` ;
- priorité de bus : démentie par T-56 et T-71 ;
- QSPI ou boot2 : écarté.

L'option effective est `-O2` (`firmware/CMakeLists.txt:41`). Passer certains fichiers graphiques en `-O3` donnerait un gain inconnu.

## 4. Performances du cœur 1 : pistes

| # | Piste | Gain | Risque |
|---|---|---|---|
| D1 | `palette` et les boucles 8/4 bpp du rappel de ligne en scratch_x | −40 % du trafic SRAM du rappel (−320 lectures de palette, ≈ −1 000 lectures d'instructions par ligne) ; 0 cycle sans attente ; libère ≈ 512 o sous `RAM_LIMIT` | faible ; prend de la place en scratch_x ; placement à comparer à égalité (T-48) |
| D2 | Mode 2 : encodeur 4 bpp direct, avec une palette TMDS de 192 o | ≈ 9 000 → ≈ 6 000 cycles ; accès SRAM −60 % ; tampons de ligne de 160 o au lieu de 704 | moyen : nouvel assembleur (banc libemul sans interpolateur) |
| D3 | Mode 2, version légère : table de paires de 1 Ko | ≈ −1 400 cycles par ligne logique | faible ; 1 Ko de RAM |
| D4 | Mode 0 : encodeur 8 bpp direct (table de 3 Ko) | gain net modeste | moyen à élevé (3 Ko de tas) |
| D5 | Alléger l'IRQ DMA de PicoDVI de l'instrumentation de T-77 (`_dvi_ctrl_audit` ×3 par ligne) | quelques centaines de cycles par ligne, en IRQ | faible à moyen ; on perd `RNDCtrlOverrun*` |
| D6 | SRAM non entrelacée, `cpuMemory` seule dans le banc 3 : la boucle de bus (≈ 6,8 M accès/s) ne touche plus les bancs du cœur 1 et du DMA (demande bmarty, 2026-10-12) | inconnu : hypothèse T-58 (bande passante SRAM) | élevé : toute la carte mémoire change (T-48) ; **essai prêt** : branche `essai/bancs-sram` `18d320db`, script de liens seul, `test-coeur1` OK ; UF2 et procédure (RENDU.NEO contre la 0.16.86) dans `~/neo-carte/essais/bancs-sram-t132/` ; limite : le code en RAM des deux cœurs partage encore le banc 0 |

**À ne pas reproposer :**
- T-56 : priorité au DMA ;
- T-71 : priorité de bus au cœur 1, 4 tampons 1 bpp ;
- T-58 : 5 tampons TMDS ;
- T-77 : autres timings du mode 1.

## 5. Défauts possibles relevés en passant (non vérifiés)

- `GFXDrawImage` à l'échelle 1 pose ses pixels par `GFXPlotPixel` **sans contrôle de coordonnées** : écriture hors de l'écran possible.
- `GFXFastLine` rejette toute ligne dont une extrémité sort de l'écran : les segments d'une ellipse pleine qui dépasse disparaissent.
- La profondeur des piles des deux cœurs est **inconnue** et aucune n'a de garde. Une pile du cœur 1 qui déborde écraserait le code de l'encodeur TMDS. Mesure possible sur carte : remplir les piles d'un motif au démarrage, puis les relire par la sonde après `RENDU`, `DIR` et `COPY`.

## 6. Ordre proposé (décision bmarty)

1. **P1** (chargement des `.neo`) : gain visible pour tous les programmes, risque faible ; vérifiable dans `neo`, chronométrable sur carte.
2. **M1** (`-mlong-calls`) : −2,35 Ko de RAM mesurés, une ligne de CMake ; à essayer sur carte (placement, T-48).
3. **P2 + P3** : dessin et texte en modes 1 et 2 ; vérifiables au pixel près dans Phosphoneo, cycles au banc libemul.
4. **D1**, puis **D2** si le mode 2 doit gagner de la marge contre les traits.
5. La mesure des piles (§ 5), avant de toucher aux scratch.

Le reste à la demande. Chaque piste ferait l'objet d'un ticket à part, avec son test et un contre-essai.
