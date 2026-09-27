; latin14.asm — console du mode 1 : Latin-1 en glyphes MDA 8x14 (Trinity T-85).
; Mode 1 (5,9), écrit en (0,0) « é » ($E9, lettre MDA) et en (1,0) « ¤ » ($A4, sans modèle MDA : glyphe
; 8 lignes centré), relit les 14 lignes de chaque case par 5,33 ; puis redéfinit $E9 par 2,5 (motif $FC)
; et relit : un caractère redéfini garde son dessin 8 lignes.
; Sortie attendue : E9 + 14 octets / A4 + 14 octets / UDG + 14 octets / END (voir latin14.expected,
; E9 et A4 = table font_latin1_8x14.h).
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
px    = $F2                 ; X de la case (0 ou 9)
row   = $F3
acc   = $F6
BUF   = $0B00               ; 3 x 14 octets relus

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #1                    ; 5,9 : mode 1
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda #12                   ; CLS puis é ¤ en (0,0) (1,0), hors journal
  jsr out
  lda #$E9
  jsr out
  lda #$A4
  jsr out
  stz px
  ldx #0
  jsr grab
  lda #9
  sta px
  ldx #14
  jsr grab

  lda #$E9                  ; 2,5 : $E9 = 7 lignes $FC
  sta API_PARAMETERS
  ldx #7
  lda #$FC
d1:
  sta API_PARAMETERS,x
  dex
  bne d1
  lda #5
  ldx #2
  jsr api
  lda #12
  jsr out
  lda #$E9
  jsr out
  stz px
  ldx #28
  jsr grab

  ldx #<se9
  ldy #0
  jsr line
  ldx #<sa4
  ldy #14
  jsr line
  ldx #<sudg
  ldy #28
  jsr line
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- A -> console seulement (2,6)
out:
  sta API_PARAMETERS
  lda #6
  ldx #2
  jmp api

; --- 14 lignes de la case d'origine (px, 0) -> BUF+X
grab:
  stz row
g1:
  stz acc
  ldy #0
g2:
  tya                       ; 5,33 Read Pixel (px + y, row)
  clc
  adc px
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda row
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  phx
  phy
  lda #33
  ldx #5
  jsr api
  ply
  plx
  lda API_PARAMETERS
  cmp #1                    ; C = pixel allumé
  rol acc
  iny
  cpy #8
  bne g2
  lda acc
  sta BUF,x
  inx
  inc row
  lda row
  cmp #14
  bne g1
  rts

; --- étiquette (X bas, page de se9) puis 14 octets de BUF+Y
line:
  phy
  ldy #>se9
  jsr print
  ply
  lda #14
  sta row
l1:
  lda BUF,y
  phy
  jsr hex
  lda #' '
  jsr wchar
  ply
  iny
  dec row
  bne l1
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

se9:     .text "E9 ", 0
sa4:     .text "A4 ", 0
sudg:    .text "UDG ", 0
sfin:    .text "END", 0
