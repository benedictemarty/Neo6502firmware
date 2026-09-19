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
