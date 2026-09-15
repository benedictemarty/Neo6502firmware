#!/bin/sh
# Applies the bmarty PicoDVI patches to the FetchContent checkout (idempotent).
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
