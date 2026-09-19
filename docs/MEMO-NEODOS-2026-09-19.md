# Mémo — NeoDOS (interpréteur de commandes façon MS-DOS) et Trinity

De : projet **Neo6502Msdos** (NeoDOS v0.8.0, `github.com/benedictemarty/neodos`) — 2026-09-19
Pour : projet **Neo6502firmware**, branche `trinity` (firmware de référence). Information et
demandes à mettre au backlog ; rien de ce qui suit n'est encore validé sur carte.

## Ce qu'est NeoDOS
Un `.neo` résident en `$C000-$FBFF` (assembleur 64tass, ~10 Ko de code) qui remplace NeoBASIC
comme environnement de commande : invite `A:\>`, `DIR CD MD RD DEL REN COPY MOVE XCOPY TYPE ATTRIB
CLS VER VOL MEM DATE TIME PATH PROMPT ECHO PAUSE REM IF GOTO CALL HELP EXIT`, jokers `* ?`,
redirection `>`/`>>`, scripts `.BAT` (`%0-%9`, `ERRORLEVEL`, `AUTOEXEC.BAT`), éditeur de ligne
avec historique, Ctrl+Alt+Suppr = redémarrage à chaud, lancement des `.NEO` (retour à l'invite
par `RTS`). Tests : 27 cas headless (Phosphoneo), documentation dans le dépôt (`docs/`).

## Démarrage sur Trinity
`make dist` produit `build/dist/` à copier à la racine de la clé : **`boot/neodos.neo` +
`boot/auto.txt`** (`neodos.neo`) → NeoDOS démarre seul (T-04), `AUTOEXEC.BAT` (`PATH \BIN`),
`BIN/` (commandes externes). `EXIT` revient à NeoBASIC (1,3 puis `jmp (0)`).
**À valider sur carte** (aucune session carte côté NeoDOS).

## Fonctions du firmware utilisées
- Système : 1,2 Key Status (Ctrl+Alt+Suppr), 1,3 Basic, 1,11 Version, **1,20/1,21 Date-Time (fork)**.
- Console : 2,1 Read Character (éditeur de ligne : codes 1/4/5/8/19/20/23/24/26/27 de la console),
  2,6 Write, 2,12 CLS ; le `ReadLine` du noyau ($FFEB) n'est plus utilisé depuis la 0.6.0.
- Fichiers : 3,2 Load File (+ `JSR $FF08`), 3,4/5/6/8/9/10 (canaux 0, 1 et 7), 3,12 Rename, 3,13
  Delete, 3,14 MkDir, 3,15 ChDir, 3,16 Stat, 3,17/18/19 Dir, 3,20 Copy, 3,21 Attributes, 3,23 CWD,
  **3,24/25/26 Volumes (fork, F-102)**. Son : 8,1 (redémarrage à chaud).

## Comportement sur Trinity (sans les fonctions du fork)
Sur carte une fonction inconnue ne touche ni les paramètres ni `$FF02` : NeoDOS 0.8.0 **sonde**
au démarrage 3,26 (P0 préchargé à `$FF`) et 1,20 (P7 préchargé à `$FF`) et se dégrade : lecteur
toujours `A:`, `VOL`/`DIR` affichent « has no label », `B:` → « Invalid drive specification »,
`DATE`/`TIME` → « Date/time not supported by this firmware », `$d`/`$t` de `PROMPT` ignorés.
Le reste (fichiers, scripts, éditeur) n'utilise que l'amont.

## Demandes (proposition de stories T-xx)
1. **Volumes 3,24-3,26** (reprise F-102, listée dans T-09) : `A:`/`B:` réels (clé + SD, ou deux
   clés) — c'est la fonctionnalité DOS la plus visible qui manque.
2. **Date/heure 1,20-1,21** (reprise F-14) : `DATE`, `TIME`, `$d $t`, et surtout **horodatage
   FAT** des fichiers créés par NeoDOS (`COPY`, `>`).
3. **Dates dans 3,18 Read Directory / 3,16 Stat** (nouveau) : `DIR` affiche aujourd'hui nom et
   taille seulement (MS-DOS montre date et heure). Proposition : P7 de 3,18 inutilisé → renvoyer
   date/heure FAT compactées (2 × 16 bits) via un tampon ou une fonction `3,28 Stat Extended`.
4. **`neo` (émulateur officiel) : normaliser `..`** dans `FISChangeDirectory`/`FISGetCurrentDirectory`
   (`A:\GAMES\..` au lieu de `A:\`) et rendre le stockage hôte **insensible à la casse** (option) :
   sur FAT la carte l'est, pas l'hôte Linux — les tests de NeoDOS contournent (noms en majuscules).
   Même remarque pour Phosphoneo (même code).
5. **Documenter le contrat `$FF08`** après 3,2 (`JMP exec` / `RTS`) dans `api-listing.md` :
   NeoDOS et le menu `boot/` en dépendent.

## Contrat des commandes externes (information)
NeoDOS recopie la ligne de commande en **`$0200`** (pstring, 200 caractères max) avant `JSR $FF08` ;
un programme `.NEO` peut la lire (exemple `BIN/ARGS.NEO`). Un programme qui écrit au-dessus de
`$C000` détruit NeoDOS. Sans rapport avec le firmware, mais utile à connaître pour `boot/`.

## Complément 2026-09-20 — survie de NeoDOS (T-11)
NeoBASIC « survit » aux programmes parce que `1,3` le recharge depuis la flash ; NeoDOS 0.8.2 fait
pareil depuis le disque avec un **stub en `$0100`** (poussé comme adresse de retour : somme de
contrôle du code, sentinelles, puis 3,2 `/boot/neodos.neo` + `$FF08`). Fragile si un programme
utilise `$0100-$01A0`. Le firmware peut rendre ce stub inutile : que `1,3` applique `boot/auto.txt`
à chaque appel (`BOOTLoadChoice` n'agit qu'au premier), ou une fonction `1,22 Reload Boot Program`
— story **T-11**. Constaté au passage sur carte (bmarty) : NeoDOS 0.8.0 gelait `poker.neo`
(chargé en `$0200`) — corrigé en 0.8.1 (plus de copie de la ligne de commande dans la zone
programme ; pointeur en `$C00C`).
