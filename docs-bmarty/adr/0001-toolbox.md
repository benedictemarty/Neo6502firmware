# ADR-01 — Toolbox Neo6502 : primitives « à la Apple IIgs » dans le firmware

Statut : **proposée** (2026-09-15), à ratifier par bmarty.

## Contexte
L'API du firmware (13 groupes, mécanisme `$FF00` : groupe + fonction + 8 octets
de paramètres, appel synchrone, 6502 gelé pendant le traitement) fournit le
matériel et des services de base (console, primitives graphiques, sprites,
fichiers, maths, son, périphériques). Il n'existe ni fenêtres, ni menus, ni
événements unifiés, ni fontes proportionnelles, ni contrôles, ni ressources.
Le Télémon (Neo6502kbd EPIC-01) et les portages auraient besoin d'une couche
de ce type ; le IIgs l'a résolue par la Toolbox (`_ToolCall` : outil + fonction
+ paramètres sur la pile), très proche du mécanisme groupe + fonction du Neo.

## Décision proposée
Ajouter une **famille de groupes « Toolbox »** au firmware (fork, proposable en
amont), implémentée en C++ dans `firmware/common/` (donc disponible sur la
carte, dans `neo` et dans Phosphoneo), avec le 6502 responsable uniquement du
code applicatif (rappels, boucles d'événements).

### Numérotation (groupes 32 et suivants, laissant 14–31 à l'amont)
| Groupe | Outil | Contenu v1 |
|---|---|---|
| 32 | **QuickDraw** | `Rect`/`Point` en RAM 6502 (pointeurs), port de dessin courant avec clipping rectangulaire, `FrameRect/PaintRect/EraseRect/InvertRect`, `MoveTo/LineTo`, motifs 8×8, fontes proportionnelles (format à définir, chargées depuis SD ou intégrées), `DrawString` avec style, `TextWidth`, `CopyBits` (via le blitter), régions rectangulaires simples (union/intersection) |
| 33 | **Event Manager** | file d'événements unifiée : clavier (touche, modificateurs, répétition), souris (mouvement, boutons, clic/double-clic), fenêtre (activation, mise à jour), timer ; `GetNextEvent` non bloquant, `WaitNextEvent` avec timeout ; couplage futur avec l'IRQ F-10 (réveil du 6502) |
| 34 | **Window Manager** | fenêtres rectangulaires avec titre, cadre, zone de contenu, ordre en Z, `NewWindow/DisposeWindow/ShowHide/SelectWindow/DragWindow/SizeWindow`, invalidation et événements `update` (le 6502 redessine le contenu via QuickDraw, le firmware dessine les cadres), `FindWindow` |
| 35 | **Menu Manager** | barre de menus, menus déroulants, items (coché, grisé, raccourci), `MenuSelect` retourne (menu, item) |
| 36 | **Control Manager** | boutons, cases à cocher, boutons radio, ascenseurs, champs de texte simples ; `TrackControl` |
| 37 | **Dialog Manager** | boîtes modales et alertes construites depuis une description en RAM 6502 |
| 38 | **Resource/Font Manager** | ressources nommées sur le stockage (fontes, icônes, menus, dialogues), poignées côté RP2040 |
| 39 | **Memory (RP2040)** | poignées de blocs dans la SRAM libre du RP2040 (≈ 47 Ko) ou en flash : `NewHandle`, `Read/Write` vers la RAM 6502 par blocs — stockage hors des 64 Ko |

### Conventions
- Paramètres : identifiants entiers 16 bits (fenêtre, menu, contrôle, poignée),
  structures (`Rect` = 4 × int16, `Point`, `EventRecord` = 8 octets, descriptions
  de menus/dialogues) en RAM 6502 passées par **adresse 16 bits** dans les
  paramètres ; résultats dans les paramètres ; erreurs dans `$FF02` (codes par
  groupe, documentés).
- Pas de rappel firmware → 6502 : les `DefProc`/rappels sont remplacés par des
  **événements** que le 6502 traite ; le firmware dessine ce qui lui appartient
  (cadres, menus, contrôles), le 6502 le contenu des fenêtres.
- Chaque fonction est documentée dans son `groupN.inc` (`DOCUMENTATION`), donc
  dans `api.tex` ; tests dans Phosphoneo (captures golden) avant la carte.
- Ordre de réalisation : 32 QuickDraw → 33 Event → 34 Window → 35/36 Menu &
  Control → 37 Dialog → 38/39 ; le Télémon est l'application-vitrine.

## Options écartées
- Tout en 6502 (bibliothèque neolib) : trop lent (6,25 MHz, 64 Ko) pour le
  dessin et les fontes ; garder le 6502 pour la logique applicative.
- Réutiliser le format exact des enregistrements IIgs (pointeurs 24 bits, ID de
  poignées) : sans objet en 64 Ko ; on garde l'esprit, pas le format.

## Conséquences
- Firmware plus gros (flash : marge large ; SRAM : à mesurer, les structures
  de fenêtres/menus restent petites) ; latence des appels à mesurer en co-sim.
- Divergence amont tant que non fusionné : numérotation ≥ 32 pour éviter les
  collisions avec de futurs groupes officiels.
- Dépendances : F-10 (IRQ) pour un `WaitNextEvent` efficace ; F-11 (blit 2 bpp)
  et F-12 (son) indépendants.
