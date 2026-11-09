# Étude : un système de base de données (create, seek, search, update) sur la clé USB — T-138

Demande de bmarty (2026-11-09) : « un système de bd avec seek, search, update, create directement sur usb », étude d'abord.
Étude en lecture seule. Aucun code n'a été modifié. Rien de ce document n'a été essayé sur carte.

## 1. Ce qui existe déjà (vérifié dans les sources)

- Groupe 3 (fichiers) : 4 Open (modes 0 lecture, 1 écriture, 2 lecture/écriture, 3 création), 6 Seek (32 bits), 7 Tell,
  8 Read, 9 Write, 10 Size, 11 Set Size, 13 Delete, 22 EOF, 27 Read Paged (lecture directe vers une page mémoire du
  blitter, sans passer par la RAM 6502).
- `FIO_NUM_FILES` = 8 canaux ouverts en même temps, partagés par tout le système (`filesystem.h:92`).
- Groupe 38 (ressources NR1) : table de ressources, « rien n'est mis en cache dans le RP2040 », lecture à la demande.
  C'est le précédent à suivre : tampon fixe, accès par seek.
- Groupes toolbox 32 à 38 et 39 (AGI Pictures, T-107) : le prochain numéro libre est le **40** (vérifié dans
  `dispatch.config` ; correction : l'étude disait 39).
- Il n'existe aucune notion d'enregistrement, de recherche ni de verrou dans le firmware.

## 2. Contraintes

- RAM statique : marge de **3 920 o** seulement (étude T-132, 0.16.86). Un nouveau groupe ne peut pas se permettre de
  gros tampons statiques. Tampon fixe de 256 o au plus, ou rien (lecture directe vers une page mémoire avec 3,27).
- Une recherche qui boucle dans le firmware bloque l'API pendant toute sa durée. Le temps par enregistrement lu sur
  clé USB n'est **pas mesuré** : je ne le connais pas. À mesurer avant de choisir entre « recherche dans le firmware »
  et « recherche côté 6502 ».
- Pas de verrou : si une autre tâche (NeoWin) ouvre la même base, le comportement est indéfini (la doc de 3,4 le dit).
- Écriture : une coupure d'alimentation ou un retrait de clé pendant un update peut laisser un enregistrement à
  moitié écrit.

## 3. Trois niveaux possibles

| # | Niveau | Apport | Coût | Risque |
|---|---|---|---|---|
| A | Bibliothèque 6502 (BASIC, assembleur) au-dessus du groupe 3 | Aucun changement de firmware, aucune RAM prise | Chaque appel traverse l'API : plus lent ; code à écrire par langage | Faible |
| B | Groupe toolbox 40 dans Trinity | Appel unique create/open/seek/search/update/delete, recherche dans le firmware, utilisable par tout programme | Nouvelle version, tests, une partie de la marge RAM | Moyen : marge RAM, temps de blocage |
| C | Format de fichier + outil PC (Python) | Bases préparables sur PC, vérifiables sans carte | Aucun usage sur carte tant que A ou B n'existe pas | Faible |

Recommandation : **C puis A**, mesurer, et ne faire B que si les mesures montrent que A est trop lent. C'est le plus
petit pas qui apporte une preuve, et il respecte la règle du projet (validé sur carte avant d'entrer dans Trinity).
C'est une recommandation de ma part, la décision est à bmarty.

## 4. Format proposé (valable pour A, B et C)

Un fichier unique `nom.db` sur la clé :

- En-tête de 32 octets : signature `ND`, version 1, taille d'enregistrement (1 à 255), nombre d'enregistrements,
  nombre de champs, puis une table de champs (nom court, type, offset, taille). L'en-tête réel doit tenir dans une
  taille fixe à définir au moment de l'implémentation.
- Enregistrements à **longueur fixe**, un octet d'état en tête (0 libre, 1 actif).
- Types de champs : octet, entier 16 bits, entier 32 bits, chaîne à longueur fixe.

Conséquences :
- **seek** par numéro N : un seul `File Seek` à `entête + N × taille`, en O(1).
- **create** : écrire à la fin (ou dans un emplacement libre), puis mettre à jour le compteur dans l'en-tête.
- **update** : seek puis écriture de l'enregistrement entier, en place.
- **delete** : mettre l'octet d'état à 0 (suppression logique, l'emplacement est réutilisable).
- **search** : parcours séquentiel, comparaison d'un champ (égalité, préfixe) ; coût linéaire, d'où la nécessité de
  mesurer. Un index trié est écarté pour l'instant : il impose de réécrire l'index à chaque insertion sur la clé USB.

Écarté : base à clé/valeur avec index ; enregistrements à longueur variable (le seek par numéro devient impossible
sans table d'offsets).

## 5. Risques et questions ouvertes

1. Temps de lecture réel par enregistrement sur clé USB : à mesurer sur carte.
2. Intégrité en cas de coupure : à définir (écrire d'abord l'enregistrement, puis le compteur ; un octet d'état
   `en cours` serait possible).
3. Accès concurrent (NeoWin et un programme) : pas de verrou dans le firmware, à interdire par convention ou à ajouter.
4. Nombre de canaux : une base ouverte consomme 1 des 8 canaux.
5. Taille maximale : la limite de fichier est celle de FAT ; non vérifiée si exFAT est actif (`FF_FS_EXFAT` non
   consulté).
6. Où sera utilisée la base (BASIC, NeoDOS, NeoWin, un jeu) : je ne le sais pas, et cela change le choix A ou B.

## 6. Décisions attendues de bmarty

- Niveau retenu : C puis A, ou B directement ?
- Usage visé (quel programme, combien d'enregistrements, quelle taille) : cela fixe la taille d'enregistrement et
  l'importance de la vitesse de search.
- Les types de champs et le format ci-dessus conviennent-ils ?

## 7. Décision et usage précisés par bmarty (2026-11-09)

- Niveau retenu : **B**, un groupe toolbox 40 dans Trinity.
- Projets visés : Neo6502Tyrann, Neo6502AigleDorEngine, et d'autres (non précisés).
- Besoin : **les données restent sur la clé USB et ne sont pas chargées en mémoire** ; le programme va chercher ce dont
  il a besoin à la demande.

Constats sur les deux projets (lecture de leurs docs) :
- Tyrann charge aujourd'hui son paquet à la compilation (`mkneo`) ; ses sauvegardes sont 3 emplacements avec somme de
  contrôle.
- AigleDorEngine décrit salles, costumes, scripts et verbes dans un fichier de données ; 3 emplacements de sauvegarde.
- Je n'ai pas trouvé dans leurs docs de besoin de recherche ou de mise à jour des données de jeu : c'est une demande de
  bmarty, pas un constat.

Conséquences sur la conception (propositions, à valider) :
- Le groupe sert d'abord à **lire un enregistrement par numéro ou par clé et à le déposer directement dans une page
  mémoire** (comme 3,27 et 38,7), sans tampon en RAM 6502 : cela répond à « pas en mémoire » et à la marge RAM de
  3 920 o.
- create et update servent aux données qui changent (sauvegardes, états de partie) ; la lecture reste le cas courant.
- La recherche par clé doit éviter le parcours complet : un **index trié par clé en tête du fichier**, relu à la demande
  (comme la table de 38,x), est à reconsidérer pour les données quasi en lecture seule. L'insertion réécrit l'index :
  acceptable si create est rare.
- Format commun possible avec NR1 (groupe 38) à examiner avant de créer un second format.
- Les builds Oric d'AigleDorEngine n'utilisent pas Trinity : ils ne profiteraient pas du groupe 40.

## 8. Lot 1 livré (0.16.97, 2026-11-09, hors carte)

Groupe **40** : 40,1 Create, 40,2 Open, 40,3 Close, 40,4 Info, 40,5 Read, 40,6 Update, 40,7 Append, 40,8 Delete,
40,9 Search. Format ND1 : en-tête de 16 octets (`ND`, version 1, taille d'enregistrement 2..4096, nombre
d'enregistrements u32, limité à 65535 par le firmware), enregistrements de longueur fixe, octet 0 = statut (1 actif,
0 supprimé). Statique RAM : 5 octets ; recherche : tampon de 96 octets sur la pile (champ dans les 96 premiers octets).
Écarté pour l'instant : index trié et lecture par clé, qui restent à faire si les mesures sur carte montrent que la
recherche séquentielle est trop lente. Mesure à faire : temps de 40,9 sur clé USB (inconnu).
