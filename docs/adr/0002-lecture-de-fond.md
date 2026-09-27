# ADR-0002 — Lecture de fichier en tâche de fond (T-82)

Statut : **acceptée** le 2026-09-27 (bmarty : « go »), proposée le même jour (bmarty : « téléchargement direct mémoire pour USB ou SD », précisé : « le 6502
lance la lecture et continue à tourner »).
Portée : `firmware/sources/system/processor_pio.cpp` (boucle du bus), `firmware/sources/hardware/storage/usb_storage.cpp`
(`wait_for_disk_io`), `firmware/common/config/system/group3_fileio.inc` (nouvelle fonction), `fileinterface.cpp`,
l'émulateur `neo`.

## Contexte

- Le cœur 0 fabrique l'horloge du 6502 : la boucle `CPUExecute` sert chaque accès du bus à la PIO. Quand le 6502
  écrit le port de commande (`$FF00`), elle appelle `DSPHandler` et ne revient au bus qu'à la fin de la commande :
  **pendant une lecture de fichier, le 6502 est arrêté** (T-49).
- Mesure carte du 2026-09-27 (SWD, `sdbench.neo`, 0.16.42) : pendant les lectures, le cœur 0 passe **92 %**
  (lectures dispersées) à **≈ 95 %** (séquentielles) du temps dans `wait_for_disk_io`, c'est-à-dire à attendre que le
  contrôleur USB finisse un transfert, en tournant `tuh_task()` à vide.
- FatFs est synchrone et vit en flash ; le découper en tranches par `DSPSync` ne rendrait rien au 6502 (il est
  arrêté pendant chaque tranche).

## Décision proposée

**Servir le bus du 6502 pendant l'attente USB**, au lieu de tourner à vide, pour une nouvelle fonction explicite :

1. **3,28 File Read Background** — mêmes paramètres que 3,27 (canal, page, adresse, taille), plus l'adresse d'un
   **octet d'état dans la RAM du 6502** (P6-P7). Le firmware lit tous ses paramètres, écrit `$01` (en cours) dans
   l'octet d'état, **libère aussitôt le 6502** (`API_COMMAND` = 0), puis exécute la lecture FatFs habituelle.
2. Pendant cette lecture, `wait_for_disk_io` alterne **un appel à `tuh_task()`** et **une salve d'accès au bus**
   servis par le même code que la boucle principale (fonction en ligne commune, en RAM, pour garder exactement le
   même minutage PIO, les `nop` compris, et la libération d'IRQB sur la lecture de `$FFFF`).
3. À la fin, l'octet d'état reçoit `$80 | code d'erreur` et, à côté, le nombre d'octets lus (P4-P5 habituels).
   **Le 6502 sonde son octet en RAM**, sans appel d'API.
4. **Si le 6502 appelle l'API pendant la lecture** (écriture de `$FF00`), la commande est **mise de côté** : le 6502
   voit `API_COMMAND` ≠ 0 et attend, comme aujourd'hui ; elle est exécutée dès la fin de la lecture. Aucun appel
   imbriqué dans FatFs ou TinyUSB.
5. Les lectures existantes (3,8, 3,27, chargements) **ne changent pas** : le service du bus pendant l'attente n'est
   actif que pour 3,28.
6. **Émulateur `neo`** : 3,28 est exécutée en une fois (état `$80` au retour) ; même contrat, pas de parallélisme.

## Contrat pour le programme 6502

- Ne pas lire ni écrire la zone de destination avant que l'octet d'état ait le bit 7 ; ne pas toucher à l'octet
  d'état lui-même.
- Tout appel d'API pendant la lecture est permis mais **attend la fin** de la lecture.
- Les interruptions du 6502 (tic 1,12, trame) continuent : elles viennent d'une alarme et du cœur 1, pas de
  `DSPSync`.

## Conséquences

- Gain attendu : le 6502 récupère l'essentiel des ≈ 90-95 % d'attente mesurés (flux son T-79, jeux qui chargent
  pendant qu'ils animent). Bonus probable pour T-31 : `tuh_task()` continue, donc le clavier aussi.
- **Risque principal : la boucle du bus.** C'est le code le plus sensible du firmware (T-46, T-48 : le placement et
  le minutage en flash ont déjà causé des pannes dépendant du binaire). La mise en facteur du corps de la boucle doit
  produire un code machine **identique** pour la boucle principale ; à vérifier par désassemblage avant et après,
  puis sur carte avec les tests existants (`sdbench`, `bus.neo`, NeoDune2000).
- `DSPSync` n'est pas appelée pendant une lecture de fond (comme aujourd'hui pendant toute commande) : le port de
  debug et la surveillance de l'écran patientent le temps de la lecture.
- Pas de nouvelle mémoire notable : quelques octets d'état (règle T-13).

## Étapes

1. Mise en facteur du service du bus, **sans changement de comportement** ; désassemblage comparé, tests `neo`,
   validation carte (rien ne doit changer).
2. 3,28 et le service pendant `wait_for_disk_io` ; test `neo` du contrat ; outil carte qui mesure le temps rendu au
   6502 (un compteur incrémenté par le 6502 pendant une lecture de 256 Ko).
3. Documentation de l'API, mesure carte, décision de garder.

## Alternatives écartées

- **Tranches par `DSPSync`** : ne rend rien au 6502 (il est arrêté pendant chaque tranche).
- **Lectures `tuh_msc_read10` asynchrones sur une liste de secteurs précalculée** : plus propre en théorie, mais
  demande de contourner FatFs (chaîne de clusters, tampon de secteur partiel) ; à reconsidérer seulement si l'étape 2
  échoue.
- **Toutes les lectures en tâche de fond** : casserait le contrat des programmes existants, qui supposent que la
  destination est remplie au retour de 3,8 / 3,27.

## Résultat (2026-09-28, carte, Trinity 0.16.48)

`BG.NEO` sur `sdbench.dat` (256 Ko par tranches de 4 Ko) : 3,27 = 28 cs, 3,28 = 29 cs ; pendant 3,28 le 6502 a
tourné ≈ 70 % du temps (101 193 tours de boucle pour 143 434 possibles) ; aucune erreur, aucun décrochage du bus
(RXUNDER/TXOVER = 0). Étape 1 : binaire identique octet pour octet. Décision : garder.
