; clocksync.asm — 1,23 Sync Clock From Modem (Trinity T-25) : modem factice sur la CDC (tests/tools/fake_sntp_modem.py)
; qui répond +CIPSNTPTIME:Tue Sep 15 12:34:56 2026. Sortie attendue (console) :
;   SYNC 00 GET 07EA 09 0F 0C 22 38 03 END   (année $07EA, mois, jour, heure, minute, seconde, source 3 = modem ;
;   02 dans neo, qui modélise un PCF8563 : la synchro l'écrit aussi et la RTC prime en lecture)
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

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
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
  ldx #<sget                ; GET : 1,20 -> année mois jour heure minute seconde source
  ldy #>sget
  jsr print
  lda #20
  ldx #1
  jsr api
  ldx #7
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
  cpx #8
  bne g2
  lda #' '
  jsr wchar
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  jmp halt

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

ssync:   .text "SYNC ", 0
sget:    .text "GET ", 0
sfin:    .text "END", 0
