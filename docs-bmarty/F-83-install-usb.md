# F-83 — Installer une image firmware (`.uf2`) depuis une clé USB dans un slot multi-boot

Question bmarty (2026-09-16) : « [le sélecteur d'OS avec] les `.uf2` sur une clé USB ».
État : **conception**, non implémenté (card-only, invérifiable en co-simulation).

## Le fait physique
Le RP2040 exécute son code depuis la flash interne (XIP `0x10000000`). Une image reload
fait ~280 Ko ; la SRAM (264 Ko) ne suffit pas à l'exécuter depuis la RAM. **On ne peut
pas booter un `.uf2` depuis la clé** : il faut le **programmer dans un slot de la flash**
(`flash_range_erase` + `flash_range_program`, SDK), puis redémarrer dessus (F-81, 1,14).

## Chemin retenu : par le firmware Neo (pas par le sélecteur)
Le firmware Neo a déjà USB hôte + MSC + FatFS. Le sélecteur `neoboot` reste minuscule
(8,9 Ko) et ne gagne pas d'USB. On ajoute une fonction au firmware Neo :

- **API 1,16 Install Image** : P0-1 = nom de fichier (`os/bbc.uf2`), P2 = slot cible (1-3).
  Lit le `.uf2` bloc par bloc (512 octets : adresse cible + 256 octets utiles), vérifie que
  chaque bloc tombe dans le slot (`neoboot_slot_base(slot) .. +NEOBOOT_SLOT_SIZE`), efface
  puis programme la flash. Erreur si le fichier n'est pas un `.uf2` RP2040, ou hors slot.
- **API 1,17 Erase Slot** (optionnel) : vide un slot (le menu le montre alors « libre »).
- Le Télémon `O` liste les `.uf2` du répertoire `os/` de la clé **à installer** à côté des
  slots déjà présents (1,15). Choisir un `.uf2` → 1,16 → 1,14 : flashé (~2 s) puis lancé.

Avantage : **une clé porte autant de machines qu'on veut** (BBC, Master, Oric, Apple, jeux
natifs…), installées à la demande, sans refaire une image combinée sur PC. Le `.uf2` doit
être lié pour le slot (`memmap_slot_N.ld`) — ou l'installateur relocalise (les blocs UF2
portent leur adresse ; on peut soustraire un décalage de slot connu).

## Risques (carte)
| # | Risque | Vérification |
|---|---|---|
| R18 | Programmer la flash pendant l'exécution : l'écriture doit être en RAM, IRQ coupées, et **core1 (DVI) gelé** le temps de l'effacement (XIP off) — sinon plantage | `flash_safe_execute` du SDK (arrête core1) ou pause manuelle du DVI |
| R19 | Le firmware Neo tourne depuis le slot 0 ; écrire un autre slot est une région distincte, donc sûr — mais **réécrire le slot 0** (mise à jour du Neo lui-même) exige un mini-loader en RAM | interdire l'auto-écriture du slot courant, ou loader RAM |
| R20 | Relocalisation d'un `.uf2` lié pour un autre slot | refuser, ou soustraire le décalage bloc par bloc (boot2 et table de vecteurs restent à +0/+0x100 du slot) |
| R21 | Invérifiable en co-sim (libemul ne modélise ni l'écriture flash ni l'hôte USB MSC) | **carte obligatoire** |

## Alternative (déjà là)
Image combinée en flash (F-80, `make -C multiboot image`) : jeu figé, un seul `.uf2` à
glisser depuis un PC, bascule instantanée. F-83 est le complément « sans PC, depuis une clé ».
