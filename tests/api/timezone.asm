; timezone.asm — 1,24 Set Time Zone / 1,25 Get Time Zone (Trinity T-26), avec 1,23 (modem factice, heure UTC
; "Tue Sep 15 12:34:56 2026", tz 0) et 1,21/1,20 en heure locale. Sortie attendue (console) :
;   ZONE 00 Europe/Paris 0078 01 / SYNC 00 GET 07EA 09 0F 0E 22 38 / SET 00 GET 07EA 0C 19 0A 1E 00
;   ZONE 00 America/Montreal FED4 00 / GET 07EA 0C 19 04 1E 00 / BAD 01 / ZONE 00 UTC-3:30 FF2E 00 / ZONE 00 UTC 0000 00 / END
; (Paris en septembre : +120 min, été ; 1,21 25/12/2026 10:30 local -> relu identique ; Montréal le 25/12 : -300 min,
;  pas d'été, 09:30 UTC = 04:30 ; UTC-3:30 = -210 min sans été)
; Auteur : bmarty <bmarty@mailo.com>

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $B00
nbuf  = $B40

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  ldx #<zparis              ; ZONE Europe/Paris
  ldy #>zparis
  jsr setzone
  ldx #<ssync               ; SYNC : 1,23 -> 00
  ldy #>ssync
  jsr print
  lda #23
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  jsr get                   ; GET 07EA 09 0F 0E 22 38 (14:34:56 local)
  ldx #<sset                ; SET : 1,21 2026-12-25 10:30:00 local -> 00
  ldy #>sset
  jsr print
  lda #<2026
  sta API_PARAMETERS
  lda #>2026
  sta API_PARAMETERS+1
  lda #12
  sta API_PARAMETERS+2
  lda #25
  sta API_PARAMETERS+3
  lda #10
  sta API_PARAMETERS+4
  lda #30
  sta API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #21
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  jsr get                   ; GET 07EA 0C 19 0A 1E 00
  ldx #<zmtl                ; ZONE America/Montreal FED4 00 (décembre : pas d'été)
  ldy #>zmtl
  jsr setzone
  jsr get                   ; GET : 10:30 Paris hiver = 09:30 UTC = 04:30 Montréal
  ldx #<sbad                ; BAD : zone inconnue -> 01
  ldy #>sbad
  jsr print
  lda #<zbad
  sta API_PARAMETERS
  lda #>zbad
  sta API_PARAMETERS+1
  lda #24
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  ldx #<znum                ; ZONE UTC-3:30 FF2E 00
  ldy #>znum
  jsr setzone
  ldx #<zutc                ; back to UTC (settings file of the test storage)
  ldy #>zutc
  jsr setzone
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  jmp halt

; --- 1,24 nom (X,Y) puis 1,25 : "ZONE err nom offset dst"
setzone:
  stx API_PARAMETERS
  sty API_PARAMETERS+1
  lda #24
  ldx #1
  jsr api
  lda API_ERROR
  sta buf
  ldx #<szone
  ldy #>szone
  jsr print
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda #40
  sta nbuf
  lda #<nbuf
  sta API_PARAMETERS
  lda #>nbuf
  sta API_PARAMETERS+1
  lda #25
  ldx #1
  jsr api
  ldx #4
sz1:
  lda API_PARAMETERS,x
  sta buf,x
  dex
  bpl sz1
  ldx #0
sz2:
  cpx nbuf
  beq sz3
  inx
  lda nbuf,x
  phx
  jsr wchar
  plx
  bra sz2
sz3:
  lda #' '
  jsr wchar
  lda buf+3
  jsr hex
  lda buf+2
  jsr hex
  lda #' '
  jsr wchar
  lda buf+4
  jsr hex
  jmp cr

; --- 1,20 : "GET aaaa mm jj hh mm ss"
get:
  ldx #<sget
  ldy #>sget
  jsr print
  lda #20
  ldx #1
  jsr api
  ldx #6
g1:
  lda API_PARAMETERS,x
  sta buf,x
  dex
  bpl g1
  lda buf+1
  jsr hex
  lda buf
  jsr hex
  ldx #2
g2:
  lda #' '
  jsr wchar
  lda buf,x
  phx
  jsr hex
  plx
  inx
  cpx #7
  bne g2
  jmp cr

cr:
  lda #13
  jmp wchar

api:
  sta API_FUNCTION
  stx API_COMMAND
wait:
  lda API_COMMAND
  bne wait
  rts

wchar:
  pha                       ; journal en RAM $2000.. (longueur 16 bits en $1FFE) : tests/toolbox/run_neo.sh
  phy
  ldy logLen
  sty ptr2
  ldy logLen+1
  sty ptr2+1
  ldy #0
  sta (ptr2),y
  inc logLen
  bne wc1
  inc logLen+1
wc1:
  ply
  pla
  sta API_PARAMETERS
  lda #6
  sta API_FUNCTION
  lda #2
  sta API_COMMAND
  jmp wait

print:
  stx ptr
  sty ptr+1
  ldy #0
ploop:
  lda (ptr),y
  beq pdone
  phy
  jsr wchar
  ply
  iny
  bne ploop
pdone:
  rts

hex:
  pha
  lsr a
  lsr a
  lsr a
  lsr a
  jsr nibble
  pla
  and #15
nibble:
  cmp #10
  bcc digit
  adc #6
digit:
  adc #'0'
  jmp wchar

szone:   .text "ZONE ", 0
ssync:   .text "SYNC ", 0
sget:    .text "GET ", 0
sset:    .text "SET ", 0
sbad:    .text "BAD ", 0
sfin:    .text "END", 0
zparis:  .ptext "Europe/Paris"
zmtl:    .ptext "America/Montreal"
zbad:    .ptext "Mars/Olympus"
znum:    .ptext "UTC-3:30"
zutc:    .ptext "UTC"
