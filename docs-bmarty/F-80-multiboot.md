# F-80 / F-81 — Multi-boot RP2040 : plusieurs firmwares en flash, choisis depuis le Télémon

Date : 2026-09-15. État : **écrit, compilé, et vérifié sur libemul** (le sélecteur saute
dans le firmware Neo relié pour le slot 0, qui démarre jusqu'à `CPUExecute`) ; **non
exécuté sur carte**.

## Faits (Pico SDK 1.5, lus dans les sources)
- Le bootrom charge les 256 premiers octets de la flash (`boot2`, configure le SSI/XIP)
  puis `boot2` vectorise **toujours** en `XIP_BASE + 0x100` (`exit_from_boot2.S`) :
  VTOR ← table, MSP ← [0], saut à [1]. Le point d'entrée d'une image est donc « sa base
  + 0x100 », quelle que soit la base, pourvu qu'elle ait été **liée pour cette base**.
- `watchdog_reboot(0,0,ms)` fait un démarrage standard (bootrom → boot2 → image à 0) ;
  les registres `scratch[0..3]` du watchdog survivent à ce reset (pas à une mise hors
  tension). Le SDK n'utilise que `scratch[4..7]`.
- `boot2` est une fonction : si LR ≠ 0 elle **retourne** au lieu de vectoriser
  (`check_return`) — on peut donc exécuter le `boot2` d'une autre image depuis la RAM
  pour appliquer *sa* configuration de flash (`PICO_FLASH_SPI_CLKDIV`).

## Disposition de la flash (2 Mo) — `multiboot/neoboot.h`
| Offset | Taille | Contenu |
|---|---|---|
| `0x00000` | 64 Ko | **sélecteur** `neoboot` (8,9 Ko : boot2 + choix + saut) |
| `0x10000` | 480 Ko | slot 0 : **firmware Neo6502** (187 Ko) — défaut |
| `0x88000` | 480 Ko | slot 1 (ex. reload BBC : 230 Ko + images disque) |
| `0x100000` | 480 Ko | slot 2 (ex. reload Oric : 190 Ko) |
| `0x178000` | 480 Ko | slot 3 (ex. reload Apple //e) |

## Chaîne
1. **Image** : lier avec `multiboot/memmap_slot_N.ld` (copie de `memmap_default.ld` du SDK,
   seule la région FLASH change). Firmware Neo : `make -C firmware build-multiboot`
   (`-DNEO_MULTIBOOT_SLOT=0`, répertoire `build_mb`) → `firmware_slot0.uf2`. reload :
   `pico_set_linker_script(bbc …/memmap_slot_1.ld)` (à faire dans le fork reload).
2. **Sélecteur** : `cmake -S multiboot -B multiboot/build -DPICO_SDK_PATH=… && make -C multiboot/build`.
   Lit `scratch[0]` (`0x4E454F00 | slot`, posé par 1,14), efface (un coup), vérifie que
   le slot contient une table de vecteurs plausible (SP en SRAM, PC dans le slot), copie
   le `boot2` de l'image en RAM et l'appelle, puis vectorise dans l'image. Slot vide →
   slot 0 ; rien du tout → LED clignotante.
3. **Image unique** : `multiboot/mkimage.py -o neo6502-multi.uf2 --selector neoboot.uf2
   --slot 0 firmware_slot0.uf2 --slot 1 bbc_slot1.uf2` — vérifie que chaque bloc UF2
   tombe dans son slot et affiche le nom `binary_info` de chaque image. À glisser sur
   `RPI-RP2`. Une image peut aussi être flashée seule plus tard (son UF2 lié pour le
   slot) : **le menu s'adapte à ce qui est réellement en flash** (demande bmarty).
4. **Détection** (`neoboot.h`, partagée sélecteur/firmware) : un slot est occupé si sa
   table de vecteurs est plausible (SP en SRAM, PC dans le slot, bit thumb) ; son nom
   est le `program name` du `binary_info` du SDK (en-tête juste après les vecteurs :
   marqueurs `0x7188ebf2`/`0xe71aa390`, entrée `ID_AND_STRING` tag `RP` id
   `0x02031c86`) — `firmware` → « Neo6502 » via `pico_set_program_name`, reload → « bbc »,
   « oric »… ; « image n » si l'image n'a pas de binary_info.
5. **API** (F-81, groupe 1) : **1,14 Reboot Image** (P0 = slot ; ne revient pas) ;
   **1,15 Get Image Name** (P0 = slot, P1-2 = tampon préfixé). Erreur si le firmware ne
   tourne pas dans un slot (flashé à 0, sans sélecteur) ou si le slot est vide ; idem
   dans `neo`/Phosphoneo.
6. **Télémon** (Neo6502kbd) : le menu `O` liste `F1..F3 <nom>` (slots occupés seulement) et `Fn` redémarre sur l'image.

## Côté reload-emulator (fork `benedictemarty`, branche `bbc`)
`cmake -DNEO_MULTIBOOT_DIR=~/Neo6502firmware/multiboot -DNEO_SLOT_BBC=1 -DNEO_SLOT_ORIC=2`
(`platforms/rp2040/build_mb`) → `bbc.uf2` (257 Ko) et `oric.uf2` (186 Ko) liés pour leurs
slots ; touche **Pause** = retour au firmware Neo (`neo_multiboot.h`). Image complète :
`mkimage.py … --slot 0 firmware_slot0.uf2 --slot 1 bbc.uf2 --slot 2 oric.uf2` (639 Ko).

## Vérifié (Phosphoneo `make test-multiboot`, sonde `tools/cosim/multiboot_probe.c`)
Flash émulée = `neoboot.elf` + `firmware.bin` (slot 0) copié à `0x10000` : le sélecteur
entre dans le slot au pas 3 000 (`0x100101F6`), le firmware relié atteint `CPUExecute`
en 15,0 M pas comme le firmware normal ; avec les images reload dans les slots 1 et 2,
`HWGetImageName` (1,15) renvoie « Neo6502 », « bbc », « oric », « vide » ; `scratch[0] =
'NEO'|1` (resp. 2) fait entrer le sélecteur dans le slot 1 (resp. 2) au pas ≈ 3 000.

## Non vérifié / risques (carte)
| # | Risque | Vérification |
|---|---|---|
| R11 | Appel du `boot2` d'une image depuis la RAM : il reconfigure le SSI pendant qu'on exécute depuis la SRAM — c'est ce que fait le bootrom, mais avec un XIP déjà actif (celui du sélecteur) | si écran noir : sauter sans rappeler `boot2` (config du sélecteur, clkdiv 4, conservée) |
| R12 | `scratch[0]` après un reset par le bouton RESET/RUN : je ne sais pas s'il est conservé (le datasheet garantit la survie au reset watchdog) — si conservé, un reset matériel relancerait l'image demandée une fois (le sélecteur l'efface ensuite) | tester RUN après 1,14 |
| R13 | Retour depuis une image reload : elle doit faire `scratch[0] = 0` + `watchdog_reboot` (F-82, touche à définir dans `hid_app`) ; sinon, coupure d'alimentation → slot 0 | — |
| R14 | Images plus grosses que 480 Ko (reload avec plusieurs `.ssd` embarqués) : ajuster `NEOBOOT_SLOT_SIZE` (2 slots de 960 Ko possibles) | `mkimage.py` refuse un bloc hors slot |
