; blittrad.asm — T-115 (demande Neo6502AigleDor) : 12,3 actions 3 (translate) et 4 (translatemasked).
; Table de 256 octets : table[v] = $FF - v, en $3400 (RAM 6502) et copiée en $90:$7F00 (RAM graphique).
; Cibles en RAM du 6502 ($3100.., remplie de $55). Chaque ligne : « Tn erreur o0 o1 o2 o3 o4 ».
;   T1 translate, octets $00 $01 $10 $FE -> octets               -> 00 FF FE EF 01 55
;   T2 translatemasked (transparent $01), idem                  -> 00 FF 55 EF 01 55
;   T3 translate, quartets $12 $34 -> quartet bas               -> 00 5E 5D 5C 5B 55
;   T4 translate, quartets $12 -> quartet haut                  -> 00 E5 D5 55 55 55
;   T5 translate, bits $A0 -> octets                            -> 00 FE FF FE FF FF
;   T6 translatemasked (transparent 0), quartets $10 $02 -> format 5 -> 00 E5 5D 55 55 55
;   T7 comme T1, table en $90:$7F00                             -> 00 FF FE EF 01 55
;   T8 table en $90:$7F01 (déborde d'un octet)                  -> 01 55 55 55 55 55
;   T9 cible au format 1 (source seulement)                     -> 01 55 55 55 55 55
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
cas   = $F2                 ; pointeur dans la table des cas
num   = $F6
err   = $F7
SRC   = $3000               ; zones du blitter (12 octets chacune)
TGT   = $300C
DATA  = $3040               ; données source
CIBLE = $3100               ; cibles, remplies de $55
TABLE = $3400               ; table de traduction

start:
  stz logLen
  lda #$20
  sta logLen+1
  ldx #0                    ; table[v] = $FF - v
t1:
  txa
  eor #$FF
  sta TABLE,x
  inx
  bne t1
  ldx #0                    ; données source
d1:
  lda donnees,x
  sta DATA,x
  inx
  cpx #10
  bne d1
  ldx #0                    ; cibles = $55
  lda #$55
d2:
  sta CIBLE,x
  inx
  cpx #$50
  bne d2
  stz API_PARAMETERS        ; table -> $90:$7F00 (12,2, 256 octets)
  lda #<TABLE
  sta API_PARAMETERS+1
  lda #>TABLE
  sta API_PARAMETERS+2
  lda #$90
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #$7F
  sta API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #1
  sta API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  lda #<cases
  sta cas
  lda #>cases
  sta cas+1
  lda #1
  sta num

boucle:
  ldy #0
  lda (cas),y               ; action, $FF = fin
  cmp #$FF
  bne b1
  jmp fin
b1:
  sta API_PARAMETERS
  iny
z1:                         ; zones : 12 + 12 octets
  lda (cas),y
  sta SRC-1,y
  iny
  cpy #25
  bne z1
  lda #<SRC
  sta API_PARAMETERS+1
  lda #>SRC
  sta API_PARAMETERS+2
  lda #<TGT
  sta API_PARAMETERS+3
  lda #>TGT
  sta API_PARAMETERS+4
  lda (cas),y               ; table : adresse et page
  sta API_PARAMETERS+5
  iny
  lda (cas),y
  sta API_PARAMETERS+6
  iny
  lda (cas),y
  sta API_PARAMETERS+7
  lda #3
  ldx #12
  jsr api
  lda API_ERROR
  sta err
  lda #'T'
  jsr wchar
  lda num
  jsr nib
  lda #' '
  jsr wchar
  lda err
  jsr hex
  ldy #28                   ; adresse à relire
  lda (cas),y
  sta ptr
  iny
  lda (cas),y
  sta ptr+1
  ldy #0
r1:
  lda #' '
  jsr wchar
  lda (ptr),y
  phy
  jsr hex
  ply
  iny
  cpy #5
  bne r1
  jsr cr
  inc num
  clc
  lda cas
  adc #30
  sta cas
  bcc b2
  inc cas+1
b2:
  jmp boucle

fin:
  ldx #<sfin
  ldy #>sfin
  jsr print
.if NEO
  jmp $FFFF
.endif
  rts

; données : $3040 $00 $01 $10 $FE ; $3044 $12 $34 ; $3046 $A0 ; $3048 $10 $02
donnees: .byte $00, $01, $10, $FE, $12, $34, $A0, 0, $10, $02

; cas : action, zone source (adresse, page, pad, pas, format, transparent, plein, hauteur, largeur),
;       zone cible (adresse, page, pad, pas, format, 5 octets ignorés), table (adresse, page), adresse relue
cases:
  .byte 3, $40,$30,0,0, 4,0, 0,0,0,   1, 4,0,   $00,$31,0,0, 8,0, 0,0,0,0,0,0,  <TABLE,>TABLE,0
  .word CIBLE+$00
  .byte 4, $40,$30,0,0, 4,0, 0,1,0,   1, 4,0,   $08,$31,0,0, 8,0, 0,0,0,0,0,0,  <TABLE,>TABLE,0
  .word CIBLE+$08
  .byte 3, $44,$30,0,0, 2,0, 1,0,0,   1, 4,0,   $10,$31,0,0, 8,0, 4,0,0,0,0,0,  <TABLE,>TABLE,0
  .word CIBLE+$10
  .byte 3, $44,$30,0,0, 1,0, 1,0,0,   1, 2,0,   $18,$31,0,0, 8,0, 3,0,0,0,0,0,  <TABLE,>TABLE,0
  .word CIBLE+$18
  .byte 3, $46,$30,0,0, 1,0, 2,0,0,   1, 8,0,   $20,$31,0,0, 8,0, 0,0,0,0,0,0,  <TABLE,>TABLE,0
  .word CIBLE+$20
  .byte 4, $48,$30,0,0, 2,0, 1,0,0,   1, 4,0,   $30,$31,0,0, 8,0, 5,0,0,0,0,0,  <TABLE,>TABLE,0
  .word CIBLE+$30
  .byte 3, $40,$30,0,0, 4,0, 0,0,0,   1, 4,0,   $38,$31,0,0, 8,0, 0,0,0,0,0,0,  $00,$7F,$90
  .word CIBLE+$38
  .byte 3, $40,$30,0,0, 4,0, 0,0,0,   1, 4,0,   $40,$31,0,0, 8,0, 0,0,0,0,0,0,  $01,$7F,$90
  .word CIBLE+$40
  .byte 3, $40,$30,0,0, 4,0, 0,0,0,   1, 4,0,   $48,$31,0,0, 8,0, 1,0,0,0,0,0,  <TABLE,>TABLE,0
  .word CIBLE+$48
  .byte $FF

sfin:   .text "END", 0

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
  jsr nib
  pla
  and #15
nib:
  cmp #10
  bcc dg
  adc #6
dg:
  adc #'0'
  jmp wchar
