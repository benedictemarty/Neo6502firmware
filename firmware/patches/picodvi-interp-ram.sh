#!/bin/sh
# PicoDVI : interp_save / interp_restore en RAM dans l'encodeur TMDS (Trinity, 2026-10-01).
# tmds_encode_data_channel_16bpp et _8bpp sont en RAM (__not_in_flash_func), mais appellent interp_save
# et interp_restore du SDK, qui sont en FLASH : six appels par ligne encodée en 16 bpp, faits par le
# cœur 1. Quand le cœur 0 exécute du code froid en flash (démarrage : énumération USB, initialisations),
# le cœur 1 attend la flash et la ligne part en retard (traits). Même règle que T-44 / T-93 : rien de
# ce qu'exécute le cœur 1 pendant l'affichage ne doit vivre en flash. Copies inline, même effet.
# Idempotent. $1 = dossier source de PicoDVI.
set -e
F="$1/software/libdvi/tmds_encode.c"
[ -f "$F" ] || { echo "PicoDVI : $F absent" >&2; exit 1; }
if grep -q "Trinity interp RAM" "$F"; then
    echo "PicoDVI : correctif interp en RAM déjà appliqué"
    exit 0
fi
grep -q '^#include "hardware/interp.h"' "$F" || { echo "PicoDVI : include hardware/interp.h introuvable" >&2; exit 1; }
sed -i '0,/^#include "hardware\/interp.h"/s//#include "hardware\/interp.h"\n\n\/\/ Trinity interp RAM : interp_save \/ interp_restore du SDK sont en flash ; copies inline pour que\n\/\/ l'"'"'encodeur (en RAM, cœur 1) ne touche jamais la flash (voir firmware\/patches\/picodvi-interp-ram.sh).\nstatic inline __attribute__((always_inline)) void tmds_interp_save(interp_hw_t *interp, interp_hw_save_t *s) {\n\ts->accum[0] = interp->accum[0]; s->accum[1] = interp->accum[1];\n\ts->base[0] = interp->base[0]; s->base[1] = interp->base[1]; s->base[2] = interp->base[2];\n\ts->ctrl[0] = interp->ctrl[0]; s->ctrl[1] = interp->ctrl[1];\n}\nstatic inline __attribute__((always_inline)) void tmds_interp_restore(interp_hw_t *interp, interp_hw_save_t *s) {\n\tinterp->accum[0] = s->accum[0]; interp->accum[1] = s->accum[1];\n\tinterp->base[0] = s->base[0]; interp->base[1] = s->base[1]; interp->base[2] = s->base[2];\n\tinterp->ctrl[0] = s->ctrl[0]; interp->ctrl[1] = s->ctrl[1];\n}/' "$F"
sed -i 's/\binterp_save(interp\([01]\)_hw/tmds_interp_save(interp\1_hw/g; s/\binterp_restore(interp\([01]\)_hw/tmds_interp_restore(interp\1_hw/g' "$F"
grep -q "Trinity interp RAM" "$F" && ! grep -q "[^_]interp_save(interp[01]_hw" "$F" || { echo "PicoDVI : échec du correctif interp en RAM" >&2; exit 1; }
echo "PicoDVI : correctif interp en RAM appliqué"
