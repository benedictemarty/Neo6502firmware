# ADR-04 — Modes « mémoire » : rendu vidéo depuis la RAM 6502 (F-70, F-71, F-72, F-73)

Statut : **proposée** (2026-09-18), aucun code ; à trancher par bmarty avant tout
développement (stories P3 de l'épopée F7, « gros, décision par ADR »).

## Contexte
Les projets Neo6502oric2, Neo6502bbc et l'EPIC-02 (Apple II) veulent faire tourner
du code 6502 d'origine qui écrit **directement sa page écran en RAM** (Oric TEXT
`$BB80`, HIRES `$A000` ; Apple II texte `$0400` ; BBC MODE 7 `$7C00`, MODE 0-6),
sans passer par l'API graphique. Aujourd'hui le rendu (core 1, `dvi_320x240x256.cpp`,
`_encode_loop`) lit exclusivement `graphicsMemory` (76,8 Ko en SRAM RP2040), jamais
`cpuMemory`.

Faits vérifiés :
- `cpuMemory` (64 Ko) et `graphicsMemory` sont deux tableaux de la SRAM du RP2040,
  lisibles par les deux cœurs. core 0 sert le bus 65C02 (`processor_pio.cpp`) ;
  core 1 encode les lignes (`_scanline_callback` + `_encode_loop`, F-50/F-52). Une
  lecture de `cpuMemory` par core 1 pendant que core 0 y écrit n'a pas d'autre
  effet qu'un déchirement d'image d'une trame (pas de verrou nécessaire).
- Le chemin lecture de la boucle bus n'a que **11 `nop` de marge** (F-60, R9) :
  toute interception d'adresse (F-73) y coûterait des cycles non mesurables sans
  carte (règle 4). Les fonctions 1,18 (banques) ont été conçues pour ne rien y
  ajouter (ADR-03) ; même contrainte ici.
- Budget de rendu (F-50) : mode 0 = encodeur 8 bpp doublé ≈ 60 % de core 1 ;
  mode 1 = encodeur 1 bpp 720 px. Une conversion par ligne en amont de l'encodeur
  (RAM 6502 → tampon de ligne 8 bpp ou 1 bpp) s'ajoute à ce budget.
- Les émulateurs `neo` et Phosphoneo rendent depuis `gMode.displayMemory` par
  `GFXReadPixelRaw` : un mode qui rend depuis la RAM 6502 exige un convertisseur
  **commun** (`firmware/common`) pour que les trois backends restent identiques.
- Oracles disponibles : Phosphoric (`~/Oric1`, rendu Oric TEXT/HIRES et attributs
  série validés par tests golden), Neo6502bbc (MODE 7 / SAA5050 documenté), aucun
  pour l'Apple II.

## Décision proposée
1. **Un descripteur de mode « mémoire »** dans `gfxModes[]` (extension d'ADR-02) :
   `source = SRC_CPU`, adresse de base dans la RAM 6502, disposition (`LAYOUT_ORIC_TEXT`,
   `LAYOUT_ORIC_HIRES`, `LAYOUT_APPLE_TEXT`, `LAYOUT_BBC_MODE7`, …) et police
   associée. `graphicsMemory` reste inutilisé dans ces modes (la console 2,x est
   désactivée : toute écriture texte passe par la RAM 6502 du programme).
2. **Convertisseur de ligne commun** `MEMRenderLine(mode, y, dest)` dans
   `firmware/common/sources/interface/memvideo.cpp` : produit une ligne de pixels
   (8 bpp index palette, ou 1 bpp pour les dispositions monochromes) à partir de
   `cpuMemory`. Sur la carte, `_encode_loop` l'appelle avant l'encodeur TMDS du mode
   hôte (8 bpp doublé pour 240/280/320 px de large, 1 bpp 720 px pour MODE 7
   480 px si doublé 1,5 — à trancher) ; dans `neo`/Phosphoneo, `GFXReadPixelRaw` le
   consulte via un tampon de ligne. **Aucune modification de la boucle bus.**
3. **Attributs série Oric** (TEXT) : l'état (encre, papier, inverse, double hauteur,
   clignotement, jeu de caractères) est rejoué de gauche à droite par ligne, comme
   l'ULA ; le jeu de caractères est lu en RAM 6502 (`$B400`/`$B800`), donc les
   programmes qui redéfinissent des glyphes fonctionnent sans API. Clignotement : phase
   du timer 100 Hz comme `CONBlinkSync`.
4. **F-73 (interception d'adresses) : refusée dans la boucle bus.** Les « soft
   switches » à effet de bord en lecture (Apple `$C0xx`) ne sont pas réalisables sans
   toucher au chemin critique ; les registres purement en RAM (VIA Oric `$0300` en
   écriture, attributs) sont relus par core 1 à chaque trame (scrutation) — suffisant
   pour l'affichage. Les programmes qui dépendent d'une lecture à effet de bord passent
   par l'API (comme aujourd'hui).
5. **Ordre de réalisation** : (a) F-71 TEXT Oric — oracle Phosphoric, clients Oric
   existants, 40×28 × 6×8 = 240×224 px dans le timing du mode 0 ; (b) F-71 HIRES
   (240×200, 1 bpp + attributs) ; (c) F-70 Apple II texte (police et entrelacement
   `$0400`, pas d'oracle : validation visuelle) ; (d) F-72 MODE 7 (SAA5050 : 12×20,
   480×500 → timing 720×480 à définir) puis MODE 0-6 (6845 : dispositions par
   caractère, palette ULA) — chacun avec son test golden dans `neo` et Phosphoneo.
6. **Mesure sur carte obligatoire avant merge** de (a) : protocole F-50 §4 (cycles
   de `_scanline_callback` par ligne, moyenne et max) ; seuil : le total encodeur +
   conversion doit rester < 8 000 cycles/ligne à 252 MHz. Estimation (à confirmer) :
   40 cellules × (lecture code + attribut + 8 lignes × 6 px) ≈ 1 200-1 500 cycles/ligne,
   soit +15-20 % sur le mode 0 — acceptable si la mesure le confirme.

## Conséquences
- Pas de dépendance à F-23 ni à la flash ; pas de SRAM supplémentaire (tampon de ligne
  ≤ 720 octets).
- Les émulateurs restent exacts par construction (convertisseur partagé) ; Phosphoric
  fournit l'oracle Oric (comparaison PPM cellule par cellule, comme Neo6502scumm).
- Point ouvert pour bmarty : (i) valider l'ordre (a)-(d) et le refus de F-73 dans la
  boucle bus ; (ii) choisir le timing pour MODE 7 (480 px : 640×480 doublé H impossible,
  720 px natif avec 12 px par cellule = 480 + bandes ?) ; (iii) confirmer que la console
  2,x est hors service dans ces modes (ou superposée par le mode 0 en page 1, F-55).

## Risques
| # | Risque | Vérification |
|---|---|---|
| R30 | Conversion par ligne trop lente sur core 1 (décrochage DVI) | mesure F-50 §4 sur carte avant merge ; repli : rendu par trame dans `graphicsMemory` à 30 Hz |
| R31 | Déchirement d'image (core 0 écrit pendant le rendu) | acceptable (comportement des machines d'origine sans double tampon) |
| R32 | Attributs série Oric incomplets (double hauteur, clignotement) | golden Phosphoric cellule par cellule sur les scènes de test de `~/Oric1` |
