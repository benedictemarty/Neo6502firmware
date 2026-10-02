/*
 * SPDX-License-Identifier: EUPL-1.2
 *
 * agipic_decodeur.h - decodeur d'images PICTURE AGI v2 (repris de Neo6502AGI
 * tools/agipic/agipic.h ; seule la garde d'inclusion est renommee).
 *
 * Auteur : bmarty <bmarty@mailo.com>
 * Projet Neo6502AGI.
 *
 * Ecrit en salle blanche d'apres SPECIFICATION.md ; les tables des pinceaux
 * (agipic_tables.h) ont ete obtenues par observation de l'oracle
 * (outil derive_tables.c).
 */
#ifndef AGIPIC_DECODEUR_H
#define AGIPIC_DECODEUR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AGIPIC_LARGEUR 160
#define AGIPIC_HAUTEUR 168
#define AGIPIC_TAILLE  (AGIPIC_LARGEUR * AGIPIC_HAUTEUR)
#define AGIPIC_FOND    0x4f

/* Capacite de la pile du remplissage (elements de 2 octets, en memoire
 * statique). La specification exige au moins 512. */
#ifndef AGIPIC_PILE
#define AGIPIC_PILE 1024
#endif

typedef struct {
    uint32_t octets;          /* octets du flux consommes (lus et non rendus) */
    uint32_t commandes[16];   /* nombre de commandes executees, indice = code & 0x0F (0xF0..0xFA) */
    uint32_t segments;        /* appels au trace de segment, y compris ceux de longueur nulle */
    uint32_t px_lignes;       /* pixels dans le plan ecrits par les commandes de ligne (points de depart compris) */
    uint32_t pinceaux;        /* motifs de pinceau poses */
    uint32_t px_pinceaux;     /* pixels dans le plan ecrits par les pinceaux */
    uint32_t remplissages;    /* germes de remplissage traites */
    uint32_t px_remplis;      /* pixels ecrits par les remplissages */
    uint32_t tests_remplis;   /* tests "remplissable ?" effectues */
    uint32_t pile_max;        /* profondeur maximale atteinte par la pile de remplissage */
} agipic_stats;

/*
 * Decode le flux `donnees` (lg octets) dans `plan` (AGIPIC_TAILLE octets,
 * ligne par ligne, octet = (priorite << 4) | visuel).
 * `effacer` non nul : le plan est d'abord rempli avec AGIPIC_FOND.
 * `stats` peut etre NULL.
 * Retour : 0, ou -1 si la pile de remplissage est epuisee.
 * Non reentrant (la pile de remplissage est statique).
 */
int agipic_decoder(const uint8_t *donnees, size_t lg, uint8_t *plan,
                   int effacer, agipic_stats *stats);

#ifdef __cplusplus
}
#endif

#endif /* AGIPIC_DECODEUR_H */
