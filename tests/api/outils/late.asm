; late.asm — 5,40 Get Late Scanlines (Trinity T-32c) : affiche le compteur de lignes DVI non encodées
; à temps (traits colorés sur carte). Tape des touches pendant qu'il tourne : le compteur monte si le
; second cœur est affamé. Échap (ou Ctrl+C) : sortie. Auteur : bmarty <bmarty@mailo.com>
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
loop:
  lda #40                   ; 5,40 -> P0-3 compteur 32 bits
  ldx #5
  jsr api
  ldx #3
c1:
  lda API_PARAMETERS,x
  sta buf,x
  dex
  bpl c1
  ldx #<slate
  ldy #>slate
  jsr print
  lda buf+3
  jsr hex
  lda buf+2
  jsr hex
  lda buf+1
  jsr hex
  lda buf
  jsr hex
  jsr cr
  ldx #50                   ; ~0,5 s
w1:
  lda #1                    ; 1,1 timer
  ldx #1
  jsr api
  dex
  bne w1
  lda #1                    ; 2,2 : touche disponible ?
  ldx #2
  jsr api
  lda API_PARAMETERS
  beq loop
  cmp #27
  bne loop
halt:
  rts                       ; retour à l'appelant

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

slate:   .text "LATE ", 0
