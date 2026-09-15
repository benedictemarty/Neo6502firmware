# Différences d'API par rapport à l'amont

Chaque fonction ajoutée est listée ici (groupe, numéro, paramètres, état amont)
et documentée dans le `groupN.inc` concerné.

| Groupe | Fonction | Paramètres | Retour | Amont | Depuis |
|---|---|---|---|---|---|
| 1 System | 12 Set Interrupt Tick | P0-1 = Hz (1..1000, 0 = arrêt) | erreur si > 1000 ; IRQB pulsée au tick, relâchée à la lecture de `$FFFF` | absente | F-60 |
| 1 System | 13 Get Interrupt Tick | — | P0-1 = Hz courant | absente | F-60 |
| 1 System | 14 Reboot Image | P0 = slot flash (0-3) | ne revient pas ; erreur si slot vide ou multi-boot absent | absente | F-81 |
| 1 System | 15 Get Image Name | P0 = slot, P1-2 = tampon préfixé | nom ASCII ; erreur si vide | absente | F-81 |
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

## Vecteurs du noyau 6502 ajoutés (F-61)

Table `jmp` étendue vers le bas (`$FFC1-$FFDC`, les 9 vecteurs amont `$FFDF-$FFF7`
sont inchangés) : `KSemSignal $FFC1`, `KSemWait $FFC4`, `KTaskTicks $FFC7`,
`KTaskUnlock $FFCA`, `KTaskLock $FFCD`, `KTaskExit $FFD0`, `KTaskSleep $FFD3`,
`KTaskYield $FFD6`, `KTaskCreate $FFD9`, `KTaskInit $FFDC`. Le vecteur IRQ `$FFFE`
pointe sur `KIrqHandler` (reset si l'ordonnanceur n'est pas actif, comme avant).
Détails : `F-61-rtos.md`.
