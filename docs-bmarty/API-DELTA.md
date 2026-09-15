# Différences d'API par rapport à l'amont

Chaque fonction ajoutée est listée ici (groupe, numéro, paramètres, état amont)
et documentée dans le `groupN.inc` concerné.

| Groupe | Fonction | Paramètres | Retour | Amont | Depuis |
|---|---|---|---|---|---|
| 5 Graphics | 9 Set Graphics Mode | P0 = mode (0 : 320×240×256, 1 : Hercules 720×350 1 bpp, 2 : 320×256×16) | erreur si mode inconnu ou non supporté par l'hôte | absente | F-51 (ADR-02) |
| 5 Graphics | 10 Get Graphics Mode | — | P0 mode, P1-2 largeur, P3-4 hauteur, P5 bpp, P6 colonnes, P7 lignes | absente | F-51 (ADR-02) |

Comportements modifiés : en modes 1 et 2, les fonctions 5,7 (image), 5,8
(tilemap), 6,2 (sprite) renvoient une erreur ; 5,33 (lecture pixel) renvoie
l'index de couleur du mode (0-1 ou 0-15).
