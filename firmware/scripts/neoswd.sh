#!/bin/sh
# ***************************************************************************************
#
#      Name :      neoswd.sh
#      Author :    bmarty <bmarty@mailo.com>
#      Purpose :   T-76 : read the firmware's counters live, through SWD, while the Neo runs.
#
#                  The debug UART needed the firmware to be healthy enough to talk. This does
#                  not : the probe reads the RP2040's memory over the bus, so it keeps working
#                  when the screen is black and even when the firmware is stuck — which is
#                  exactly the state T-71 leaves the machine in.
#
#      Probe :     a Pico flashed with Raspberry Pi's debugprobe firmware (CMSIS-DAP), wired
#                  GP2 -> SWC, GP3 -> SWD, GND -> GND. The Neo must be powered.
#
#      Usage :     firmware/scripts/neoswd.sh [intervalle_ms] [firmware.elf]
#
#      Reads :     trames    frameCounter — incremented by core 1 at every frame start. It is
#                            the number that settles a black screen : still climbing means the
#                            signal lives and the picture is merely wrong ; frozen means the
#                            encoder stopped.
#                  late      lateTotal (T-57) — episodes of late scanlines.
#                  secteurs  stoSectorCount (T-73) — sectors moved by the storage driver.
#
# ***************************************************************************************
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
MS=${1:-500}
ELF=${2:-$HERE/../firmware.elf}
[ -f "$ELF" ] || { echo "neoswd : $ELF introuvable (make -C firmware build)"; exit 1; }

# Les adresses changent à chaque compilation : on les relit dans l'ELF plutôt que de les figer.
addr() { arm-none-eabi-nm "$ELF" | awk -v s="$1" '$3 == s { print "0x" $1 }'; }
FRAMES=$(addr frameCounter)
GMODE=$(addr gMode)
SCREEN=$(addr screenMemory)
VRAM=$(addr graphicsMemory)
LATE=$(addr _ZL9lateTotal)
SECTORS=$(addr stoSectorCount)
[ -n "$FRAMES" ] || { echo "neoswd : symboles introuvables dans $ELF"; exit 1; }
XGSIZE=$(printf "0x%x" $(( GMODE + 4 )))   # gMode.xGSize : 320 en mode 0, 720 en mode 1
echo "neoswd : trames=$FRAMES late=$LATE secteurs=$SECTORS largeur=$XGSIZE ecran=$SCREEN (intervalle ${MS} ms)"

exec openocd -f interface/cmsis-dap.cfg -c "set USE_CORE 0" -f target/rp2040.cfg \
    -c "adapter speed 1000" -c "init" \
    -c "set previous 0" \
    -c "while {1} {
            set f [read_memory $FRAMES 16 1]
            set l [read_memory $LATE 32 1]
            set s [read_memory $SECTORS 32 1]
            set x [read_memory $XGSIZE 16 1]
            set m [read_memory $SCREEN 32 1]
            set plein 0
            for {set k 0} {\$k < 16} {incr k} {
                set mot [read_memory [expr {$VRAM + \$k * 1800}] 32 1]
                if {\$mot != 0} { incr plein }
            }
            set delta [expr {(\$f - \$previous) & 0xFFFF}]
            set previous \$f
            echo [format \"trames=%5d (+%3d)  late=%4d  sect=%5d  larg=%4d  ecran=0x%08x  vram=%2d/16\" \$f \$delta \$l \$s \$x \$m \$plein]
            sleep $MS
        }"
