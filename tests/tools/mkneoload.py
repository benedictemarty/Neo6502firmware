# mkneoload.py — fabrique tests/api/neoload.bin (T-132 P1) : un .neo à trois blocs pour le test API neoload.
#   bloc 1 : $5000, 3 000 octets, (i*7+3) & 255, commentaire « un »        (bit de suite = 1)
#   bloc 2 : $FFFF (mémoire des objets), 1 000 octets, (i*13+1) & 255       (bit de suite = 1)
#   bloc 3 : $6000, 300 octets, i ^ $5A, commentaire de 40 caractères        (dernier bloc)
# Exécution $FFFF : chargé, pas lancé. Les sommes attendues sont aussi affichées.
# Auteur : bmarty <bmarty@mailo.com>
import sys
out = bytearray([0x03, ord('N'), ord('E'), ord('O'), 0, 0, 0xFF, 0xFF])
def bloc(suite, adr, data, commentaire):
    global out
    out += bytes([0x80 if suite else 0, adr & 0xFF, adr >> 8, len(data) & 0xFF, len(data) >> 8])
    out += commentaire.encode("ascii") + b"\0" + data
b1 = bytes((i * 7 + 3) & 255 for i in range(3000))
b2 = bytes((i * 13 + 1) & 255 for i in range(1000))
b3 = bytes((i ^ 0x5A) & 255 for i in range(300))
bloc(True, 0x5000, b1, "un")
bloc(True, 0xFFFF, b2, "")
bloc(False, 0x6000, b3, "commentaire de quarante caracteres ABCDE")
open(sys.argv[1] if len(sys.argv) > 1 else "tests/api/neoload.bin", "wb").write(out)
print("sommes : %04X %04X %04X" % (sum(b1) & 0xFFFF, sum(b2) & 0xFFFF, sum(b3) & 0xFFFF))
