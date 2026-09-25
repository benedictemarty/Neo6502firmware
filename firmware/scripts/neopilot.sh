#!/bin/sh
# ***************************************************************************************
#
#      Name :      neopilot.sh
#      Author :    bmarty <bmarty@mailo.com>
#      Purpose :   T-71 : drive the Neo through SWD — type at its keyboard and read the
#                  display pipeline — when the screen is black and the debug UART is mute.
#
#                  T-74 gave the firmware a terminal on UART0 and T-75 hunted for the pin ;
#                  the wire has never been found and the port has stayed silent through
#                  every fault. This does not need it : the probe writes straight into the
#                  firmware's keyboard queue (queue/queueTail, keyboard.cpp) and reads the
#                  counters, the PIO FIFO levels and the DMA channels while both cores run.
#
#      RULE :      attach core 0 ONLY (set USE_CORE 0). Attaching core 1 as well drops the
#                  encoder to 15 frames per second and invents a fault that is not there
#                  (measured 2026-09-25) — the instrument must not touch the encoder.
#
#      Usage :     firmware/scripts/neopilot.sh "MODE 1" "DIR"    (types them, then samples)
#
#      Reads :     flevel   PIO0 TX FIFO levels, one byte per state machine. Mode 1 healthy
#                           shows all three fed (0x00070707..0x00080808) ; all three empty
#                           is the black screen.
#                  fdebug   PIO0 sticky flags. 0x07000000 = TXSTALL on the three SMs,
#                           0x00040000 = TXOVER on SM2 — both appear during the cold DIR.
#                  trames   frameCounter : frozen means core 1 has stopped.
#                  rejets   T-71 lot 1 : lines the callback dropped. Stays at 0.
#                  secours  T-71 lot 4 : escapes from the bounded wait on the three data
#                           channels (dvi_tcr_timeouts). Must stay 0 : anything else is the
#                           reload anomaly that used to freeze core 1 for good.
#
# ***************************************************************************************
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
ELF=${ELF:-$HERE/../firmware.elf}
[ -f "$ELF" ] || { echo "neopilot : $ELF introuvable (make -C firmware build)"; exit 1; }
addr() { arm-none-eabi-nm "$ELF" | awk -v s="$1" '$3 == s { print "0x" $1 }'; }

FRAMES=$(addr frameCounter)
LATE=$(addr _ZL9lateTotal)
REJETS=$(addr _ZL14publishRejects)
SECT=$(addr stoSectorCount)
SECOURS=$(addr dvi_tcr_timeouts)   # T-71 lot 4 : echappees de l attente bornee des canaux
REPRISES=$(addr _ZL15displayRestarts)   # T-71 lot 6 : modes redemarres par core 0
GMODE=$(addr gMode)
XG=$(printf "0x%x" $(( GMODE + 4 )))
# La file clavier : deux symboles _ZL5queue existent (clavier 65 o, toolbox 256 o).
QUEUE=$(arm-none-eabi-nm -S "$ELF" | awk '$2 == "00000041" && $4 == "_ZL5queue" { print "0x" $1 }')
TAIL=$(addr _ZL9queueTail)
[ -n "$QUEUE" ] && [ -n "$TAIL" ] || { echo "neopilot : file clavier introuvable"; exit 1; }

SCRIPT=$(mktemp)
trap 'rm -f "$SCRIPT"' EXIT
{
    echo "proc touche {c} {"
    echo "    set t [read_memory $TAIL 8 1]"
    echo "    write_memory [expr {$QUEUE + \$t}] 8 [list \$c]"
    echo "    write_memory $TAIL 8 [list [expr {(\$t + 1) & 63}]]"
    echo "    sleep 40"
    echo "}"
    echo "proc frappe {texte} { foreach ch [split \$texte \"\"] { touche [scan \$ch %c] } ; touche 13 }"
    echo "proc ligne {etiquette} {"
    echo "    echo [format \"%-11s flevel=0x%08x fdebug=0x%08x trames=%5d late=%4d rejets=%4d secours=%4d reprises=%3d sect=%4d larg=%4d\" \\"
    echo "        \$etiquette [read_memory 0x5020000c 32 1] [read_memory 0x50200008 32 1] \\"
    echo "        [read_memory $FRAMES 16 1] [read_memory $LATE 32 1] [read_memory $REJETS 32 1] [read_memory $SECOURS 32 1] [read_memory $REPRISES 32 1] \\"
    echo "        [read_memory $SECT 32 1] [read_memory $XG 16 1]]"
    echo "}"
    echo "ligne depart"
    for texte in "$@"; do
        echo "frappe \"$texte\""
        echo "sleep 1200"
        echo "ligne \"apres\""
    done
    echo "for {set i 1} {\$i <= 20} {incr i} { ligne [format \"+%4.1fs\" [expr {\$i * 0.25}]] ; sleep 250 }"
    echo "shutdown"
} >"$SCRIPT"

exec openocd -f interface/cmsis-dap.cfg -c "set USE_CORE 0" -f target/rp2040.cfg \
    -c "adapter speed 1000" -c "init" -f "$SCRIPT"
