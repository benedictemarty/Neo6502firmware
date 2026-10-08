/* SPDX-License-Identifier: EUPL-1.2 */
/*
 * agicel.cpp : repris tel quel de Neo6502AGI tools/agicel/agicel.c (EUPL 1.2),
 * seul le chemin d'inclusion change ; octet 2 de l'en-tête au format PC (T-109,
 * 2026-10-08 : couleur transparente dans le quartet faible). Écrit d'après Neo6502AGI
 * docs/specs/view-v2.md, elle-même tirée d'AGI Specs (doc/agispecs.sgml de
 * https://github.com/cmatsuoka/sarien ; licence citée intégralement dans
 * view-v2.md).
 */
#include "interface/agicel.h"

uint8_t agicel_priorite(const uint8_t *plan, int x, int y)
{
    for (; y < AGICEL_HAUTEUR; y++) {
        uint8_t p = plan[y * AGICEL_LARGEUR + x] >> 4;
        if (p >= 4)
            return p;
    }
    return AGICEL_PRIO_BAS;
}

int agicel_dessiner_vers(const uint8_t *mem, size_t mem_lg, size_t adresse, int x, int y_bas,
                         uint8_t priorite, int miroir, const uint8_t *plan, agicel_poser_f poser, void *ctx)
{
    size_t i = adresse;
    int largeur, hauteur, y0, ligne;
    uint8_t transparent;

    if (adresse + 3 > mem_lg)
        return AGICEL_PARAM;
    largeur = mem[i];
    hauteur = mem[i + 1];
    transparent = mem[i + 2] & 0x0F;            /* format PC : bit 7 miroir, bits 6-4 boucle */
    i += 3;
    if (largeur == 0 || hauteur == 0 || x < 0 || x >= AGICEL_LARGEUR || y_bas < 0 || y_bas >= AGICEL_HAUTEUR)
        return AGICEL_PARAM;
    y0 = y_bas - hauteur + 1;
    for (ligne = 0; ligne < hauteur; ligne++) {
        int col = 0, y = y0 + ligne;
        for (;;) {
            uint8_t b, couleur;
            int n;
            if (i >= mem_lg)
                return AGICEL_FORMAT;
            b = mem[i++];
            if (b == 0)
                break;
            couleur = b >> 4;
            for (n = b & 0x0F; n > 0; n--, col++) {
                int sx;
                if (col >= largeur || couleur == transparent || y < 0)
                    continue;               /* ligne coupée, transparent, au-dessus du plan */
                sx = x + (miroir ? largeur - 1 - col : col);
                if (sx >= AGICEL_LARGEUR)
                    continue;
                if (priorite >= agicel_priorite(plan, sx, y))
                    poser(ctx, sx, y, couleur);
            }
        }
    }
    return AGICEL_OK;
}

int agicel_restaurer_vers(int x, int y, int largeur, int hauteur, const uint8_t *plan,
                          agicel_poser_f poser, void *ctx)
{
    int xx, yy;
    if (largeur <= 0 || hauteur <= 0)
        return AGICEL_PARAM;
    for (yy = y; yy < y + hauteur; yy++) {
        if (yy < 0 || yy >= AGICEL_HAUTEUR)
            continue;
        for (xx = x; xx < x + largeur; xx++)
            if (xx >= 0 && xx < AGICEL_LARGEUR)
                poser(ctx, xx, yy, plan[yy * AGICEL_LARGEUR + xx] & 0x0F);
    }
    return AGICEL_OK;
}

/* Adaptateurs vers un tampon de 160 x 168 couleurs. */
static void poser_tampon(void *ctx, int x, int y, uint8_t couleur)
{
    ((uint8_t *)ctx)[y * AGICEL_LARGEUR + x] = couleur;
}

int agicel_dessiner(const uint8_t *mem, size_t mem_lg, size_t adresse, int x, int y_bas,
                    uint8_t priorite, int miroir, const uint8_t *plan, uint8_t *ecran)
{
    return agicel_dessiner_vers(mem, mem_lg, adresse, x, y_bas, priorite, miroir, plan, poser_tampon, ecran);
}

int agicel_restaurer(int x, int y, int largeur, int hauteur, const uint8_t *plan, uint8_t *ecran)
{
    return agicel_restaurer_vers(x, y, largeur, hauteur, plan, poser_tampon, ecran);
}
