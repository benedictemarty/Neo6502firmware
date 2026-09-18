# Chaîne de build du fork (F-00)

État vérifié le 2026-09-16 (Linux, `arm-none-eabi-gcc` 14.2.1, `cmake`, `64tass` 1.60).

## Dépendances RP2040
| Composant | Version | Où |
|---|---|---|
| Pico SDK | **1.5.1** (`~/pico-sdk-1.5.1`, clone `--depth 1 --branch 1.5.1`) | `PICO_SDK_PATH` |
| TinyUSB | 0.16.0 | récupéré par FetchContent (`firmware/CMakeLists.txt`) |
| PicoDVI | fork paulscottrobson + `patches/picodvi-vertical-repeat.diff` | FetchContent |
| no-OS-FatFS-SD-SPI | `bbccc5d` (SDCARD seulement) | FetchContent |

`~/pico-sdk-internal` (v2.2.0) **ne convient pas** : le PicoDVI amont y échoue
(`dma_debug_channel_hw_t` n'a plus de membre `tcr`) et `boot_stage2` a changé de
dossier. On reste sur le SDK 1.5.x visé par le firmware amont ; le CMake accepte
toutefois les deux emplacements de `boot2_generic_03h.S` (SDK 1.x / 2.x).

## Commandes
```
export PICO_SDK_FETCH_FROM_GIT=OFF PICO_SDK_PATH=$HOME/pico-sdk-1.5.1
make -C firmware build STORAGE=USB        # firmware/firmware.uf2   (398 848 octets, 194 Ko flash)
make -C firmware build STORAGE=SDCARD     # variante carte SD      (428 544 octets)
make -C firmware build-multiboot          # firmware/firmware_slot0.uf2 (slot 0, F-80)
make -C multiboot image check             # neo6502-multi.uf2 (4 slots, 1 028 Ko) + sonde libemul
make -C emulator emulator                 # bin/neo
make -C ~/Phosphoneo test                 # suite de tests (golden, co-sim, différentiel neo)
```
`multiboot/Makefile` prend `PICO_SDK_PATH` s'il est défini, sinon le SDK téléchargé
dans `firmware/build/_deps/pico_sdk-src`.

Le `Makefile` de l'émulateur ne suit pas les en-têtes : après une modification d'un `.h`
de `firmware/common/include` (ex. `GFX_MODE_COUNT`), supprimer les objets
(`rm firmware/common/sources/interface/*.o emulator/src/core/*.o`) avant `make -C emulator emulator`,
sinon `bin/neo` garde l'ancienne valeur (constaté le 2026-09-17, F-57).

Le dossier `firmware/build` doit être **supprimé** quand on change de SDK
(`cmake --fresh` ne suffit pas : les sous-projets pioasm/picotool gardent leur cache).

## Résultat du 2026-09-16
USB, SDCARD et slot 0 compilés sans erreur (3 avertissements amont « unused variable »,
non bloquants grâce à `-Wno-error=unused-variable`) ; image multi-boot assemblée ;
Phosphoneo `make test` : 0 échec. Rien n'a encore été flashé sur une carte.

## R22 : correctifs appliqués aux dépendances à la configuration

- `firmware/patches/apply-picodvi.sh` : `vertical_repeat` dans PicoDVI (FetchContent `PATCH_COMMAND`).
- `firmware/patches/apply-fatfs-tiny.sh` : `FF_FS_TINY 1` dans `ffconf.h` de la FatFs SD (`no-OS-FatFS-SD-SPI`),
  lancé par `execute_process` à chaque `cmake` (idempotent) — vaut aussi pour un `PICO_FATFS_PATH` fourni par
  l'environnement. La FatFs de la clé USB est dans `firmware/lib/fatfs` (F-105, déjà en `FF_FS_TINY 1`).
- TinyUSB **0.21.0** (F-105) : FetchContent, ou `PICO_TINYUSB_PATH=~/neo-deps/tinyusb-0.21` (clone local du tag) ;
  `tinyusb_board` n'est plus lié (son BSP exige le SDK 2.x).
- `firmware/sources/memmap_neo.ld` : script d'édition de liens (`pico_set_linker_script`), copie de
  `memmap_default.ld` du SDK 1.5.1 avec le dérouleur d'exceptions de libgcc laissé en flash ; à resynchroniser
  si le SDK change.
