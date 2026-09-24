#!/usr/bin/env python3
# ***************************************************************************************
#
#      Name :      neoterm.py
#      Author :    bmarty <bmarty@mailo.com>
#      Purpose :   Terminal and capture for the Trinity debug port (T-73/T-74), on the
#                  UEXT UART through a USB-serial adapter (CP2104 on /dev/ttyUSB0).
#
#                  capture [fichier]   lit en continu et horodate chaque ligne ; c'est la
#                                      forme utile quand la carte plante, puisque le
#                                      fichier survit à l'écran
#                  send "texte"        envoie du texte (\n = Entrée, ! = commande firmware)
#                  watch [secondes]    capture pendant N secondes puis affiche
#
#      Interactif : picocom -b 115200 /dev/ttyUSB0   (Ctrl-A Ctrl-X pour sortir)
#
#      Commandes du firmware : !s famines, !m mémoire, !z remise à zéro, !! un vrai '!'
#
# ***************************************************************************************
import sys, time, serial

PORT = "/dev/ttyUSB0"
BAUD = 115200


def open_port():
    return serial.Serial(PORT, BAUD, timeout=0.2)


def capture(path=None, seconds=None):
    out = open(path, "a", buffering=1) if path else None
    started = time.time()
    line = b""
    with open_port() as port:
        while seconds is None or time.time() - started < seconds:
            chunk = port.read(4096)
            if not chunk:
                continue
            line += chunk
            while b"\n" in line:
                one, line = line.split(b"\n", 1)
                text = "%7.2f  %s" % (time.time() - started, one.decode("latin-1").rstrip("\r"))
                print(text, flush=True)
                if out:
                    out.write(text + "\n")


def send(text):
    with open_port() as port:
        port.write(text.replace("\\n", "\r").encode("latin-1"))
        port.flush()
        time.sleep(0.5)
        answer = port.read(8192)
        if answer:
            print(answer.decode("latin-1"), end="")


if __name__ == "__main__":
    what = sys.argv[1] if len(sys.argv) > 1 else "capture"
    if what == "send":
        send(sys.argv[2])
    elif what == "watch":
        capture(None, float(sys.argv[2]) if len(sys.argv) > 2 else 10.0)
    else:
        capture(sys.argv[2] if len(sys.argv) > 2 else None)
