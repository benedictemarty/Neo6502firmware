; late.asm — 5,40 Get Late Scanlines (Trinity T-32c) : compteur des lignes DVI non encodées à temps
; (traits colorés sur carte). Tape des touches pendant qu'il tourne : le compteur monte si le second
; coeur est affamé. Une touche quelconque : sortie. Outil carte (pas de journal RAM).
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $B00
count = $B10

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
  lda #30                   ; ~0,3 s : 30 lectures du timer espacées
  sta count
w1:
  lda #1                    ; 1,1 Timer -> P0-3 (centièmes)
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta buf+4
w2:
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  cmp buf+4
  beq w2                    ; attend le centième suivant
  dec count
  bne w1
  lda #1                    ; 2,1 Read Character : 0 = rien
  ldx #2
  jsr api
  lda API_PARAMETERS
  beq loop
  rts                       ; une touche : retour à l'appelant

api:
  sta API_FUNCTION
  stx API_COMMAND
wait:
  lda API_COMMAND
  bne wait
  rts

cr:
  lda #13
  ; tombe dans wchar

wchar:
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
