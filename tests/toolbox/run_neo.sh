#!/bin/bash
# run_neo.sh — joue un test tests/toolbox/NOM.asm (ou TESTDIR=tests/api NOM.asm) dans l'émulateur neo (bin/neo) et compare le journal
# console (copie en RAM $2000.., longueur en $1FFE, écrite par wchar) à l'attendu tests/toolbox/NOM.expected.
#   tests/toolbox/run_neo.sh quickdraw        (0 si identique ; --ref régénère l'attendu)
set -u
HERE=$(cd "$(dirname "$0")/../.." && pwd)
REF=0; [ "${1:-}" = "--ref" ] && { REF=1; shift; }
NAME=${1:?usage: run_neo.sh [--ref] NOM}
DIR=$(cd "$(dirname "$0")" && pwd); TDIR=$(cd "${TESTDIR:-$DIR}" && pwd)      # tests/toolbox ou tests/api (TESTDIR=)
OUT=$HERE/build/tests/$(basename "$TDIR")/$NAME; rm -rf "$OUT"; mkdir -p "$OUT/storage"
cp "$TDIR"/*.res "$TDIR"/*.bin "$OUT/storage/" 2>/dev/null
mkdir -p "$OUT/storage1" && printf "un" > "$OUT/storage1/vol1.txt"                 # volume 1 (3,24-3,26)
sed 's/^NEO = 0/NEO = 1/' "$TDIR/$NAME.asm" > "$OUT/$NAME.asm"
64tass --mw65c02 --nostart -q -o "$OUT/$NAME.neo6502" "$OUT/$NAME.asm" || exit 2
ARGS=""; [ -r "$TDIR/$NAME.args" ] && ARGS=$(cat "$TDIR/$NAME.args")             # crochets neo : mouse:C:X,Y,B keys:C:TEXTE cycles:N (T-19)
cd "$OUT" && timeout 120 "$HERE/bin/neo" "$NAME.neo6502@800" run@800 $ARGS > neo.log 2>&1
python3 - "$OUT/memory.dump" > "$OUT/journal.txt" <<'PY'
import sys
m=open(sys.argv[1],'rb').read()
n=m[0x1FFE]|(m[0x1FFF]<<8)
s=m[0x2000:n].decode('latin-1')
print(s.replace('\x0c','').replace('\r','\n'),end='')
PY
if [ $REF = 1 ]; then cp "$OUT/journal.txt" "$TDIR/$NAME.expected"; echo "référence écrite : $NAME.expected"; cat "$OUT/journal.txt"; exit 0; fi
if [ -r "$TDIR/$NAME.expected.re" ]; then                                   # attendu en expressions régulières (une par ligne)
    python3 - "$TDIR/$NAME.expected.re" "$OUT/journal.txt" <<'PY' && echo "OK : $NAME" || { echo "ÉCHEC : $NAME"; exit 1; }
import re,sys
exp=open(sys.argv[1]).read().splitlines(); got=open(sys.argv[2]).read().splitlines()
ok = len(exp)==len(got) and all(re.fullmatch(e,g) for e,g in zip(exp,got))
if not ok: print("attendu :",exp,"\nobtenu  :",got)
sys.exit(0 if ok else 1)
PY
elif diff "$TDIR/$NAME.expected" "$OUT/journal.txt"; then echo "OK : $NAME"; else echo "ÉCHEC : $NAME"; exit 1; fi
