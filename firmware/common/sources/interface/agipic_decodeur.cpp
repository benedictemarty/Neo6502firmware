/*
 * SPDX-License-Identifier: EUPL-1.2
 *
 * agipic_decodeur.cpp - decodeur d'images PICTURE AGI v2 (repris de Neo6502AGI
 * tools/agipic/agipic.c).
 *
 * Auteur : bmarty <bmarty@mailo.com>
 * Projet Neo6502AGI.
 *
 * Ecrit en salle blanche d'apres SPECIFICATION.md. Les donnees des pinceaux
 * (masques des cercles, suite du splatter et positions de depart) sont dans
 * agipic_tables.h, genere par derive_tables.c a partir d'observations de
 * l'oracle.
 *
 * Contraintes respectees : pas d'allocation dynamique, pas de division ni de
 * modulo par une valeur variable (le trace de segment utilise un
 * accumulateur), tables en static const, compilable en C99 et en C++17.
 */
/* Intégration Trinity (T-107, T-108) : fichier repris tel quel de Neo6502AGI
 * tools/agipic/agipic.c (35badbd), seuls changent les chemins d'inclusion et
 * AGIPIC_PILE = 0 : pas de pile statique, la pile est fournie par agipic.cpp
 * (agipic_decoder_pile, dans gfxObjectMemory après le plan). */
#define AGIPIC_PILE 0

#include "interface/agipic_decodeur.h"

#include <string.h>

#include "interface/agipic_tables.h"

/* ------------------------------------------------------------------------ */
/* Contexte de decodage                                                      */
/* ------------------------------------------------------------------------ */

typedef struct {
    const uint8_t *donnees;
    size_t lg;
    size_t pos;            /* prochain octet a lire (peut depasser lg) */
    uint8_t *plan;
    int vis_actif;
    int prio_actif;
    uint8_t vis_couleur;
    uint8_t prio_couleur;
    uint8_t motif;         /* octet de motif (0xF9) */
    uint8_t num_motif;     /* numero de motif (splatter) */
    agipic_stats *st;
    uint8_t *pile_x;       /* pile du remplissage : un element = (x, y) */
    uint8_t *pile_y;
    size_t pile_cap;       /* nombre d'elements */
} contexte;

#if AGIPIC_PILE > 0
/* Pile statique de agipic_decoder (AGIPIC_PILE = 0 : pas de pile statique,
 * seul agipic_decoder_pile est disponible). */
static uint8_t pile_x_statique[AGIPIC_PILE];
static uint8_t pile_y_statique[AGIPIC_PILE];
#endif

/* Lecture d'un octet ; au-dela de la fin du flux : 0xFF. */
static uint8_t lire(contexte *c)
{
    uint8_t v = (c->pos < c->lg) ? c->donnees[c->pos] : (uint8_t)0xFF;
    c->pos++;
    return v;
}

/* Rend le dernier octet lu (il deviendra la commande suivante). */
static void rendre(contexte *c)
{
    c->pos--;
}

/* Ecrit le pixel (x, y) selon les plans actifs.
 * Renvoie 1 si le pixel est dans le plan, 0 sinon. */
static int ecrire(contexte *c, int x, int y)
{
    uint8_t *p;
    if (x < 0 || y < 0 || x >= AGIPIC_LARGEUR || y >= AGIPIC_HAUTEUR)
        return 0;
    p = c->plan + (size_t)y * AGIPIC_LARGEUR + (size_t)x;
    if (c->prio_actif)
        *p = (uint8_t)((*p & 0x0F) | (c->prio_couleur << 4));
    if (c->vis_actif)
        *p = (uint8_t)((*p & 0xF0) | c->vis_couleur);
    return 1;
}

/* ------------------------------------------------------------------------ */
/* Segments et commandes de lignes (sections 4 et 5)                         */
/* ------------------------------------------------------------------------ */

static void ecrire_ligne(contexte *c, int x, int y)
{
    if (ecrire(c, x, y))
        c->st->px_lignes++;
}

static void segment(contexte *c, int x1, int y1, int x2, int y2)
{
    int dx, dy, sx, sy, i, n, acc;

    c->st->segments++;
    if (x1 > AGIPIC_LARGEUR - 1) x1 = AGIPIC_LARGEUR - 1;
    if (x2 > AGIPIC_LARGEUR - 1) x2 = AGIPIC_LARGEUR - 1;
    if (y1 > AGIPIC_HAUTEUR - 1) y1 = AGIPIC_HAUTEUR - 1;
    if (y2 > AGIPIC_HAUTEUR - 1) y2 = AGIPIC_HAUTEUR - 1;

    sx = (x2 >= x1) ? 1 : -1;
    sy = (y2 >= y1) ? 1 : -1;
    dx = (x2 - x1) * sx;
    dy = (y2 - y1) * sy;

    if (dx == 0) {                         /* colonne */
        for (i = 0; i <= dy; i++)
            ecrire_ligne(c, x1, y1 + sy * i);
        return;
    }
    if (dy == 0) {                         /* ligne */
        for (i = 0; i <= dx; i++)
            ecrire_ligne(c, x1 + sx * i, y1);
        return;
    }
    if (dy > dx) {
        /* x_i = x1 + sx * floor((floor(dy/2) + i*dx) / dy) ; dx < dy donc
         * le quotient augmente d'au plus 1 par pas. */
        int q = 0;
        n = dy;
        acc = dy >> 1;
        for (i = 0; i <= n; i++) {
            ecrire_ligne(c, x1 + sx * q, y1 + sy * i);
            acc += dx;
            if (acc >= dy) { acc -= dy; q++; }
        }
    } else {
        int q = 0;
        n = dx;
        acc = dx >> 1;
        for (i = 0; i <= n; i++) {
            ecrire_ligne(c, x1 + sx * i, y1 + sy * q);
            acc += dy;
            if (acc >= dx) { acc -= dx; q++; }
        }
    }
}

/* Lit le point de depart ; renvoie 0 si la commande doit s'arreter. */
static int point_depart(contexte *c, int *x, int *y)
{
    uint8_t a, b;
    a = lire(c);
    if (a >= 0xF0) { rendre(c); return 0; }
    b = lire(c);
    if (b >= 0xF0) { rendre(c); return 0; }
    *x = a;
    *y = b;
    ecrire_ligne(c, a, b);
    return 1;
}

static void cmd_lignes_absolues(contexte *c)
{
    int x, y;
    uint8_t a, b;
    if (!point_depart(c, &x, &y))
        return;
    for (;;) {
        a = lire(c);
        if (a >= 0xF0) { rendre(c); return; }
        b = lire(c);
        if (b >= 0xF0) { rendre(c); return; }
        segment(c, x, y, a, b);
        x = a;
        y = b;
    }
}

static void cmd_lignes_relatives(contexte *c)
{
    int x, y, ddx, ddy;
    uint8_t d;
    if (!point_depart(c, &x, &y))
        return;
    for (;;) {
        d = lire(c);
        if (d >= 0xF0) { rendre(c); return; }
        ddx = (d >> 4) & 7;
        if (d & 0x80) ddx = -ddx;
        ddy = d & 7;
        if (d & 0x08) ddy = -ddy;
        segment(c, x, y, x + ddx, y + ddy);
        x += ddx;
        y += ddy;
    }
}

/* Escalier : x_dabord non nul pour 0xF5, nul pour 0xF4. */
static void cmd_escalier(contexte *c, int x_dabord)
{
    int x, y, sur_x = x_dabord;
    uint8_t v;
    if (!point_depart(c, &x, &y))
        return;
    for (;;) {
        v = lire(c);
        if (v >= 0xF0) { rendre(c); return; }
        if (sur_x) {
            segment(c, x, y, v, y);
            x = v;
        } else {
            segment(c, x, y, x, v);
            y = v;
        }
        sur_x = !sur_x;
    }
}

/* ------------------------------------------------------------------------ */
/* Remplissage (section 6)                                                   */
/* ------------------------------------------------------------------------ */

/* Mode de remplissage : 0 aucun, 1 visuel == 15, 2 priorite == 4. */
static int mode_remplissage(const contexte *c)
{
    if (!c->vis_actif && !c->prio_actif)
        return 0;
    if (!c->prio_actif && c->vis_couleur != 15)
        return 1;
    if (!c->vis_actif && c->prio_couleur != 4)
        return 2;
    if (c->vis_actif && c->vis_couleur != 15)
        return 1;
    return 0;
}

static int remplissable(contexte *c, int mode, int x, int y)
{
    uint8_t v;
    c->st->tests_remplis++;
    v = c->plan[(size_t)y * AGIPIC_LARGEUR + (size_t)x];
    if (mode == 1)
        return (v & 0x0F) == 0x0F;
    return (v >> 4) == 4;
}

static int remplir(contexte *c, int gx, int gy)
{
    int mode = mode_remplissage(c);
    unsigned sp = 0;

    if (mode == 0 || gx >= AGIPIC_LARGEUR || gy >= AGIPIC_HAUTEUR)
        return 0;
    c->pile_x[0] = (uint8_t)gx;
    c->pile_y[0] = (uint8_t)gy;
    sp = 1;
    if (c->st->pile_max < 1) c->st->pile_max = 1;

    while (sp > 0) {
        int x, y, g, d, i, k;
        sp--;
        x = c->pile_x[sp];
        y = c->pile_y[sp];
        if (!remplissable(c, mode, x, y))
            continue;
        /* etendre l'intervalle horizontal */
        g = x;
        while (g > 0 && remplissable(c, mode, g - 1, y))
            g--;
        d = x;
        while (d < AGIPIC_LARGEUR - 1 && remplissable(c, mode, d + 1, y))
            d++;
        for (i = g; i <= d; i++) {
            ecrire(c, i, y);
            c->st->px_remplis++;
        }
        /* germes des rangees voisines : un par suite de cases remplissables */
        for (k = -1; k <= 1; k += 2) {
            int ny = y + k, avant = 0;
            if (ny < 0 || ny >= AGIPIC_HAUTEUR)
                continue;
            for (i = g; i <= d; i++) {
                int r = remplissable(c, mode, i, ny);
                if (r && !avant) {
                    if (sp >= c->pile_cap)
                        return -1;
                    c->pile_x[sp] = (uint8_t)i;
                    c->pile_y[sp] = (uint8_t)ny;
                    sp++;
                    if (sp > c->st->pile_max) c->st->pile_max = sp;
                }
                avant = r;
            }
        }
    }
    return 0;
}

static int cmd_remplissage(contexte *c)
{
    uint8_t a, b;
    for (;;) {
        a = lire(c);
        if (a >= 0xF0) { rendre(c); return 0; }
        b = lire(c);
        if (b >= 0xF0) { rendre(c); return 0; }
        c->st->remplissages++;
        if (remplir(c, a, b) != 0)
            return -1;
    }
}

/* ------------------------------------------------------------------------ */
/* Pinceaux (section 7)                                                      */
/* ------------------------------------------------------------------------ */

static void poser_motif(contexte *c, int x, int y)
{
    int s = c->motif & 7;
    int carre = (c->motif & 0x10) != 0;
    int splat = (c->motif & 0x20) != 0;
    unsigned pos = agipic_splat_depart[c->num_motif & 0x7F];
    int x0, y0, r, col;

    c->st->pinceaux++;
    if (x < s) x = s - 1;
    if (y < s) y = s;
    x0 = x - ((s + 1) >> 1);
    y0 = y - s;
    for (r = 0; r <= 2 * s; r++) {
        uint8_t m = carre ? (uint8_t)0xFF : agipic_masque_cercle[s][r];
        for (col = 0; col <= s; col++) {
            if (!(m & (0x80u >> col)))
                continue;
            if (splat) {
                int bit = (agipic_splat_bits[pos >> 3] >> (7 - (pos & 7))) & 1;
                pos++;
                if (pos >= AGIPIC_SPLAT_PERIODE)
                    pos = 0;
                if (!bit)
                    continue;
            }
            if (ecrire(c, x0 + col, y0 + r))
                c->st->px_pinceaux++;
        }
    }
}

static void cmd_pinceaux(contexte *c)
{
    uint8_t n, a, b;
    for (;;) {
        if (c->motif & 0x20) {
            n = lire(c);
            if (n >= 0xF0) { rendre(c); return; }
            c->num_motif = (uint8_t)((n >> 1) & 0x7F);
        }
        a = lire(c);
        if (a >= 0xF0) { rendre(c); return; }
        b = lire(c);
        if (b >= 0xF0) { rendre(c); return; }
        poser_motif(c, a, b);
    }
}

/* ------------------------------------------------------------------------ */
/* Boucle principale                                                         */
/* ------------------------------------------------------------------------ */

#if AGIPIC_PILE > 0
int agipic_decoder(const uint8_t *donnees, size_t lg, uint8_t *plan,
                   int effacer, agipic_stats *stats)
{
    return agipic_decoder_pile(donnees, lg, plan, effacer, stats,
                               pile_x_statique, pile_y_statique, AGIPIC_PILE);
}
#endif

int agipic_decoder_pile(const uint8_t *donnees, size_t lg, uint8_t *plan,
                        int effacer, agipic_stats *stats,
                        uint8_t *pile_x, uint8_t *pile_y, size_t pile_cap)
{
    contexte c;
    agipic_stats local;
    int ret = 0;

    if (pile_x == NULL || pile_y == NULL || pile_cap == 0)
        return -1;

    memset(&local, 0, sizeof local);
    c.donnees = donnees;
    c.lg = (donnees != NULL) ? lg : 0;
    c.pos = 0;
    c.plan = plan;
    c.pile_x = pile_x;
    c.pile_y = pile_y;
    c.pile_cap = pile_cap;
    c.vis_actif = 0;
    c.prio_actif = 0;
    c.vis_couleur = 15;
    c.prio_couleur = 4;
    c.motif = 0;
    c.num_motif = 0;
    c.st = &local;

    if (effacer)
        memset(plan, AGIPIC_FOND, AGIPIC_TAILLE);

    for (;;) {
        uint8_t cmd = lire(&c);
        if (cmd < 0xF0 || cmd > 0xFA)
            break;
        local.commandes[cmd & 0x0F]++;
        switch (cmd) {
        case 0xF0:
            c.vis_couleur = (uint8_t)(lire(&c) & 0x0F);
            c.vis_actif = 1;
            break;
        case 0xF1:
            c.vis_actif = 0;
            break;
        case 0xF2:
            c.prio_couleur = (uint8_t)(lire(&c) & 0x0F);
            c.prio_actif = 1;
            break;
        case 0xF3:
            c.prio_actif = 0;
            break;
        case 0xF4:
            cmd_escalier(&c, 0);
            break;
        case 0xF5:
            cmd_escalier(&c, 1);
            break;
        case 0xF6:
            cmd_lignes_absolues(&c);
            break;
        case 0xF7:
            cmd_lignes_relatives(&c);
            break;
        case 0xF8:
            ret = cmd_remplissage(&c);
            break;
        case 0xF9:
            c.motif = lire(&c);
            break;
        default: /* 0xFA */
            cmd_pinceaux(&c);
            break;
        }
        if (ret != 0)
            break;
    }

    local.octets = (uint32_t)((c.pos < c.lg) ? c.pos : c.lg);
    if (stats != NULL)
        *stats = local;
    return ret;
}
