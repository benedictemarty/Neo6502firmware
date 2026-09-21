#!/usr/bin/env python3
# fake_sntp_modem.py — modem factice minimal sur un pty (NEO_CDC_TTY de neo) : répond à AT (OK) et à
# AT+CIPSNTPTIME? par +CIPSNTPTIME:Tue Sep 15 12:34:56 2026 (T-25). Usage : fake_sntp_modem.py PTYFILE
import os, pty, sys, select
master, slave = pty.openpty()
open(sys.argv[1], "w").write(os.ttyname(slave))
buf = b""
while True:
    r, _, _ = select.select([master], [], [], 1.0)
    if not r: continue
    try: data = os.read(master, 256)
    except OSError: break
    if not data: break
    buf += data
    while b"\n" in buf or b"\r" in buf:
        i = min([j for j in (buf.find(b"\r"), buf.find(b"\n")) if j >= 0])
        line, buf = buf[:i].strip(), buf[i+1:]
        if not line: continue
        if line == b"AT+CIPSNTPTIME?": os.write(master, b"+CIPSNTPTIME:Tue Sep 15 12:34:56 2026\r\nOK\r\n")
        elif line == b"AT+CIPSNTPCFG?": os.write(master, b"+CIPSNTPCFG:1,0,\"pool.ntp.org\"\r\nOK\r\n")
        elif line.startswith(b"AT"): os.write(master, b"OK\r\n")
        else: os.write(master, b"ERROR\r\n")
