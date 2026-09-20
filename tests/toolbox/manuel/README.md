# Tests manuels (entrées clavier/souris)

`events.asm` (groupe 33) attend une souris en (200,80) bouton 1 puis les touches « ab » : l'émulateur `neo` de
Trinity n'injecte pas d'entrées (le fork avait `mouse:`/`keys:`), il se joue donc sur carte ou dans `neo` à la main.
Sortie attendue : `AVAIL 00 / NULL 00 0000 0000 / E 06 00 00 00 00C8 0050 / E 04 01 01 01 00C8 0050 /
E 01 61 04 00 00C8 0050 / E 02 61 04 00 00C8 0050 / E 01 62 05 00 00C8 0050 / E 02 62 05 00 00C8 0050`.
