# Trinity — firmware Neo6502 amont + modem USB

Branche `trinity` (bmarty, 2026-09-18) : le firmware **amont** (`v1.0.0-14-gdc70908`, Paul Robson, MIT)
plus **une seule chose** : la reconnaissance d'un modem série USB (Pico W « picowifiusb », CDC-ACM) sur
un port USB-A de la carte — F-90 (groupe 14, `cdc.cpp`, `cdcserial.cpp`, TinyUSB `cdc_host`) et F-93
(routage des fonctions UART 10,13-10,18 vers le modem, 10,19, AUTO par défaut). Rien d'autre du fork
`bmarty/main` (toolbox, banques, modes vidéo, R22…).

Bannière : `Trinity Firmware: v0.0.1` (tag `trinity-v0.0.1` ; entre deux tags : `v0.0.1-N-gXXXXXXX`). Compilation : comme l'amont
(`make -C firmware build STORAGE=USB`, SDK 1.5.1, TinyUSB 0.16.0, PicoDVI amont non modifié).

## Versions

- **0.1.0** (2026-09-19) — **TinyUSB 0.21.0** au lieu de 0.16.0 (amont) : le pilote hôte RP2040 de 0.16 lisait
  **un secteur en 2,5 s** derrière le hub 4 ports (transferts bulk sur les « interrupt endpoints », latches
  partagés — tinyusb #3533/#1261) ; avec la refonte HCD de 0.21 (EPX, double tampon) : **1 ms**. Mesuré sur
  carte (clé 8 Go FAT32, clavier, modem). Conséquences : FatFs (R0.15) copié dans `firmware/lib/fatfs`
  (TinyUSB 0.21 ne le livre plus), `tinyusb_board` retiré (BSP pour SDK 2.x, inutilisé). Messages console
  `MSC SCSI inquiry failed` / `MSC filesystem mount failed, FatFs error NN` quand une clé n'est pas montée.
  Constats carte du 2026-09-18 : alimenter la carte par un bloc secteur (le port USB-C d'un PC ne suffit
  pas : clavier/hub décrochent) ; une clé 60 Go USB 3 n'énumère pas, une 8 Go USB 2 oui.
- **0.0.1** (2026-09-18) — amont `dc70908` + modem USB CDC (F-90/F-93), message `USB serial modem found`.
