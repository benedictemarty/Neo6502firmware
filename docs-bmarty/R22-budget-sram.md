# R22 — Budget SRAM du RP2040 (revue du 2026-09-17)

Le RP2040 a 264 Ko de SRAM : 256 Ko de banques principales (`RAM`, où l'éditeur
de liens place `.data`/`.bss` puis le **tas**) et 2 × 4 Ko de scratch (pile de
core0 en `SCRATCH_Y`, celle de core1 est le tableau `core1_stack` de 2 Ko en
`.bss`). Le tas = `__StackLimit` (fin de `RAM`, `$20040000`) − `__end__`.
`make -C firmware build` affiche désormais cette valeur et **refuse** un
firmware sous `HEAP_MIN` = 2 048 octets.

## Mesures (firmware USB, `arm-none-eabi-nm --size-sort`)

| Poste | Octets | Origine | Réductible ? |
|---|---:|---|---|
| `graphicsMemory` | 81 920 | 2 pages de 40 960 (modes 1/2, F-55) ; le mode 0 en prend 76 800 | −5 120 si une seule page en modes 1/2 |
| `cpuMemory` | 65 536 | RAM 6502 | non |
| `gfxObjectMemory` | 32 768 | sprites/tuiles (amont) | non (API) |
| `bankStorage` | 16 384 | F-23, 2 banques | −8 192 avec 1 banque |
| `consoleMemory` | 7 040 | console (80×43 pour F-57) | — |
| `fileHandles` | 4 416 | 8 `FIL` FatFs de 552 (tampon de secteur par fichier, `FF_FS_TINY 0`) | **−4 096 avec `FF_FS_TINY 1`** (tampon partagé par volume ; lecture/écriture un peu plus lentes en accès non alignés) |
| `sprites` | 3 072 | amont | non |
| `msc_fatfs_volumes` | 2 840 | 4 `FATFS` (F-102 volumes 0-3) | −1 420 avec 2 volumes |
| `rxBuffer` (UART) | 2 048 | série UEXT | −1 024 (risque de dépassement à 230 400 bauds) |
| `core1_stack` | 2 048 | rendu DVI | non |
| `buffer1/2`, `monoEncoded`, `blackChannel` | 5 952 | rendu DVI (dont F-52 1 bpp) | — |
| `cdch_data` | 1 480 | TinyUSB CDC hôte (F-90, 2 ports) | −740 avec 1 port |
| `channel` (son) | 1 168 | amont | non |
| `gfxLineScratch` | 720 | **partagé** tilemap + doublage blitter (cette revue : −720) | fait |
| files/fenêtres (F-42/F-43) | ~640 | 32 événements × 8 + 8 fenêtres × 44 | — |

Tas restant : **2 692 o avant la revue → 3 408 o après** (USB) ; SDCARD ≈ +1,2 Ko.

Mise à jour du 2026-09-18 : le Control Manager (F-44, 432 o) ne tenait plus (2 220 o de
tas sur la base avec les modes 3 et 4 de l'autre session). **Décision bmarty : modes vidéo
3 (F-57) et 4 (ADR-04 a) retirés** (revert) et console dimensionnée aux modes conservés
(`MAXCONSOLEHEIGHT` 43 → 32, −1 760 o) : tas USB **3 804 o**, SDCARD 5 052 o. Les
options 1-4 ci-dessous restent ouvertes pour la suite (F-45 dialogues, F-46 ressources).

Mise à jour du 2026-09-18 (soir) : **`DSPHandler` est copié en SRAM** (`__time_critical_func`, 6,8 Ko) et
chaque groupe toolbox ajouté à son `switch` coûtait ≈ 300-370 o (F-46 : −368 o). Les groupes ≥ 32 sont
désormais servis par `DSPToolbox` en flash (`dispatch_toolbox.h`, `noinline`) : tas USB **5 808 o**,
SDCARD 7 056 o (F-45 dialogues : −524 o ; F-17 : 0).

## Qui utilise le tas ?
`std::string` du groupe 3 (chemins > 15 caractères : allocations transitoires
≤ 256 o), TinyUSB (statique, hors tas), FatFs (statique), pas de `iostream`.
Le besoin réel est faible mais **non mesuré sur carte** ; F-83 (installation
d'un `.uf2`) aura besoin d'un tampon de page flash de 4 Ko : à prendre sur un
tableau statique, pas sur le tas.

## Décisions à prendre (PO)
1. **`FF_FS_TINY 1`** (−4 Ko, le plus gros gain) : `ffconf.h` est dans les
   dépendances (TinyUSB pour USB, no-OS-FatFS pour SDCARD) ; il faudrait le
   patcher au `cmake` (copie d'un `ffconf.h` du fork par-dessus après
   `FetchContent`). Change le comportement d'E/S : à valider sur carte.
2. **Banques 2 → 1** (−8 Ko) si le besoin civ tient en 8 Ko.
3. **Volumes 4 → 2** (−1,4 Ko) : deux clés USB suffisent-elles ?
4. Une seule page en modes 1/2 (−5 Ko) : renonce au double tampon F-55.

Tant que rien n'est décidé, le seuil `HEAP_MIN` protège les prochaines
évolutions (F-44… : ~1 Ko de structures chacune au plus).

## Plan de la SRAM (firmware USB `v1.0.0-95-g8db5e86`, `arm-none-eabi-readelf -S` / `nm -S`)

264 Ko = banques principales `$20000000-$2003FFFF` (256 Ko) + scratch X `$20040000` (4 Ko) + scratch Y
`$20041000` (4 Ko, pile de core0).

| Adresse | Taille | Zone | Contenu |
|---|---:|---|---|
| `$20000000` | 192 o | vecteurs | table d'interruptions en RAM (SDK) |
| `$200000C0` | 20 284 o | `.data` | **code copié en SRAM** (`__time_critical_func` + libgcc) et variables initialisées : `DSPHandler` 4 932 (dispatch des groupes 1-13 ; les groupes toolbox sont en flash depuis F-46), `dvi_dma_irq_handler` 1 376, `_scanline_callback` 988, `core1_main` 776, `CPUExecute` 248 (boucle bus), `__malloc_av_` 1 032, **`__gnu_unwind_*` + `_Unwind_VRS_Pop` ≈ 2 170** (déroulement d'exceptions C++, jamais utilisé), `__global_locale` + `_impure_data` 684 (newlib), `flash_do_cmd` 308 |
| `$20005000` | 235 856 o | `.bss` | voir le détail ci-dessous |
| `$2003E950` | **5 808 o** | tas | `malloc` (std::string du groupe 3, TinyUSB en régime transitoire) |
| `$20040000` | 588 o | scratch X | `boot2`, `core1_stack` **2 048** (pile de core1, rendu DVI, placée ici et non en `.bss`) |
| `$20041000` | 4 096 o | scratch Y | pile de core0 |

### Détail de `.bss` (≥ 256 o, ordre des adresses)

| Adresse | Octets | Symbole | Rôle | Levier |
|---|---:|---|---|---|
| `$200050D0` | 1 440 | `blackChannel` | DVI : ligne noire (F-52 1 bpp) | — |
| `$200056A8` | 3 072 | `sprites` | amont | non (API) |
| `$200062A8` | 352 | `windows` | 8 fenêtres (F-43) | — |
| `$20006420` | 480 | `controls` | 24 contrôles (F-44) | — |
| `$20006630` | 576 | `_hidh_itf` | TinyUSB HID hôte | — |
| `$200068F8` | 256 | `default_alarm_pool_entries` | SDK | — |
| `$20006B30` | 712 | `__sf`, `__atexit0` | newlib stdio | — |
| `$20006E48` | 512 | `LfnBuf` | FatFs noms longs | −512 si `FF_USE_LFN` en pile (option 2 de FatFs) |
| `$2000705C` | **16 384** | `bankStorage` | 2 banques de 8 Ko (F-23) | −8 192 avec 1 banque |
| `$2000B060` | **4 416** | `fileHandles` | 8 `FIL` de 552 (tampon de secteur par fichier) | **−4 096 avec `FF_FS_TINY 1`** |
| `$2000C1A0` | 1 504 | `monoEncoded` | DVI 1 bpp (F-52) | — |
| `$2000C8C8` | 490 | `functionKeyText` | 2,4 Define Hotkey | — |
| `$2000CB1C` | **2 840** | `msc_fatfs_volumes` | 4 `FATFS` (F-102) | −1 420 avec 2 volumes |
| `$2000D7E4` | 1 168 | `channel` | son | non |
| `$2000DE7C` | **2 048** | `rxBuffer` | UART UEXT | −1 024 (risque à 230 400 bauds) |
| `$2000EA14` | 1 548 | `_msch_itf`, `_usbh_*` | TinyUSB MSC / hôte | — |
| `$2000F108` | 3 008 | `buffer1`, `buffer2` | DVI : 2 lignes encodées | — |
| `$2000FCC8` | 1 480 | `cdch_data` | TinyUSB CDC hôte, 2 ports (F-90) | −740 avec 1 port |
| `$20010294` | 5 280 | `consoleMemory` | 80 × 32 cellules × 2 (R22 : 43 → 32 lignes) | — |
| `$20011734` | **65 536** | `cpuMemory` | RAM 6502 | non |
| `$20021734` | 768 | `currentPalette` | 256 couleurs × 3 | — |
| `$20021A84` | 696 | `dvi0` | PicoDVI | — |
| `$20021E40` | 512 | `ep_pool` | TinyUSB | — |
| `$20022090` | 720 | `gfxLineScratch` | partagé tilemap / blitter (R22) | fait |
| `$20022360` | **32 768** | `gfxObjectMemory` | sprites / tuiles (amont, page `$90`) | non (API) |
| `$2002A364` | **81 920** | `graphicsMemory` | 2 pages de 40 960 (modes 1/2, F-55) ; mode 0 = 76 800 | −5 120 avec une seule page en modes 1/2 |
| `$2003E380` | 512 | `palette` | — | — |
| `$2003E6B8` | 512 | `userDefinedFont` | UDG `$C0-$FF` (Latin-1 par défaut, F-17) | — |
| (divers) | ≈ 3 000 | < 256 o chacun | dialogues 156, files d'événements 256, menus, etc. | — |

Total `.bss` : 235 856 o, dont **200 000 o (85 %)** pour les quatre tableaux `cpuMemory`, `graphicsMemory`,
`gfxObjectMemory`, `bankStorage`.

### Leviers restants, par ordre de rendement

| # | Mesure | Gain | Coût / risque |
|---|---|---:|---|
| 1 | `FF_FS_TINY 1` (tampon de secteur partagé par volume) | 4 096 o | lectures non alignées un peu plus lentes ; à mesurer sur carte (3,8 et 3,27) |
| 2 | Compiler sans déroulement d'exceptions (`-fno-exceptions -fno-unwind-tables`, vérifier que libgcc/newlib ne tirent plus `__gnu_unwind_*` en `.data`) | ≈ 2 100 o | à vérifier : d'où vient le placement en `.data` (script d'édition de liens du SDK ?) |
| 3 | Une seule page graphique en modes 1/2 (F-55) | 5 120 o | perd le double tampon des modes 1/2 |
| 4 | 1 banque au lieu de 2 (F-23) | 8 192 o | API 1,18 réduite |
| 5 | 2 volumes au lieu de 4 (F-102) | 1 420 o | — |
| 6 | 1 port CDC hôte (F-90) | 740 o | modem + 2ᵉ périphérique série impossibles |
| 7 | `rxBuffer` 1 Ko | 1 024 o | dépassement possible à 230 400 bauds |

Priorité proposée : 1 puis 2 (sans effet fonctionnel), le reste seulement sur besoin.
