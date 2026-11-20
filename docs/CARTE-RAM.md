# Carte de la RAM du RP2040 (Trinity 0.16.110)

Relevée le 2026-11-20 dans `firmware/firmware.elf` (`arm-none-eabi-size -A`, `arm-none-eabi-nm -S`). Les nombres de ce
fichier sont mesurés, sauf ceux marqués « étude » (repris de `ETUDE-PERF-MEMOIRE-2026-10-12.md`, non revérifiés).
À refaire à chaque version qui change `RAM_LIMIT` (`firmware/Makefile`).

RAM totale : 270 336 octets, de `0x20000000` à `0x20042000`.

| Zone | Adresses | Octets | Contenu |
|---|---|---|---|
| Vecteurs | `0x20000000` | 192 | table des vecteurs en RAM |
| `.data` | dès `0x20000040` | 16 720 | code copié en RAM (`DSPHandler`, IRQ DMA, cœur 1…) et données initialisées |
| `.bss` | jusqu'à `0x20039288` (`__bss_end__`) | 217 208 | voir plus bas |
| **Statique** (`.data` + `.bss` + vecteurs) | | **234 120** | `RAM_LIMIT` = 234 400 (marge 280 o) |
| Tas | `0x20039288` à `0x20040000` | 28 024 | `malloc` : tampons TMDS, tampons réservés à l'usage (T-152, T-155) |
| `scratch_x` | `0x20040000` à `0x20041000` | 4 096 | 892 o d'encodeurs ; pile du cœur 1 (`__StackOneTop` = `0x20041000`) |
| `scratch_y` | `0x20041000` à `0x20042000` | 4 096 | pile du cœur 0 (`__StackTop` = `0x20042000`) |

Étude : `0x20041000`–`0x20041800` n'est pas libre, c'est la marge de la pile du cœur 0 (sans garde, profondeur inconnue).
Tampons TMDS du tas ≈ 11,7 Ko (modes 0 et 2), 13,1 Ko (mode 1), pic mesuré 15 380 o (T-79).

## Plus gros postes statiques

| Poste | Octets | Remarque |
|---|---|---|
| `graphicsMemory` | 76 800 | écran 320 × 240 ; 2 pages Hercules = 63 000 (graphics.h) |
| `cpuMemory` | 65 536 | mémoire du 6502 |
| `gfxObjectMemory` | 32 768 | page `$90` : sprites, tuiles, plan AGI, flux de son (contrat de l'API) |
| `ramBank` | 8 192 | banque RAM (T-124) |
| `consoleMemory` | 4 960 | contrat de l'API (T-13) |
| `DSPHandler` (en RAM) | 3 648 | groupes 1, 2, 4, 5, 6 (T-153, T-154 ont sorti les autres vers `DSPRare`, en flash) |
| `sprites` | 3 072 | `static_assert` (T-113) |
| `msc_fatfs_volumes` | 2 840 | |
| `scanBuffer` | 2 816 | |
| `cdch_data` | 2 784 | |
| `core1_stack`, `rxBuffer` | 2 048 chacun | |
| `dvi0` | 1 736 | |
| `dvi_dma_irq_handler`, `_scanline_callback`, `core1_main` | 1 652, 1 352, 1 132 | cœur 1 : jamais en flash (`make test-coeur1`) |

Les cinq premiers (`graphicsMemory`, `cpuMemory`, `gfxObjectMemory`, `ramBank`, `consoleMemory`) font 188 256 octets,
80 % du statique. Code en RAM au total : 16 802 octets.

## Historique des gains

| Version | Ticket | Statique |
|---|---|---|
| 0.16.106 | | 240 200 |
| 0.16.107 | T-152 | 238 560 (−1 640) |
| 0.16.108 | T-153 | 236 080 (−2 480) |
| 0.16.109 | T-154 | 235 136 (−944) |
| 0.16.110 | T-155 | 234 120 (−1 016) |

## Ce qui reste, et ce qui est écarté

- Reste : M5 est fait (T-155) ; M7 (`DBGCommand`/`DBGReport*` en flash, ≈ −1,2 Ko), M8 (`strtod`, −364 o), `edCurrentLine`
  (256 o, besoin d'un crochet d'entrée/sortie de l'éditeur), groupes 1, 2, 4, 5, 6 de `DSPHandler` (appelés en boucle : mesure sur carte d'abord).
- Fusionner `graphicsMemory` et `gfxObjectMemory`, ou les mettre dans le tas : aucun gain (somme identique, les deux servent
  ensemble en mode 0/2), et les adresses `$80`/`$90` sont un contrat avec le 6502.
- Déjà refusé : voir `ETUDE-PERF-MEMOIRE-2026-10-12.md` (T-104, T-124, T-71, T-58).
