#!/bin/sh
# bmarty R22 : FF_FS_TINY 1 in a FatFs ffconf.h (idempotent). One sector buffer per volume instead of one
# per open file : 8 FIL of 552 bytes become 8 of 40, about 4 KB of SRAM given back to the heap.
# $1 = ffconf.h
set -e
f="$1"
if grep -q "^#define FF_FS_TINY[[:space:]]*1" "$f"; then
    echo "FatFs : FF_FS_TINY 1 déjà appliqué ($f)"
else
    sed -i 's/^#define FF_FS_TINY[[:space:]]*0/#define FF_FS_TINY\t\t1\t\/* bmarty R22 *\//' "$f"
    grep -q "^#define FF_FS_TINY[[:space:]]*1" "$f" || { echo "FatFs : FF_FS_TINY introuvable dans $f"; exit 1; }
    echo "FatFs : FF_FS_TINY 1 appliqué ($f)"
fi
