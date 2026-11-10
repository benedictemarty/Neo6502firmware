# Étude : une « banque » dont le stockage serait un fichier de la clé USB — T-140

Demande de bmarty (2026-11-10, « peut-on faire de la RAM sur banque USB », puis « go » sur la variante de 1,18). Étude en lecture seule : aucun code
modifié, rien d'essayé sur carte hors les mesures de T-139 (carte en 0.16.96, une seule clé).

## 1. Ce que fait 1,18 aujourd'hui (vérifié dans `firmware/common/sources/interface/banks.cpp`)

- `BNKSelect` fait un `memcpy` de 8 Ko de la flash (XIP) vers une fenêtre de la RAM 6502, pendant que le 65C02 attend l'appel.
  Il n'y a **aucun test d'accès par banque** : la boucle de bus sert `cpuMemory[]` directement. Une « banque » est donc une copie, pas un mappage.
- 1,22 (`BNKWrite`) efface et programme la flash : écran noir ≈ 150 ms, usure de la flash, 32 banques seulement.
- Les banques sont aussi les pages blitter `$A0`-`$BF` (lecture seule : 12,2, images de sprites 6,x). 3,27 refuse ces pages en destination.

## 2. Conséquence : une variante « fichier » de 1,18 n'apporte presque rien

Parce que 1,18 est déjà une copie, une banque sur USB revient à **3,6 Seek + 3,8 Read de 8 192 octets vers la fenêtre**, ce que le 6502 sait déjà faire
sans changer le firmware. Mesuré (T-139) : **47,3 ms** en position aléatoire, 18,2 ms si consécutif. Le « chargement » d'une banque USB coûte donc ≈ 47 ms
contre ≈ 2,2 ms pour 1,18 depuis la flash (mesure T-81, sdbench.neo) : 20 fois plus lent.

Ce qu'une variante firmware (par exemple « 1,18 avec le numéro de volume et un fichier ») gagnerait seulement : un seul appel au lieu de deux, et le même
vocabulaire que les banques. Ce qu'elle ne peut pas donner : le mappage (pas de test à l'accès, pour des raisons de temps de bus), les pages blitter `$A0+n`
(lecture directe de la flash, pas de fichier), ni une latence meilleure que celle de 3,6 + 3,8.

## 3. Le seul vrai intérêt : des banques **modifiables** sans flash

Les banques en flash ne se réécrivent pas à volonté (≈ 150 ms, écran noir, usure). Un fichier de la clé se réécrit avec 3,6 + 3,9 : pas d'usure de la flash du
RP2040, pas d'écran noir (à vérifier : écriture FatFs de 8 Ko). **Vitesse d'écriture de 8 Ko non mesurée** (T-138 : un Append de 64 o = 52 ms, ce qui n'est
pas une mesure de 8 Ko). Je ne sais pas si l'écriture d'une banque sur la clé est plus rapide ou plus lente que 150 ms.

## 4. Pistes (aucune appliquée)

| Piste | Firmware | Coût RAM statique | Avis |
|---|---|---|---|
| A. 6502 seul : fichier `BANKnn.BIN` lu par 3,6 + 3,8 (déjà possible) | aucun | 0 | **Recommandée** : zéro risque, 47 ms par banque |
| B. Mesurer l'écriture de 8 Ko (3,6 + 3,9) sur carte, pour savoir si un « cache modifiable » tient | aucun (programme de test) | 0 | À faire avant toute autre piste |
| C. Nouvelle fonction « charger une banque depuis un fichier » | quelques dizaines d'octets de code | marge statique 3 920 o (T-132) | Gain : 1 appel au lieu de 2 ; faible |
| D. Banque en SRAM (T-124) | relever `RAM_LIMIT` | ≈ 8 Ko | Refusé par bmarty le 2026-10-08 |

## 5. Non vérifié

Autres clés, autre taille de fichier, hub USB, écriture de 8 Ko, comportement si la clé est retirée pendant la lecture (3,8 renvoie une erreur ?).
