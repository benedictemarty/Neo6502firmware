#!/bin/bash
# run_neo.sh — joue un test tests/toolbox/NOM.asm dans l'émulateur neo (bin/neo) et compare le journal
# console (copie en RAM $2000.., longueur en $1FFE, écrite par wchar) à l'attendu tests/toolbox/NOM.expected.
#   tests/toolbox/run_neo.sh quickdraw        (0 si identique ; --ref régénère l'attendu)
set -u
HERE=$(cd "$(dirname "$0")/../.." && pwd)
REF=0; [ "${1:-}" = "--ref" ] && { REF=1; shift; }
NAME=${1:?usage: run_neo.sh [--ref] NOM}
OUT=$HERE/build/tests/toolbox/$NAME; rm -rf "$OUT"; mkdir -p "$OUT/storage"
cp "$HERE"/tests/toolbox/*.res "$OUT/storage/" 2>/dev/null
sed 's/^NEO = 0/NEO = 1/' "$HERE/tests/toolbox/$NAME.asm" > "$OUT/$NAME.asm"
64tass --mw65c02 --nostart -q -o "$OUT/$NAME.neo6502" "$OUT/$NAME.asm" || exit 2
cd "$OUT" && timeout 60 "$HERE/bin/neo" "$NAME.neo6502@800" run@800 > neo.log 2>&1
python3 - "$OUT/memory.dump" > "$OUT/journal.txt" <<'PY'
import sys
m=open(sys.argv[1],'rb').read()
n=m[0x1FFE]|(m[0x1FFF]<<8)
s=m[0x2000:n].decode('latin-1')
print(s.replace('\x0c','').replace('\r','\n'),end='')
PY
if [ $REF = 1 ]; then cp "$OUT/journal.txt" "$HERE/tests/toolbox/$NAME.expected"; echo "référence écrite : $NAME.expected"; cat "$OUT/journal.txt"; exit 0; fi
if diff "$HERE/tests/toolbox/$NAME.expected" "$OUT/journal.txt"; then echo "OK : $NAME"; else echo "ÉCHEC : $NAME"; exit 1; fi
