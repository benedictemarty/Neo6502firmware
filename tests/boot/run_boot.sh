#!/bin/bash
# run_boot.sh — tests de démarrage (T-37, ADR-0001 f) : chaque scénario prépare le stockage de neo, démarre
# sans programme, guette une ligne de fin (invite NeoDOS ou message du programme) au plus 60 s, puis vérifie
# les lignes de la console (écho sur la sortie d'erreur de neo), DANS L'ORDRE, et l'absence de lignes interdites.
#   tests/boot/run_boot.sh [scénario...]      (0 si tout passe)
# Non couverts dans neo : clé lente, sans clavier (neo en a toujours un), choix en flash (T-35 non réalisé).
set -u
HERE=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$HERE/build/tests/boot; mkdir -p "$OUT"
MKNEO=${MKNEO:-$HOME/Neo6502Msdos/tools/mkneo.py}
64tass --mw65c02 --nostart -q -o "$OUT/good.bin" "$HERE/tests/boot/good.asm" || exit 2
python3 "$MKNEO" "$OUT/good.bin" "$OUT/good.neo" 800 800 good >/dev/null || { echo "mkneo introuvable : $MKNEO"; exit 2; }
64tass --mw65c02 --nostart -q -o "$OUT/rtos.bin" "$HERE/tests/api/rtos.asm" || exit 2           # T-96 : la démo multitâche (3 tâches)
python3 "$MKNEO" "$OUT/rtos.bin" "$OUT/rtos.neo" 800 800 rtos >/dev/null || exit 2
PROMPT='A:\\>'
fails=0

# scenario NOM FIN "attendu1|attendu2|..." "interdit1|..."  (le stockage est préparé par la fonction prep_NOM)
scenario() {
	local name=$1 until=$2 want=$3 forbid=$4
	local d=$OUT/$name; rm -rf "$d"; mkdir -p "$d"
	( cd "$d" && prep_$name )
	( cd "$d" && exec env NEO_NO_HOST_INPUT=1 "$HERE/bin/neo" >out.txt 2>err.txt ) & local p=$!
	local i; for i in $(seq 1 60); do grep -aq -- "$until" "$d/err.txt" 2>/dev/null && break; sleep 1; done
	sleep 1; kill $p 2>/dev/null; wait $p 2>/dev/null
	tr -d '\r' < "$d/err.txt" | grep -av '^FIS\|^I2C\|^0$\|^$' > "$d/console.txt"
	python3 - "$d/console.txt" "$want" "$forbid" <<'PY' && echo "OK : boot/$name" || { echo "ÉCHEC : boot/$name"; cat "$d/console.txt"; exit 1; }
import re,sys
lines=open(sys.argv[1],errors='replace').read().splitlines()
want=[w for w in sys.argv[2].split('|') if w]; forbid=[f for f in sys.argv[3].split('|') if f]
i=0
for w in want:
    while i < len(lines) and not re.search(w,lines[i]): i+=1
    if i == len(lines): print("manque, dans l'ordre :",w); sys.exit(1)
    i+=1
for f in forbid:
    if any(re.search(f,l) for l in lines): print("interdit :",f); sys.exit(1)
PY
	[ $? -eq 0 ] || fails=$((fails+1))
}

prep_cle_vide()       { mkdir -p storage; }
prep_sans_cle()       { touch storage; }                                   # un FICHIER storage : pas de clé
prep_boot_sans_image(){ mkdir -p storage/boot; echo x > storage/boot/lisezmoi.txt; }
prep_menu_sans_auto() { mkdir -p storage/boot; cp "$OUT/good.neo" storage/boot/good.neo; }
prep_auto_invalide()  { prep_menu_sans_auto; printf 'nope.neo\n' > storage/boot/auto.txt; }
prep_auto_corrompue() { mkdir -p storage/boot; head -c 300 /dev/urandom > storage/boot/bad.neo; printf 'bad.neo\n' > storage/boot/auto.txt; }
prep_auto_valide()    { prep_menu_sans_auto; printf 'good.neo\n' > storage/boot/auto.txt; }
prep_auto_rtos()      { mkdir -p storage/boot; cp "$OUT/rtos.neo" storage/boot/rtos.neo; printf 'rtos.neo\n' > storage/boot/auto.txt; }   # T-96 : auto.txt écrasait le noyau en $FE00

ALL="cle_vide sans_cle boot_sans_image menu_sans_auto auto_invalide auto_corrompue auto_valide auto_rtos"
for s in ${@:-$ALL}; do
	case $s in
	cle_vide)        scenario $s "$PROMPT" "Trinity Firmware|Stored in 'storage'|NeoDOS version|$PROMPT" "^Boot :|no key" ;;
	sans_cle)        scenario $s "$PROMPT" "Trinity Firmware|USB Storage \\(no key\\)|NeoDOS version|$PROMPT" "^Boot :|Stored in" ;;
	boot_sans_image) scenario $s "$PROMPT" "Stored in 'storage'|NeoDOS version|$PROMPT" "^Boot :" ;;
	menu_sans_auto)  scenario $s "$PROMPT" "^Boot : 1 NeoDOS  2 good\\.neo|^-> NeoDOS|NeoDOS version|$PROMPT" "Boot : auto|BOOT OK" ;;
	auto_invalide)   scenario $s "$PROMPT" "^Boot : 1 NeoDOS  2 good\\.neo|^-> NeoDOS|NeoDOS version|$PROMPT" "Boot : auto|BOOT OK" ;;
	auto_corrompue)  scenario $s "$PROMPT" "^Boot : auto bad\\.neo|^-> bad\\.neo|bad\\.neo not started|NeoDOS version|$PROMPT" "BOOT OK" ;;
	auto_valide)     scenario $s "BOOT OK" "^Boot : auto good\\.neo|^-> good\\.neo|BOOT OK" "NeoDOS version|not started" ;;
	auto_rtos)       scenario $s "T=0064" "^Boot : auto rtos\\.neo|^-> rtos\\.neo|B.*T=0064" "not started" ;;
	*) echo "scénario inconnu : $s"; fails=$((fails+1)) ;;
	esac
done
exit $fails
