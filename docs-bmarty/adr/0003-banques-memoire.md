# ADR-03 — Banques mémoire 6502 par copie (F-23)

Statut : **proposée** (2026-09-17), implémentée sur `feat/memory-banks`, à ratifier
par bmarty ; validation carte en attente (R22).

## Contexte
Demande bmarty du 2026-09-17 : « prévois de la banque dans le firmware ». Besoin
concret : Neo6502civ manque de 8 Ko (`$A000` occupé par le code plateforme) ;
plus généralement, garder du code ou des données hors des 64 Ko sans repasser
par la SD.

Faits vérifiés :
- `processor_pio.cpp` : chaque lecture du 65C02 est servie par `cpuMemory[a]`
  sur core0 ; la marge est de 11 `nop` par lecture (14 avant F-60). Un test
  d'adresse ou une indirection par banque à chaque lecture coûterait des cycles
  sur ce chemin, non mesurables sans carte (règle 4).
- Pendant un appel API le 65C02 attend `$FF00` (RDY tenu par le PIO) : les
  fonctions 3,2 / 3,8 écrivent déjà `cpuMemory` pendant l'appel sans que le
  65C02 le voie. Une copie de banque pendant l'appel est donc du même ordre.
- SRAM RP2040 : le firmware USB occupait 222 Ko de `.bss` ; avec **4 banques
  de 8 Ko** l'éditeur de liens déborde de 10 528 octets ; avec **2 banques**
  il reste 5 856 octets de tas (`__end__` = `$2003E920`, `__StackLimit` =
  `$20040000`), 7 100 en SDCARD.

## Décision
1. **Commutation par copie**, pas par indirection : `BNKSelect(bank, addr)`
   recopie la banque courante de la fenêtre vers son stockage (write-back) puis
   la banque demandée dans la fenêtre. Aucun coût sur la boucle bus.
2. **Fenêtre de 8 Ko** (`BANK_SIZE`), adresse alignée sur une page, fenêtre
   terminée avant `$FF00`. L'adresse est choisie par le programme à chaque
   sélection (la même banque peut être ramenée ailleurs).
3. **2 banques** (`BANK_COUNT`) identiques sur carte, `neo` et Phosphoneo (les
   tests golden lisent le nombre par 1,19). Stockage `bankStorage[][]` dans
   `banks.cpp`, mis à zéro au démarrage ; `BNKReset()` au DSP Reset (1,0)
   oublie la table sans write-back.
4. **API** groupe 1 : `1,18 Select Bank` (P0 banque, `$FF` = démonter ;
   P1-2 adresse), `1,19 Get Bank Info` (banque courante, adresse, nombre,
   taille). Erreur générique (1) si banque ou adresse invalide.
5. **Contrat côté programme** : la fenêtre ne doit contenir ni le code qui
   appelle, ni la pile, ni la page zéro ; le contenu de la fenêtre est
   complet au retour de l'appel (pas d'attente).

## Alternatives écartées
- **Indirection dans la boucle bus** (`bankPtr[a>>13][a&0x1FFF]`) : commutation
  instantanée, mais surcoût sur chaque lecture ; à reconsidérer seulement si
  une mesure sur carte montre que la marge le permet.
- **Banques en flash (F-21)** : lecture seule, même mécanisme de copie avec
  source en XIP ; non fait — extension naturelle (numéros ≥ 128) quand le
  besoin ROM sera exprimé.
- **4 banques ou plus** : refusé par l'éditeur de liens (voir Contexte) ;
  rouvrir si F-52/F-53 libèrent de la SRAM ou si les modes 1/2 sont abandonnés.
- **8 Ko vs 16 Ko** : 8 Ko donne 2 banques dans la SRAM restante et couvre
  le besoin civ ; 16 Ko n'en donnerait qu'une.

## Conséquences
- Coût d'une sélection : 2 × 8 Ko de `memcpy` sur core0 pendant l'appel, à
  **mesurer sur carte** (attendu : dizaines de µs) — R22 avec le tas résiduel
  (5,8 Ko USB : surveiller `malloc` de TinyUSB / `std::string`).
- Le contenu des banques n'est pas sauvegardé au reset ni sur SD.
- Test `docs-bmarty/tests/banks.asm` : identique dans `neo`, Phosphoneo et
  en co-simulation du vrai firmware ARM.
