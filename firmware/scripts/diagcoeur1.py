#!/usr/bin/env python3
# ***************************************************************************************
#
#      Name :      diagcoeur1.py
#      Author :    bmarty <bmarty@mailo.com>
#      Purpose :   Version de diagnostic du cœur 1 (branche diag-coeur1) : lit par SWD, sans rien arrêter,
#                  les lignes en retard par phase du cœur 0 (diagLateOnset), la plus longue durée d'encodage
#                  d'une ligne (diagEncMax, µs), lateTotal (5,40) et le compteur de trames.
#                  Les adresses viennent de l'ELF (variable ELF, défaut firmware/firmware.elf) : il doit être
#                  celui de la carte.
#
#      Usage :     firmware/scripts/diagcoeur1.py [secondes]   relevé immédiat, puis un second après N s
#
# ***************************************************************************************
import os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ELF = os.environ.get("ELF", os.path.join(HERE, "..", "firmware.elf"))
OPENOCD = os.path.expanduser("~/.local/openocd-dev/bin/openocd")
PHASES = ["6502", "API", "DSPSync", "disque", "P0", "P1 (USB)", "P2 (catalogue)", "P3"]


def sym(name):
    out = subprocess.run(["arm-none-eabi-nm", ELF], capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        f = line.split()
        if len(f) == 3 and f[2] == name:
            return int(f[0], 16)
    sys.exit("diagcoeur1 : %s absent de %s (ce n'est pas la version de diagnostic ?)" % (name, ELF))


def lire(attente):
    onset, enc, late, fc = sym("diagLateOnset"), sym("diagEncMax"), sym("_ZL9lateTotal"), sym("frameCounter")
    q = 'echo "R [read_memory 0x%x 16 8] [read_memory 0x%x 32 1] [read_memory 0x%x 32 1] [read_memory 0x%x 16 1] [read_memory 0x40054028 32 1]"' % (onset, enc, late, fc)
    cmds = [q] + (["sleep %d" % (attente * 1000), q] if attente else [])
    r = subprocess.run([OPENOCD, "-f", "interface/cmsis-dap.cfg", "-c", "set USE_CORE 0", "-f", "target/rp2040.cfg",
                        "-c", "adapter speed 2000", "-c", "init"] + sum([["-c", c] for c in cmds], []) + ["-c", "shutdown"],
                       capture_output=True, text=True, timeout=120 + attente)
    lignes = [l for l in (r.stdout + r.stderr).splitlines() if l.startswith("R ")]
    if not lignes:
        sys.exit("diagcoeur1 : pas de réponse de la sonde\n" + r.stderr[-400:])
    for l in lignes:
        v = [int(x, 0) for x in l[2:].split()]
        print("t = %.1f s, trames %d, lateTotal %d, encodage max %d µs" % (v[11] / 1e6, v[10], v[9], v[8]))
        print("   lignes en retard (débuts) par phase du cœur 0 : " +
              ", ".join("%s %d" % (PHASES[i], v[i]) for i in range(8) if v[i]) if any(v[:8]) else
              "   aucune ligne en retard")


lire(int(sys.argv[1]) if len(sys.argv) > 1 else 0)
