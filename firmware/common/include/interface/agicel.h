/* SPDX-License-Identifier: EUPL-1.2 */
/*
 * agicel : dessin d'un cel de vue AGI masqué par la priorité, et restauration
 * d'un rectangle depuis le plan (FW-2, Trinity T-109). Écrit d'après
 * docs/specs/view-v2.md (format et règles d'AGI Specs) ; aucune allocation.
 *
 * Le plan AGI (160 x 168 octets, priorité << 4 | visuel) n'est jamais modifié.
 * La sortie est un écran logique de 160 x 168 couleurs (un octet par pixel
 * AGI) ; le firmware fait le doublement en largeur.
 */
#ifndef AGICEL_H
#define AGICEL_H

#include <stddef.h>
#include <stdint.h>

#define AGICEL_LARGEUR 160
#define AGICEL_HAUTEUR 168

#define AGICEL_OK      0
#define AGICEL_PARAM   1        /* cel vide, position hors du plan, adresse hors mémoire */
#define AGICEL_FORMAT  2        /* données du cel coupées par la fin de la mémoire */

/* Priorité retenue quand la recherche sous une ligne de contrôle atteint le
 * bas du plan (hypothèse, docs/specs/view-v2.md). */
#define AGICEL_PRIO_BAS 15

/* Priorité effective du point (x, y) du plan : sous une ligne de contrôle
 * (0 à 3), celle du premier point plus bas qui n'en est pas une. */
uint8_t agicel_priorite(const uint8_t *plan, int x, int y);

/*
 * Dessine le cel dont l'en-tête est à mem[adresse] (mem : mem_lg octets),
 * x à gauche, y_bas = ligne du bas, priorité de l'objet, miroir (0 ou 1).
 * Un pixel est écrit dans ecran (160 x 168) s'il n'est pas transparent, s'il
 * est dans le plan, et si priorite >= agicel_priorite() (hypothèse « >= »).
 * Une ligne plus longue que la largeur est coupée.
 */
int agicel_dessiner(const uint8_t *mem, size_t mem_lg, size_t adresse, int x, int y_bas,
                    uint8_t priorite, int miroir, const uint8_t *plan, uint8_t *ecran);

/* Recopie le visuel du plan dans l'écran sur le rectangle donné (coupé au plan). */
int agicel_restaurer(int x, int y, int largeur, int hauteur, const uint8_t *plan, uint8_t *ecran);

/* Mêmes fonctions, chaque pixel étant écrit par poser(ctx, x, y, couleur) au
 * lieu d'un tampon (le firmware écrit directement à l'écran). */
typedef void (*agicel_poser_f)(void *ctx, int x, int y, uint8_t couleur);
int agicel_dessiner_vers(const uint8_t *mem, size_t mem_lg, size_t adresse, int x, int y_bas,
                         uint8_t priorite, int miroir, const uint8_t *plan, agicel_poser_f poser, void *ctx);
int agicel_restaurer_vers(int x, int y, int largeur, int hauteur, const uint8_t *plan,
                          agicel_poser_f poser, void *ctx);

#endif
