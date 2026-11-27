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
# T-167 (2026-11-25) : a data sequence error panicked ("Data Seq Error") and froze the machine ; the transfer now fails,
# and the MSC class reports the failure of any transfer of a command (it ignored it). Applied after T-100 and T-137.
if grep -q "Trinity T-167" src/portable/raspberrypi/rp2040/hcd_rp2040.c; then
    echo "TinyUSB : correctif T-167 déjà appliqué"
else
    git apply "$HERE/tinyusb-data-seq.diff" 2>/dev/null || \
    patch -p1 < "$HERE/tinyusb-data-seq.diff"
    echo "TinyUSB : correctif T-167 appliqué"
fi
# T-168 (2026-11-25) : a transfer that timed out kept the USB key busy until it was unplugged. Writes the transfer abort
# and the data toggle reset of the RP2040 host (missing, or panic()) and an MSC abort, used by msc_recovery.cpp.
# Applied after T-167.
if grep -q "Trinity T-168" src/portable/raspberrypi/rp2040/hcd_rp2040.c; then
    echo "TinyUSB : correctif T-168 déjà appliqué"
else
    git apply "$HERE/tinyusb-msc-recovery.diff" 2>/dev/null || \
    patch -p1 < "$HERE/tinyusb-msc-recovery.diff"
    echo "TinyUSB : correctif T-168 appliqué"
fi
# T-168 (2026-11-27) : the Bulk-Only reset of the key did not bring it back on the board. A port of the hub can now be
# enumerated again (hub.c), used by msc_recovery.cpp. Applied after the former T-168 patch.
if grep -q "Trinity T-168" src/host/hub.c; then
    echo "TinyUSB : correctif T-168 (hub) déjà appliqué"
else
    git apply "$HERE/tinyusb-hub-replug.diff" 2>/dev/null || \
    patch -p1 < "$HERE/tinyusb-hub-replug.diff"
    echo "TinyUSB : correctif T-168 (hub) appliqué"
fi
