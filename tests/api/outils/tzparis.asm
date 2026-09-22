; tzparis.asm — règle le fuseau Europe/Paris (1,24, sauvé en flash) puis 1,23 (heure du modem) et affiche 1,20.
; Modifier la chaîne zname pour un autre fuseau (America/Montreal, UTC-3:30...).
; Auteur : bmarty <bmarty@mailo.com>
NEO = 0
ptr2   = $F4
logLen = $1FFE

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $B00

start:
  stz logLen
  lda #$20
  sta logLen+1
  ldx #<szone
  ldy #>szone
  jsr print
  lda #<zname
  sta API_PARAMETERS
  lda #>zname
  sta API_PARAMETERS+1
  lda #24
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr
  ldx #<ssync
  ldy #>ssync
  jsr print
  lda #23
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr
  ldx #<sget
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
  jsr cr
halt:
  rts                       ; retour à l'appelant (sys du BASIC, ou NeoDOS pour un .NEO) ; reset sinon

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

szone:   .text "ZONE Europe/Paris -> err ", 0
ssync:   .text "SYNC modem -> err ", 0
sget:    .text "GET aaaa mm jj hh mm ss src ", 0
zname:   .ptext "Europe/Paris"
