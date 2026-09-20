#!/usr/bin/env python3
# mkres.py — construit un fichier de ressources NR1 pour le Resource Manager (groupe 38, F-46 du fork).
#   mkres.py sortie.res TYPE:ID:fichier [TYPE:ID:fichier ...]
#   TYPE = 4 caractères ASCII au plus (FONT, TEXT, ICON, MENU, DLOG… complétés par des espaces), ID = 0..65535.
# Format (petit-boutiste) : en-tête 8 octets 'N','R',1,nombre,0,0,0,0 ; puis nombre × 16 octets :
#   type[4], id u16, décalage u32 (depuis le début du fichier), taille u32, 2 réservés ; puis les données.
# SPDX-License-Identifier: EUPL-1.2 — bmarty <bmarty@mailo.com>
import struct,sys

def main(argv):
    if len(argv) < 3:
        sys.exit("usage : mkres.py sortie.res TYPE:ID:fichier ...")
    entries = []
    for spec in argv[2:]:
        rtype,rid,path = spec.split(":",2)
        rtype = rtype.encode("ascii")
        if len(rtype) > 4 or len(rtype) == 0: sys.exit("type invalide : " + spec)
        entries.append((rtype.ljust(4),int(rid,0),open(path,"rb").read()))
    if len(entries) > 255: sys.exit("255 ressources au plus")
    offset = 8 + 16 * len(entries)
    table = b""
    data = b""
    for rtype,rid,blob in entries:
        table += struct.pack("<4sHIIH",rtype,rid,offset + len(data),len(blob),0)
        data += blob
    with open(argv[1],"wb") as f:
        f.write(b"NR" + bytes([1,len(entries),0,0,0,0]) + table + data)
    print("{0} : {1} ressource(s), {2} octets".format(argv[1],len(entries),offset + len(data)))

if __name__ == "__main__": main(sys.argv)
