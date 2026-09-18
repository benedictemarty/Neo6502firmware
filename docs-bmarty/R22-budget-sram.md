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
