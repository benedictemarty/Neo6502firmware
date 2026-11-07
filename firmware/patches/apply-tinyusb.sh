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
# T-137 (2026-11-08) : a freed endpoint kept its interrupt slot number (TinyUSB #4173, fixed upstream
# after 0.21.0) and, earlier in the pool, took the completions of the device now on that slot : a DS4
# pad mounted but never delivered a report. Applied after T-100.
if grep -q "Trinity T-137" src/portable/raspberrypi/rp2040/hcd_rp2040.c; then
    echo "TinyUSB : correctif T-137 déjà appliqué"
else
    git apply "$HERE/tinyusb-int-ep-stale-num.diff" 2>/dev/null || \
    patch -p1 < "$HERE/tinyusb-int-ep-stale-num.diff"
    echo "TinyUSB : correctif T-137 appliqué"
fi
