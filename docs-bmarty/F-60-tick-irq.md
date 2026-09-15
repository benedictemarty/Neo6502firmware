# F-60 — Tick d'interruption vers le 65C02

Date : 2026-09-15. État : **code écrit, vérifié dans `neo` et Phosphoneo, non exécuté
sur carte** (aucun Neo6502 disponible).

## API (groupe 1)
- **1,12 Set Interrupt Tick** : P0-1 = cadence en Hz (1..1000), 0 = arrêt. Erreur si
  > 1000. `1,0` (DSP Reset) arrête le tick.
- **1,13 Get Interrupt Tick** : P0-1 = cadence courante.

## Contrat côté 65C02
- Le vecteur `$FFFE/$FFFF` est en RAM (le noyau y met `start` : une IRQ sans
  gestionnaire **réinitialise la machine**) : installer son gestionnaire, puis `CLI`.
- **Pas d'acquittement** : IRQB est relâchée par le firmware quand le 65C02 lit `$FFFF`
  pendant la séquence d'interruption (vue par la boucle bus de core0). Le gestionnaire
  ne peut donc pas être ré-entré par le même tick. Avec I=1, la ligne reste basse
  jusqu'au `CLI` : les ticks se cumulent en une seule interruption (aucun n'est perdu,
  mais ils ne sont pas comptés).
- Un gestionnaire peut appeler l'API (le gel du 65C02 pendant `DSPHandler` empêche
  toute interruption au milieu d'un appel) mais doit sauver/restaurer `$FF01-$FF0B`
  si le programme principal peut être en train de préparer un appel.

## Implémentation
| Hôte | Tick | Relâchement |
|---|---|---|
| Carte (`tick.cpp`, `processor_pio.cpp`) | `add_repeating_timer_us` sur core0 (le bus est gelé par le PIO pendant le handler, comme pour `DSPHandler`) → `wdc65C02cpu_set_irq(true)` | test `address == $FFFF && irqAsserted` dans le chemin lecture de la boucle bus |
| `neo` (`sys_processor.cpp`, `6502.cpp`) | compteur de cycles (`CYCLE_RATE / hz`) | `CPUTriggerIRQ()` : pris si I=0, sinon reste en attente |
| Phosphoneo (`main.c`) | compteur de cycles | `cpu_set_irq(false)` dans `bus_read` sur `$FFFF` |

## Risque carte
| # | Risque | Vérification |
|---|---|---|
| R9 | Le test `$FFFF` ajoute ~3 cycles ARM au chemin lecture de la boucle bus ; 3 `nop` de calibration ont été retirés (14 → 11) pour compenser **sans mesure** | oscilloscope : cycle bus identique avant/après ; sinon rétablir les `nop` et réduire ailleurs |
| R10 | Latence du timer matériel (IRQ ARM pendant la boucle bus) : le 65C02 est gelé quelques µs par tick | mesurer la gigue à 1000 Hz |

## Démo
`Phosphoneo/tests/corpus/irqtick.neo6502` : 100 Hz, compteur affiché chaque seconde
(`TICKS 0064`, `00C8`, …), identique `neo` ↔ Phosphoneo.
