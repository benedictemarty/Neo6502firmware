# F-90 — Série USB CDC : modems et adaptateurs USB-série sur le port hôte (groupe 14)

Date : 2026-09-15. Demande bmarty : « ajouter la prise en charge des modems CDC »
(Neo6502drive EPIC-02 télécom, modem Hayes en premier ; US-C1..C6 déléguées au fork).
État : **fait et vérifié sur le vrai firmware en co-simulation** (classe TinyUSB `cdc_host`
réelle, modem émulé par libemul) ; **non exécuté sur carte** (aucun Neo6502 disponible).

## Ce que c'est
Un périphérique **USB CDC-ACM** (modem USB, adaptateur USB-série, Pico « WiFi modem »)
branché sur le port USB hôte du Neo6502 devient une liaison série accessible au 6502 par
l'API. Les adaptateurs FTDI et CP210x sont pris en charge par la même classe TinyUSB
(`CFG_TUH_CDC_FTDI` / `CFG_TUH_CDC_CP210X`). Deux périphériques simultanés
(`CDC_MAX_DEVICES`), choisis par Parameter:7 (0 = premier).

## API — groupe 14 « USB Serial (CDC) »
| Fonction | Entrée | Sortie |
|---|---|---|
| 14,1 Status | P7 = périphérique | P0 = 1 si connecté, P1-2 = octets en attente |
| 14,2 Read Byte | P7 | P0 = octet ; erreur si rien / pas de périphérique |
| 14,3 Write Byte | P0 = octet, P7 | erreur si pas de périphérique |
| 14,4 Read Block | P0-1 adresse, P2-3 max, P7 | P2-3 = octets lus (peut être 0) |
| 14,5 Write Block | P0-1 adresse, P2-3 nombre, P7 | P2-3 = octets écrits |
| 14,6 Set Line Coding | P0-3 bauds, P4 bits (8), P5 parité (0 N/1 O/2 E), P6 stop (1/2), P7 | erreur si pas de périphérique (beaucoup de modems USB l'ignorent) |

Au branchement le firmware programme 115200 8N1 et lève DTR/RTS (les modems ne répondent
pas sans DTR).

## Implémentation
- Commun : `interface/cdcserial.{h,cpp}`, `config/system/group14_cdc.inc` (documentation API).
- Carte : `hardware/cdc.cpp` (rappels `tuh_cdc_mount_cb/umount_cb`, table des interfaces,
  `HWCDC*` sur `tuh_cdc_*`) ; `tusb_config.h` : `CFG_TUH_CDC 2`, FTDI/CP210x, RX 1 Ko.
  La classe est servie par `tuh_task` (déjà appelé par `KBDSync`).
- PC (`neo`, Phosphoneo) : `emulator/src/core/cdc_host_tty.cpp` — le « périphérique » est
  un tty/pty/fichier de l'hôte donné par `NEO_CDC_TTY` (et `NEO_CDC_TTY1`).
- Co-sim Phosphoneo : `--cdc-tty PATH` — `emul_cdc.c` de libemul (HLE des `tuh_cdc_*`)
  ponté sur le pty, puis le vrai `tuh_cdc_mount_cb` du firmware est joué.

## Vérifié
`make test-cdc` (Phosphoneo) : faux modem Hayes (`tools/fake_modem.py`, pty) ; le
programme `hayes.neo6502` attend 14,1 = connecté, envoie `ATI` puis `ATZ`, affiche
« Neo6502 fake modem 1.0 / OK / OK / FIN » — identique sur le backend comportemental et
sur le vrai firmware en co-sim. Pour un vrai modem sur PC : `NEO_CDC_TTY=/dev/ttyACM0 bin/neo`.

## Vérifié avec un vrai modem (2026-09-15)
Pico W Wi-Fi modem de Neo6502drive (`2e8a:000a`, `/dev/ttyACM0` sur le PC) :
`NEO_CDC_TTY=/dev/ttyACM0` (comportemental) et `--cdc-tty /dev/ttyACM0` (vrai firmware
en co-sim) → `ATI` = « Neo6502drive Pico W modem 0.1.0 », `ATZ` = `OK`. Le modem fait
l'écho des commandes (`ATE0` pour le couper).

## Risques carte
| # | Risque | Vérification |
|---|---|---|
| R15 | Énumération d'un modem composite (CDC + MSC, hubs) : `CFG_TUH_DEVICE_MAX` 5, tampon d'énumération 256 | brancher un vrai modem ; si échec, `CFG_TUH_ENUMERATION_BUFSIZE` 512 |
| R16 | Débit : `tuh_task` n'est appelé qu'au rythme de `DSPSync` (~100 Hz) → RX 1 Ko suffit à 115 200 bauds pendant 10 ms (≈ 115 octets) ; au-delà, pertes possibles si le 6502 ne lit pas | mesurer avec un transfert continu ; option : appeler `tuh_task` aussi dans 14,1/14,4 |
| R16b | **Avéré sur carte (2026-09-19, Trinity 0.2.0 + ProphetGui)** : la lecture par blocs routée (10,13 → `UARTRReadBlock`) attendait sans servir `tuh_task` → seuls les octets déjà dans le FIFO étaient lus, timeout 5 s sur un corps HTTP de 1 Ko (« liste indisponible »). Corrigé : `KBDSync()` dans la boucle (Trinity 0.2.1, `bmarty/main`). `CDCReadBlock` (14,4) rend ce qui est disponible sans attendre : non concerné. | Trinity 0.2.1 sur carte : `prophetgui.neo` doit afficher la grille |
| R17 | Coût mémoire : `CFG_TUH_CDC 2` × (1 Ko RX + 256 TX) + FTDI/CP210x | `arm-none-eabi-size` : à surveiller (SRAM ~47 Ko libres avant F5) |

## Suite (Neo6502drive EPIC-02)
US-T1 « modem Hayes » peut maintenant être écrite côté 6502 sur le groupe 14 (le programme
`hayes.asm` en est l'embryon) ; terminal série, XMODEM, pile TCP par modem WiFi (AT+CIP…).
