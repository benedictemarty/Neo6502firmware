#!/bin/sh
# Applies the bmarty TinyUSB patches (idempotent) to a TinyUSB 0.21.0 checkout DEDICATED to Trinity :
# never to a copy shared with other projects (~/neo-deps/tinyusb-0.21 is used elsewhere).
# T-100 (2026-09-30) : EPX left AVAILABLE after a failed transfer -> "already available" panic on
# the next one, core 0 dead (USB key pulled out during a read). $1 = TinyUSB source dir.
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"
cd "$1"
if grep -q "Trinity T-100" src/portable/raspberrypi/rp2040/hcd_rp2040.c; then
    echo "TinyUSB : correctif T-100 déjà appliqué"
else
    git apply "$HERE/tinyusb-epx-stale-avail.diff" 2>/dev/null || \
    patch -p1 < "$HERE/tinyusb-epx-stale-avail.diff"
    echo "TinyUSB : correctif T-100 appliqué"
fi
