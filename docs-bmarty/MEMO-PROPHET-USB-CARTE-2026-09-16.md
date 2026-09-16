# Mémo — Prophet par USB sur le Neo6502 physique (demande de test sur carte)

De : projet Neo6502Prophet — 2026-09-16
Pour : **Neo6502firmware** (fork bmarty : F-90 hôte USB CDC, F-93 routage UART→CDC)
et **Neo6502drive** (modem Pico W). Demande du PO : valider sur la **carte réelle**
la chaîne « client Prophet → USB → Pico W → Wi-Fi → prophet.3617.fr ».

## Ce qui est déjà prouvé (co-simulation, aujourd'hui)
Phosphoneo (vrai firmware du fork, routage F-93) + **vrai Pico W** sur
`/dev/ttyACM0` :
- `prophet.neo` (client amont, non modifié) : catalogue, `list`, `cat` OK
  (validé par Neo6502drive) ;
- ProphetGui (`~/Neo6502ProphetGui/build/prophetgui.neo`) : grille, fiche,
  jaquette, téléchargement de 22 Ko avec SHA-256 vérifié, en HTTP :8998 et
  en HTTPS :443 (`AT+TLSPORT=443`) — `make real`.
Les deux clients n'utilisent que l'API UART (10,13-18) : c'est le firmware qui
route vers le modem CDC (10,19 = AUTO).

## Ce qui n'est PAS vérifié
- **F-90 sur carte** : « carte non testée » dans le backlog du fork. Aucun
  programme n'a encore parlé au Pico W **branché sur le port USB hôte du
  Neo6502 physique**.
- Le firmware fork n'est peut-être pas flashé sur la carte du PO.

## Procédure proposée (à exécuter par l'équipe firmware/drive)
1. Flasher le fork (`make -C firmware build STORAGE=usb|sd`, UF2 via BOOTSEL) —
   garder l'UF2 officiel pour revenir en arrière.
2. Brancher le Pico W (firmware modem 0.2.0, SSID déjà enregistré) sur le port
   USB hôte du Neo (hub si le clavier/la clé USB l'occupent : à vérifier —
   cohabitation clavier HID + stockage + CDC sur le même hub).
3. Copier `prophetgui.neo` (et `prophet.neo` pour comparaison) sur le stockage ;
   `run "prophetgui.neo"`. Attendu : « PROPHET » puis la grille en < 2 s ;
   `c` pour changer serveur/port si besoin (`prophet.3617.fr`, 8998).
4. Fiche (entrée) → jaquette 160×120 ; `g` → téléchargement + « empreintes
   verifiees ». Comparer l'empreinte du fichier écrit avec
   `https://prophet.3617.fr/sha256/<id>/<n>`.
5. Relever : durée du catalogue, débit du téléchargement (Ko/s), stabilité
   (10 téléchargements), comportement à la déconnexion du modem.
6. Variante TLS : `AT+TLSPORT=443` sur le modem puis port 443 dans la config.

## Points d'attention connus
- Le modem 0.2.0 a émis une fois une réponse **sans `CONNECT` ni `+IPD`**
  (mémo `MEMO-PROPHET-MODEM-IPD-2026-09-16.md` chez Neo6502drive) ; ProphetGui
  tolère le cas, `prophet.neo` non.
- Timeouts de ProphetGui en trames 60 Hz (3 s commandes, 10 s connexion,
  30 s données) : valeurs pour la carte réelle, non ajustées sur matériel.
- Consommation USB du Pico W (Wi-Fi) sur le port hôte du Neo : non mesurée.

Résultats attendus en retour : oui/non par étape, captures ou photos, journal
`AT` si possible (le Pico W écho sur USB CDC et UART0 simultanément).
