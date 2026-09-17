# Différences d'API par rapport à l'amont

Chaque fonction ajoutée est listée ici (groupe, numéro, paramètres, état amont)
et documentée dans le `groupN.inc` concerné.

| Groupe | Fonction | Paramètres | Retour | Amont | Depuis |
|---|---|---|---|---|---|
| 1 System | 12 Set Interrupt Tick | P0-1 = Hz (1..1000, 0 = arrêt) | erreur si > 1000 ; IRQB pulsée au tick, relâchée à la lecture de `$FFFF` | absente | F-60 |
| 1 System | 13 Get Interrupt Tick | — | P0-1 = Hz courant | absente | F-60 |
| 1 System | 14 Reboot Image | P0 = slot flash (0-3) | ne revient pas ; erreur si slot vide ou multi-boot absent | absente | F-81 |
| 1 System | 15 Get Image Name | P0 = slot, P1-2 = tampon préfixé | nom ASCII ; erreur si vide | absente | F-81 |
| 10 UExt, fn 13-19 | 13-18 routées vers un modem USB CDC selon 10,19 ; **19 Route UART to CDC** (P0 : 0 matériel, 1 CDC, 2 AUTO — défaut) | AUTO = CDC si un modem est branché | fn 19 nouvelle ; 13-18 étendues | routage absent | F-93 |
| 14 USB Serial (CDC) | 1 Status, 2 Read Byte, 3 Write Byte, 4 Read Block, 5 Write Block, 6 Set Line Coding | P7 = périphérique (0/1) ; voir `F-90-cdc.md` | nouveau groupe | absent | F-90 |
| 2 Console | 20 Console Debug Echo | P0 = 0 arrêt, 1 texte imprimable + CR/LF, 2 aussi $C0-$FF | — | absente | F-92 |
| 3 File I/O | 24 Volume Info | P0 = volume (0-3), P1-2 = tampon préfixé | nom (`USBn`/`SDn` carte, `HOSTn` émulateurs), P3 attributs (bit 0 présent, 1 lecture seule, 2 réseau) ; erreur Invalid Drive ($13) si absent | absente | F-102 |
| 3 File I/O | 25 Select Volume | P0 = volume | erreur si absent ; chaque volume garde son répertoire courant | absente | F-102 |
| 3 File I/O | 26 Get Current Volume | — | P0 = volume courant | absente | F-102 |
| 3 File I/O | 27 File Read Paged | P0 = canal, P1 = page (`$00` RAM 6502, `$80`/`$81` VRAM, `$90` RAM graphique — pages du blitter 12,2), P2-3 = adresse dans la page, P4-5 = taille | P4-5 = octets lus ; position du fichier avancée comme 3,8 ; erreur Invalid Parameter ($15) si la plage dépasse la page (pas de retenue, pas de passage `$80`→`$81` : deux appels) ; EOF ($02) si rien lu | absente (3,8 avec `$FFFF` ne vise que la RAM graphique à l'offset 0) | F-16 |
| 3 File I/O (tous chemins) | préfixe `n:` (`1:/jeux/tetris.neo`) = volume n ; sans préfixe = volume courant | — | déjà compris par FatFs sur la carte (clés USB montées à leur adresse USB, SD en `0:`) ; émulateurs : `<storage>` = 0, `<storage>1..3` = 1..3 | extension | F-102 |
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
