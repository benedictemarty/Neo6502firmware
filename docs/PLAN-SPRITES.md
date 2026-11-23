# Plan de mise à niveau des sprites (T-157 à T-162)

Décision bmarty 2026-11-23 : « go plan de mise à niveau des sprites ». Ce document est **un plan, pas un engagement** :
chaque ticket attend son propre go de bmarty, et une fonction n'entre dans Trinity que validée **sur carte**.
Rien n'est codé. Les faits viennent du code (0.16.110), de `SPRITES-MODE0.md` et de la mesure `perfspr.asm`
(2026-11-23) ; les idées et les inconnues sont marquées comme telles.

## 1. Constat (lu ou mesuré)

- Les sprites du mode 0 sont **logiciels** : le cœur 0 écrit dans le quartet haut des octets de `graphicsMemory`
  (`sprites.cpp`, `sprites_xor.cpp`). Pas de couche séparée.
- Manques par rapport à l'état de l'art : **zoom** (5,66 ignoré par 6,2 et 6,7), **priorité fond/sprite** (le fond est
  toujours derrière), **collisions** par distance des centres seulement (`SPRCollisionCheck`), **un appel API par sprite**.
- **Le coût CPU n'est pas le problème** : 6 sprites 24 × 32 déplacés pendant 100 trames prennent 16 à 46 cs (XOR/opaque)
  et 138 cs au pire (octet entier, sprites qui se touchent), pour un budget de 200 cs à 50 Hz (`BACKLOG.md` T-157).
- Contraintes : `SPRITE_INTERNAL` figé à 24 octets (T-113, `static_assert`) ; marge de RAM statique **280 o** (`RAM_LIMIT`
  234 400, `CARTE-RAM.md`) ; tas ≈ 28 Ko ; le cœur 1 sert l'affichage en IRQ (`_scanline_callback`, jamais en flash) ;
  l'API du groupe 6 existante ne doit pas changer (6,1 à 6,10).

## 2. Principe

Les manques à lever sont la priorité fond/sprite, le zoom, les collisions et le groupe de déplacements. Aucun ne demande
une couche composée à l'affichage : on les traite d'abord **dans le cœur 0**, sans toucher au cœur 1, avec des
fonctions **nouvelles** du groupe 6 (numéros libres à vérifier dans `dispatch_code.h`). La couche composée (T-157)
reste une option, décidée seulement sur mesure du cœur 1.

## 3. Tickets proposés (ordre de réalisation)

| ID | P | Contenu | Pourquoi / inconnues |
|---|---|---|---|
| T-158 | P2 | **Mesurer le cœur 1** : temps libre par ligne, `RNDLateScanlines` (graphics.h:72) et `lateTotal` pendant `perfspr`, tas libre au pic. Sortie : un tableau dans `CARTE-RAM.md`. | Inconnue qui décide de T-162. Aucun code de fonction. |
| T-159 | P2 | **Décor devant les sprites** (idée, mode 2 octet entier) : une fonction 6,x donne un **seuil de premier plan** ; un pixel de fond de valeur ≥ seuil n'est pas écrasé par un sprite. | Résout « un arbre devant le joueur » sans couche. Idée **non vérifiée dans le code** : à étudier avec la sauvegarde/restauration du fond (`_SPRByteSaveDraw`) et le masque `$F0` du groupe 5. Modes 0/1 : non couverts (le fond y est dans 4 bits). |
| T-160 | P3 | **Zoom entier des sprites** (×2, ×3, par sprite) : bits libres de l'octet de flip de 6,2 ou nouvelle fonction ; dessin pixel par pixel. | Seulement **entier** ; le ×1,5 demandé par Spellbinder n'est pas couvert (image pré-agrandie). À vérifier : bits libres de `flip`, effet sur les rectangles de sauvegarde (256 o par 16 × 16 devient plus). Coût à mesurer sur carte. |
| T-161 | P3 | **Collisions par rectangle** (et pas seulement distance des centres) + **déplacement de plusieurs sprites en un appel** (liste en page `$90`). | Le second point avait été noté « pas indispensable » par NeoWoW (T-146) ; Spellbinder déplace ~6 sprites par trame : ne le faire que sur demande confirmée. |
| T-162 | P4 | **Couche composée à l'affichage** (= T-157). Décision **seulement si T-158 montre une marge** du cœur 1 et si T-159 à T-161 ne suffisent pas. Prototype sur l'émulateur avant la carte. | RAM (marge 280 o), IRQ du cœur 1, compatibilité des modes empaquetés. |

## 4. Règles de chaque ticket (Definition of Done)

1. Test API `tests/api/NOM.asm` + `.expected`, avec **contre-essai** (le test échoue sans la modification), comme `sprbyte`.
2. Essai **sur carte** par RP3Sonde (flasher, dépôt sur la clé, lecture de l'écran) ; résultats chiffrés au backlog.
3. `make test-coeur1` et les suites existantes (sprites : `sprmode0`, `sprmode1`, `sprimg`, `sprbyte`) restent OK.
4. RAM statique relevée (`CARTE-RAM.md`) ; `RAM_LIMIT` non dépassée.
5. `SPRITES-MODE0.md`, `docs/BACKLOG.md`, `README-TRINITY.md` (version), CHANGELOG local ; tag `trinity-vX.Y.Z` à la livraison.
6. Compatibilité : les appels 6,1 à 6,10 se comportent comme avant ; 6,1 remet à zéro les nouveaux réglages.

## 5. Ce qui n'est pas dans le plan

Sprites matériels au sens des puces (la carte n'en a pas), zoom fractionnaire, priorité par sprite pour chaque plan,
modes empaquetés (1 et 4 bits par pixel : restent en XOR, § 2 de `SPRITES-MODE0.md`).
