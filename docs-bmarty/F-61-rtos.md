# F-61 — Ordonnanceur préemptif dans le noyau 6502

Date : 2026-09-15. État : **fait et vérifié dans `neo` et Phosphoneo** (démos
`rtos.neo6502` et `rtos_idle.neo6502`, identiques dans les deux) ; sur carte, dépend de
F-60 (tick IRQ) non testé.

## Pourquoi dans le noyau (question bmarty « c'est pas dans le kernel ? ou le firmware ? »)
La commutation de contexte sauve A/X/Y/S/P/PC : seul du code 65C02 peut le faire ;
le RP2040 fournit le tick (F-60). Le noyau 6502 (`kernel/kernel.asm`, 64tass) est
compilé dans le firmware (`kernel_binary.h`) et copié en `$FC00-$FFFF` (RAM) : il
héberge le code (`kernel/rtos.asm`, ~600 octets, 7 octets de marge avant `$FF00`), les
blocs de contrôle (`kernel/rtos_data.asm`, `$FF10-$FF6D`) et le gestionnaire d'IRQ.

## Contrat
- **4 tâches**, pile `$0100` découpée en 4 × 64 octets : tâche *n* = `$01FF - 64n` vers
  le bas (tâche 0 = l'appelant de `KTaskInit`, pile `$01C0-$01FF`).
- **Page zéro privée** `$E0-$EF` : copiée/restaurée à chaque commutation (16 octets).
  Page zéro noyau : `$FC-$FF`.
- **Tourniquet** sur le tick (100 Hz conseillé) ; `KTaskYield` cède volontairement.
- **API non réentrante** : encadrer les appels `$FF00` par `KTaskLock` / `KTaskUnlock`
  (le tick est compté, la commutation est différée). Un tick reçu pendant le verrou
  n'est pas perdu.
- **Sémaphores** : un octet n'importe où en mémoire (compteur) ; `KSemWait` bloque la
  tâche, l'ordonnanceur la réveille (et décrémente) quand il choisit une tâche.
- **Rien de prêt** : `WAI` puis lecture de `$FFFF` (acquitte IRQB, F-60) et compte du
  tick — la machine n'est jamais bloquée.
- **IRQ/BRK sans ordonnanceur actif** : `$FFFE` pointe sur `KIrqHandler`, qui saute au
  reset comme avant (`bit rtActive`) — comportement amont conservé.
- **Ordre** : `KTaskInit` (remet la table à zéro et démarre le tick) **puis**
  `KTaskCreate`.

## Vecteurs (table `$FFC1`, `kernel/neo6502.inc` régénéré)
| Vecteur | Adresse | Entrée | Sortie |
|---|---|---|---|
| `KTaskInit` | `$FFDC` | A/X = Hz lo/hi | interruptions activées |
| `KTaskCreate` | `$FFD9` | A/X = adresse de la tâche | C=0 et A = id ; C=1 si plus de place |
| `KTaskYield` | `$FFD6` | — | — |
| `KTaskSleep` | `$FFD3` | A = ticks (1-255) | — |
| `KTaskExit` | `$FFD0` | — | ne revient pas |
| `KTaskLock` / `KTaskUnlock` | `$FFCD` / `$FFCA` | — | — (imbricables) |
| `KTaskTicks` | `$FFC7` | — | A/X = compteur lo/hi |
| `KSemWait` / `KSemSignal` | `$FFC4` / `$FFC1` | A/X = adresse du sémaphore | — |

## Émulateur `neo`
`WAI` (`$CB`) ajouté au cœur (attente d'une IRQ en attente) ; la lecture de `$FFFF`
relâche l'IRQ en attente, comme la carte (F-60).

## Limites v1
- Pas de priorités, pas de mutex nommés, pile de 64 octets par tâche (les
  `KSendMessage` du noyau en consomment ~6) ; un débordement de pile n'est pas détecté.
- Une tâche qui utilise plus de page zéro que `$E0-$EF` doit se coordonner avec les
  autres (verrou) ; BASIC n'est pas compatible (page zéro et pile).
