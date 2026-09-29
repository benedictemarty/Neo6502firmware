#!/usr/bin/env python3
# ***************************************************************************************
#
#      Name :      neotests.py
#      Author :    bmarty <bmarty@mailo.com>
#      Purpose :   T-98 : play the API tests on the board through SWD. `make tests-carte` builds
#                  them (NEO = 0 : they return to NeoDOS by RTS and leave their journal in the
#                  6502 RAM, length in $1FFE, text from $2000) ; the TESTS directory is copied to
#                  the key. For each test : clear the journal length, type the name at NeoDOS
#                  (keyboard queue), wait for the journal to stop growing, read it back from
#                  cpuMemory and compare it to NAME.expected (or NAME.expected.re, one regular
#                  expression per line), exactly as tests/toolbox/run_neo.sh does in neo.
#
#                  Core 0 only is attached (neopilot.sh rule), nothing is halted.
#
#      Usage :     firmware/scripts/neotests.py deposer         copy TESTS/*.NEO and *.bin to \TESTS on
#                                                               the key, through SWD (T-98, diagUpCmd),
#                                                               and read each one back to compare
#                  firmware/scripts/neotests.py [--cd] [nom ...]   play the tests (all of liste.txt)
#                  --cd first types CD \TESTS (the .bin files are opened by bare name).
#
# ***************************************************************************************
import os, re, subprocess, sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
ELF = os.environ.get("ELF", os.path.join(HERE, "..", "firmware.elf"))
SRC = os.environ.get("TESTS", os.path.expanduser("~/neo-carte/cle-usb/TESTS/src"))
OPENOCD = os.path.expanduser("~/.local/openocd-dev/bin/openocd")


def symbols():
    out = subprocess.run(["arm-none-eabi-nm", "-S", ELF], capture_output=True, text=True, check=True).stdout
    sym = {}
    for line in out.splitlines():
        f = line.split()
        if len(f) == 4:
            sym.setdefault(f[3], []).append((int(f[0], 16), int(f[1], 16)))
    queue = [a for a, s in sym["_ZL5queue"] if s == 0x41][0]                     # keyboard.cpp : MAX_QUEUE_SIZE + 1
    return queue, sym["_ZL9queueTail"][0][0], sym["cpuMemory"][0][0], {k: sym[k][0][0] for k in
                                                                        ("diagUpCmd", "diagUpLen", "diagUpStatus", "diagUpBuffer")}


QUEUE, TAIL, CPU, UP = symbols()
CHUNK = 8192                                                                    # DIAG_UP_CHUNK, fileimplementation.cpp


def openocd(tcl):
    with tempfile.NamedTemporaryFile("w", suffix=".tcl", delete=False) as f:
        f.write("init\n" + tcl + "\nshutdown\n")
    r = subprocess.run([OPENOCD, "-f", "interface/cmsis-dap.cfg", "-c", "set USE_CORE 0", "-f", "target/rp2040.cfg",
                        "-c", "adapter speed 2000", "-f", f.name], capture_output=True, text=True, timeout=180)
    os.unlink(f.name)
    return r.stdout + r.stderr


def typing(text):
    """Tcl that types text then Return into the firmware keyboard queue, 40 ms per key."""
    lines = ["proc touche {c} { set t [read_memory %d 8 1]; write_memory [expr {%d + $t}] 8 [list $c];"
             " write_memory %d 8 [list [expr {($t + 1) & 63}]]; sleep 40 }" % (TAIL, QUEUE, TAIL)]
    lines += ["touche %d" % ord(c) for c in text] + ["touche 13"]
    return "\n".join(lines)


def run(short, name, wait):
    base = os.path.join(SRC, name)
    dump = tempfile.mktemp(suffix=".bin")
    tcl = [typing("CLS"), "sleep 500",
           "write_memory %d 8 {0 0}" % (CPU + 0x1FFE),
           typing(short),
           # The journal is complete when its length has not moved for 1.5 s (and is not empty) : at most `wait` s.
           "set p -1; set n 0; set t0 [clock milliseconds]",
           "while {[clock milliseconds] - $t0 < %d} {" % (wait * 1000),
           "  set l [read_memory %d 16 1]" % (CPU + 0x1FFE),
           "  if {$l == $p && $l >= 0x2000} { incr n; if {$n >= 6} break } else { set n 0 }",
           "  set p $l; sleep 250 }",
           "set l [read_memory %d 16 1]; echo \"LONGUEUR $l\"" % (CPU + 0x1FFE),
           "if {$l > 0x2000} { dump_image %s [expr {%d + 0x2000}] [expr {$l - 0x2000}] }" % (dump, CPU)]
    log = openocd("\n".join(tcl))
    if "Error" in log and "Polling failed" not in log:
        return False, "OpenOCD : " + " / ".join(l for l in log.splitlines() if "Error" in l)
    got = ""
    if os.path.exists(dump):
        got = open(dump, "rb").read().decode("latin-1").replace("\x0c", "").replace("\r", "\n")
        os.unlink(dump)
    lines = got.splitlines()
    if os.path.exists(base + ".carte.expected.re") or os.path.exists(base + ".expected.re"):  # the board's own first
        exp = open(base + (".carte.expected.re" if os.path.exists(base + ".carte.expected.re") else ".expected.re")).read().splitlines()
        ok = len(exp) == len(lines) and all(re.fullmatch(e, g) for e, g in zip(exp, lines))
    else:
        exp = open(base + ".expected").read().splitlines()
        ok = exp == lines
    return ok, "" if ok else "attendu %s\n    obtenu  %s" % (exp, lines)


def same_firmware():
    """The ELF must be the firmware of the board, or every address is wrong (incident of 2026-09-30 :
    an ELF rebuilt in the tree sent a command into PicoDVI's dma_irq_privdata). Compares the
    banner string, in .rodata, and 256 bytes of FISDebugUploadPoll, both read from the board's flash."""
    import shutil
    tmp = tempfile.mkdtemp()
    subprocess.run(["arm-none-eabi-objcopy", "-O", "binary", "--only-section=.rodata", ELF, tmp + "/ro.bin"], check=True)
    subprocess.run(["arm-none-eabi-objcopy", "-O", "binary", "--only-section=.text", ELF, tmp + "/tx.bin"], check=True)
    head = subprocess.run(["arm-none-eabi-objdump", "-h", ELF], capture_output=True, text=True).stdout.split()
    ro = int(head[head.index(".rodata") + 2], 16)
    tx = int(head[head.index(".text") + 2], 16)
    rodata, text = open(tmp + "/ro.bin", "rb").read(), open(tmp + "/tx.bin", "rb").read()
    k = rodata.find(b"Trinity Firmware: v")
    out = subprocess.run(["arm-none-eabi-nm", ELF], capture_output=True, text=True).stdout
    fn = [int(l.split()[0], 16) for l in out.splitlines() if l.endswith(" _Z18FISDebugUploadPollv")][0] & ~1
    want = [(ro + k, rodata[k:k + 32]), (fn, text[fn - tx:fn - tx + 256])]
    tcl = "\n".join("dump_image %s/b%d.bin 0x%08x %d" % (tmp, i, a, len(b)) for i, (a, b) in enumerate(want))
    openocd(tcl)
    ok = all(os.path.exists("%s/b%d.bin" % (tmp, i)) and open("%s/b%d.bin" % (tmp, i), "rb").read() == b
             for i, (a, b) in enumerate(want))
    shutil.rmtree(tmp)
    if not ok:
        sys.exit("neotests : %s n'est pas le firmware de la carte (bannière %s) — rien n'est écrit"
                 % (ELF, rodata[k:k + 30].split(b"\r")[0].decode()))


def up_tcl():
    """Tcl helpers for the T-98 commands : cmd N waits for the firmware, fails on a FatFs error."""
    return "\n".join([
        "proc cmd {n} { mww %d $n; set k 0; while {[read_memory %d 32 1] != 0} { sleep 10; incr k;"
        " if {$k > 1000} { error \"délai : commande $n\" } }; set e [read_memory %d 32 1];"
        " if {$e != 0} { error \"commande $n : FatFs $e\" } }" % (UP["diagUpCmd"], UP["diagUpCmd"], UP["diagUpStatus"]),
        "proc nom {buf texte} { set o {}; foreach c [split $texte \"\"] { lappend o [scan $c %c] }; lappend o 0;"
        " write_memory $buf 8 $o }",
        "cmd 7", "set buf [read_memory %d 32 1]" % UP["diagUpBuffer"]])


def deposer():
    here = os.path.dirname(SRC)
    files = sorted(f for f in os.listdir(here) if f.endswith(".NEO") or f.endswith(".bin"))
    tmp = tempfile.mkdtemp()
    tcl = [up_tcl(), "nom $buf /TESTS", "cmd 6"]
    for k, f in enumerate(files):
        data = open(os.path.join(here, f), "rb").read()
        tcl += ["nom $buf /TESTS/%s" % f, "cmd 1"]
        for i in range(0, len(data), CHUNK):
            part = os.path.join(tmp, "f%d_%d.bin" % (k, i))                   # one name per file and chunk
            open(part, "wb").write(data[i:i + CHUNK])
            tcl += ["load_image %s $buf bin" % part, "mww %d %d" % (UP["diagUpLen"], len(data[i:i + CHUNK])), "cmd 2"]
        tcl += ["cmd 3"]
        for i in range(0, len(data) + 1, CHUNK):                                # read back, chunk by chunk
            tcl += ["nom $buf /TESTS/%s" % f, "mww %d %d" % (UP["diagUpLen"], i), "cmd 4",
                    "set n [read_memory %d 32 1]" % UP["diagUpLen"],
                    "if {$n > 0} { dump_image %s $buf $n }" % os.path.join(tmp, "r%d_%d.bin" % (k, i))]
        tcl += ["echo \"déposé %s\"" % f]
    tcl += ["cmd 5"]
    t0 = time.time()
    log = openocd("\n".join(tcl))
    bad = 0
    for k, f in enumerate(files):
        data = open(os.path.join(here, f), "rb").read()
        back = b"".join(open(os.path.join(tmp, "r%d_%d.bin" % (k, i)), "rb").read()
                        for i in range(0, len(data) + 1, CHUNK) if os.path.exists(os.path.join(tmp, "r%d_%d.bin" % (k, i))))
        ok = back == data
        bad += not ok
        print(("relu OK : " if ok else "DIFFÉRENT : ") + "%s (%d octets)" % (f, len(data)))
    errors = [l for l in log.splitlines() if "rror" in l and "Polling" not in l]
    if errors:
        print("\n".join(errors))
    print("%d fichier(s) en %.0f s, %d différent(s)" % (len(files), time.time() - t0, bad))
    sys.exit(1 if bad or errors else 0)


def main():
    args = sys.argv[1:]
    same_firmware()
    if args[:1] == ["deposer"]:
        deposer()
    cd = "--cd" in args
    args = [a for a in args if a != "--cd"]
    table = [l.split() for l in open(os.path.join(SRC, "liste.txt")) if l.strip()]
    if args:
        table = [t for t in table if t[1] in args or t[0] in args]
    if cd:
        openocd(typing("CD \\TESTS") + "\nsleep 1000")
    bad = 0
    for short, name in table:
        ok, why = run(short, name, int(os.environ.get("ATTENTE", "20")))
        print(("OK : " if ok else "ÉCHEC : ") + name + ("" if ok else "\n    " + why), flush=True)
        bad += not ok
    print("%d/%d OK" % (len(table) - bad, len(table)))
    sys.exit(1 if bad else 0)


main()
