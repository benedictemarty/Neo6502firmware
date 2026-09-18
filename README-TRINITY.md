# Trinity — firmware Neo6502 amont + modem USB

Branche `trinity` (bmarty, 2026-09-18) : le firmware **amont** (`v1.0.0-14-gdc70908`, Paul Robson, MIT)
plus **une seule chose** : la reconnaissance d'un modem série USB (Pico W « picowifiusb », CDC-ACM) sur
un port USB-A de la carte — F-90 (groupe 14, `cdc.cpp`, `cdcserial.cpp`, TinyUSB `cdc_host`) et F-93
(routage des fonctions UART 10,13-10,18 vers le modem, 10,19, AUTO par défaut). Rien d'autre du fork
`bmarty/main` (toolbox, banques, modes vidéo, R22…).

Bannière : `Trinity Firmware: v0.0.1` (tag `trinity-v0.0.1` ; entre deux tags : `v0.0.1-N-gXXXXXXX`). Compilation : comme l'amont
(`make -C firmware build STORAGE=USB`, SDK 1.5.1, TinyUSB 0.16.0, PicoDVI amont non modifié).
