# Revue du backlog et recette carte — 2026-10-01

Base : Trinity **0.16.67** (`f2556cc`). **Décision bmarty 2026-10-01 : « oui »** — les 23 tickets du § 1 sont clos dans
`docs/BACKLOG.md` (état « Clos 2026-10-01 », ancien état conservé à la suite). Chaque proposition cite sa preuve ; « par l'usage » veut dire que la fonction a servi sur la carte
depuis, sans défaut signalé, mais sans test dédié.

## 1. Proposés à clore

### Validés sur carte par un test ou une mesure
| Ticket | Preuve |
|---|---|
| T-91 Date des fichiers (3,29) | test `fdate` passé sur carte en 0.16.62 (20/20, T-98) ; échec du 2026-09-30 expliqué (pas d'heure après coupure secteur) |
| T-92 Pixel 0 transparent des tilemaps | test `tilezero` passé sur carte en 0.16.62 (T-98) |
| T-36 Contrats `1,0` / `1,3` | test `softreset` passé sur carte en 0.16.62 (T-98) |
| T-17 Banques en XIP | tests `banks`, `bankcsum`, `blitbank` passés sur carte en 0.16.62 (T-98) ; 1,26 mesuré en 0.16.64 (T-81) |
| T-14 Multitâche 6502 | `frameirq` passé sur carte en 0.16.64 ; démo `rtos` lancée par `auto.txt`, `T=0064` relu (T-96) |
| T-49 / T-50 Instrumentation du bus | 5,41 / 5,42 utilisés pour les mesures du 2026-09-30 (T-90, T-81) |
| T-57 `5,40` cumule | 74 → 0 mesuré avec ce compteur (T-101) |
| T-44 Image du curseur hors flash | `make test-coeur1` (T-103) : aucune fonction ni donnée en flash sur le cœur 1 |
| T-73 / T-74 Port de debug | parle depuis 0.16.37 (T-75) ; console série utilisée par la sonde |
| T-68 Échap au démarrage | couvert par T-68b, validé carte 0.16.36 |
| T-39 Touches multimédia | codes relevés sur carte par SWD (T-86), 0.16.50 « tout ok » |

### Expliqués ou remplacés
| Ticket | Raison |
|---|---|
| T-48 / T-48b (P0) Panne selon le placement en flash | T-54 l'explique (débordement des tableaux MSC : ce qu'il écrase dépend du binaire) ; non reproduit depuis, sur des dizaines de builds (NeoLegacy, BattleNeo, POP, tests carte 18-20/20) |
| T-38 / T-38b / T-58b Traits rouges | cause trouvée et mesurée par T-101 (encodeur TMDS en flash : 74 → 0) ; T-102 et T-103 complètent |
| T-53 Quatre tampons de ligne | partie de la même série ; mesure T-101 : 0 ligne en retard |
| T-84 NeoDOS 0.24.1 embarqué | remplacé : NeoDOS 0.32.1 embarqué et utilisé tous les jours |
| T-13 Budget mémoire (0.3.1) | firmware en service depuis ; `RAM_LIMIT` contrôlé à chaque build |
| T-32 Boot déterministe | des dizaines de démarrages sur carte les 2026-09-30/10-01 (par l'usage) |
| T-78 Balayage UEXT | retiré en 0.16.38 ; démarrages normaux depuis (par l'usage) |

## 2. Recette carte (prochaine séance)

Ordre pensé pour flasher une fois et réutiliser la clé. Outils déjà sur la clé, sauf mention.

| # | Ticket | Geste | Attendu | Qui |
|---|---|---|---|---|
| 1 | — | flash 0.16.67 par SWD, coupure secteur | bannière v0.16.67, `lateTotal` = 0 au démarrage | sonde |
| 2 | T-102 | `MODE 1` dans NeoDOS | bordures noires ; `RENDU.NEO` : 0 ligne en retard en mode 1 | œil + sonde |
| 3 | T-100 | `ARRACHE`, clé arrachée en pleine lecture | erreur rendue au 6502, retour à NeoDOS, aucune panique, affichage vivant | bmarty + sonde |
| 4 | T-101 | usage normal 10 min, Tab et `DIR` répétés | aucun trait ; `lateTotal` relu à la fin | bmarty + sonde |
| 5 | T-67 / T-69 | `MODE 1` : police MDA 9×14 | glyphes du MDA, pas du VGA | œil |
| 6 | T-20 | clavier AZERTY : `é è ç à ù ² ° £ § µ`, touches mortes | caractères justes | bmarty |
| 7 | T-41 | Verr Num, Verr Maj | voyants et effet | bmarty |
| 8 | T-29 / T-42 / T-43 | souris en mode 0 et 1, bords de l'écran | curseur suivi, rogné aux bords, sans éclat | bmarty (souris USB) |
| 9 | T-28 | Échap pendant le démarrage avec un `boot/` | menu ouvert | bmarty |
| 10 | T-18 | volumes (deux clés) | `A:` / `B:` vus | bmarty (2ᵉ clé) |
| 11 | T-105 | `COPIE.NEO` en 0.16.68 puis 0.16.69 | durée de 12,2 (5,42 P1) plus courte sans `printf` | sonde |
| 12 | T-104 | branche `t104-fatfs-tiny` contre 0.16.69 : `ARRACHE`, `COPY`, `RENDU`, `neotests` | débits comparables | sonde + bmarty |
| 13 | T-106 | branche `diag-coeur1` : démarrage, 10 min d'usage, `RENDU` | `diagcoeur1.py` : phases et encodage max | sonde + bmarty |

## 3. Laissés ouverts (fonctions, pas des validations)

T-08, T-09, T-10, T-11, T-12 (toolbox en cours), T-33 (watchdog de boot), T-35 (politique de démarrage),
T-70 (lecteurs de disquette USB), T-77 (boîte noire PicoDVI, gardée comme instrument), T-87 (casque USB, matériel),
T-90 (mode 16 couleurs, étude).

## 4. Séance carte du 2026-10-02 (sonde SWD, bmarty au secteur)

Faits : étape 1 (0.16.69 : 0 épisode au démarrage), 2 (T-102, mesure SWD ; écran non regardé), 3 (T-100 validé), 11 (T-105 mesuré : 2 à 4× plus rapide), 12 (T-104 mesuré, livrable), 13 (T-106 : épisodes seulement aux changements de mode, en phase API). Restent : 4 (10 min d'usage normal à l'œil), 5-10 (vérifications à l'œil et au matériel : police MDA, AZERTY, Verr Num/Maj, souris, Échap au démarrage, deux clés). Carte laissée en 0.16.69, clé en place.
