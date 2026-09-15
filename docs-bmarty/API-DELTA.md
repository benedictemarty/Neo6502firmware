# Différences d'API par rapport à l'amont

Chaque fonction ajoutée est listée ici (groupe, numéro, paramètres, état amont)
et documentée dans le `groupN.inc` concerné.

| Groupe | Fonction | Paramètres | Retour | Amont | Depuis |
|---|---|---|---|---|---|
| 5 Graphics | 9 Set Graphics Mode | P0 = mode (0 : 320×240×256, 1 : Hercules 720×350 1 bpp, 2 : 320×256×16) | erreur si mode inconnu ou non supporté par l'hôte | absente | F-51 (ADR-02) |
| 5 Graphics | 10 Get Graphics Mode | — | P0 mode, P1-2 largeur, P3-4 hauteur, P5 bpp, P6 colonnes, P7 lignes | absente | F-51 (ADR-02) |
| 5 Graphics | 11 Set Draw Page | P0 = page (0-1 ; mode 0 : 0 seulement) | erreur si la page n'existe pas | absente | F-55 |
| 5 Graphics | 12 Set Display Page | P0 = page | erreur si la page n'existe pas ; effective à la trame suivante (attendre avec 5,37) | absente | F-55 |

**Attributs MDA (mode 1, Hercules)** : les quartets encre/papier de la console
(2,15 Set Text Color, codes `$80+encre`, `$90+papier`) portent les attributs :
encre bit 0 = allumée, bit 1 = souligné (ligne 12 de 14), bit 2 = gras (double
frappe), bit 3 = clignotant (0,5 s, phase du timer 100 Hz, repeint par
`CONBlinkSync` depuis `DSPSync`) ; papier bit 0 = allumé → vidéo inverse quand
l'encre est éteinte (`$80,$91`). Encre par défaut en mode 1 : 1.

Comportements modifiés : en modes 1 et 2 les sprites (groupe 6, tortue groupe 9)
sont **dessinés en XOR dans le tampon** (pas de couche séparée) : couleurs exactes
sur fond noir, mélangées (XOR) sur fond coloré, inversion en monochrome ; un
texte ou une primitive dessinés par-dessus effacent le sprite (comme sur un
micro 8 bits sans matériel de sprite) et 2,12 les efface aussi ; 5,7 (image) et
5,8 (tilemap) dessinent avec les index réduits du mode ; 5,33 (lecture pixel) renvoie l'index de couleur du mode
(0-1 ou 0-15) de la page de dessin ; 2,12 (effacement) n'efface que la page de
dessin.
