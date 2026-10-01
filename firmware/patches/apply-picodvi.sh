#!/bin/sh
# Applies the bmarty PicoDVI patches to the FetchContent checkout (idempotent).
# Trinity 2026-09-19 : the vertical repeat test in the DMA IRQ uses a mask, not a modulo — a division by a
# variable inside dvi_dma_irq_handler (core1, RAM) gave a black screen on the board ; vertical_repeat must be 1 or 2.
# $1 = PicoDVI source dir. Runtime vertical repeat is needed by the video modes (F-52/F-53).
set -e
cd "$1"
if grep -q "vertical_repeat" software/libdvi/dvi.h; then
    echo "PicoDVI : correctif vertical_repeat déjà appliqué"
else
    git apply "$(dirname "$0")/picodvi-vertical-repeat.diff" 2>/dev/null || \
    patch -p1 < "$(dirname "$0")/picodvi-vertical-repeat.diff"
    echo "PicoDVI : correctif vertical_repeat appliqué"
fi
# 2026-09-30 : compatibilité pico-sdk 2.x (tcr -> dbg_tcr), sans effet avec le SDK 1.5.1.
sh "$(dirname "$0")/picodvi-dbg-tcr.sh" "$1"
# 2026-10-01 : interp_save / interp_restore en RAM dans l'encodeur TMDS (le cœur 1 ne doit jamais toucher la flash).
sh "$(dirname "$0")/picodvi-interp-ram.sh" "$1"
