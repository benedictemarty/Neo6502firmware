; sprimg.asm — T-113 (demandes Neo6502AigleDor et Neo6502Bagman) : 6,7 Sprite Set Image.
; RAM graphique : en-tête 01 00 01 00 et image classique n° 0 (16 x 16, couleur 1) en $0100 ;
;   $0400 image 5 x 3, 8 bits, valeur 3 partout          $0500 image 4 x 1, 2 bits, $6C (1 2 3 0)
;   $0510 table 00 05 06 07                              $0520 image 4 x 1, 1 bit, $A0 (1 0 1 0)
;   $0530 table 00 09                                    $0540 image 3 x 1, 8 bits, $10 $21 $00
; Chaque ligne : « lettre erreur » puis les 6 octets de la VRAM à partir de (100,y).
;   A sprite 0 = $0400 5x3 8 bits, en (100,196)           -> 00 30 30 30 30 30 00
;   B sprite 1 = $0500 2 bits + table $0510, (100,200)    -> 00 50 60 70 00 00 00
;   C sprite 2 = $0520 1 bit + table $0530, (100,202)     -> 00 90 00 90 00 00 00
;   D sprite 3 = $0540 8 bits sans table, (100,204)       -> 00 00 10 00 00 00 00 ($10 -> 0 : transparent)
;     (sprites espacés : le sprite 0 fait 3 lignes, 196 à 198)
;   E 6,7 avec 3 bits par pixel (ligne 196)               -> 01 30 30 30 30 30 00
;   F 6,7 image en $7FFF, 2 x 1 en 8 bits (déborde)       -> 02 30 30 30 30 30 00
;   G 6,2 sprite 1 retourné en x (ligne 200)              -> 00 00 70 60 50 00 00
;   H 6,7 sprite 0 = $0500 2 bits + table : même ancre    -> 00 50 60 70 00 00 00
;   I 6,2 sprite 0 image n° 0 (retour aux images classiques) -> 00 10 10 10 10 10 10
;   J 6,6 mode 1 (opaque), ligne 200 : sprite 0 devant    -> 00 10 10 10 10 10 10
;   K 6,3 sprite 0, ligne 200 : sprite 1 redessiné        -> 00 00 70 60 50 00 00
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
pas   = $F2                 ; pointeur dans la liste des étapes
err   = $F7
LU    = $3200               ; 6 octets relus dans la VRAM
GFX   = $3300               ; image de la RAM graphique ($90:0000..$05FF) préparée ici

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12                   ; écran effacé : la console reste en haut
  jsr wchar
  ldx #0                    ; image de la RAM graphique, page par page
g0:
  stz GFX,x
  stz GFX+$100,x
  stz GFX+$400,x
  stz GFX+$500,x
  lda #$11
  sta GFX+$100,x
  inx
  bne g0
  ldx #0
g1:
  lda entete,x
  sta GFX,x
  inx
  cpx #4
  bne g1
  ldx #0
  lda #3
g2:
  sta GFX+$400,x
  inx
  cpx #15
  bne g2
  ldx #0
g3:
  lda page5,x
  sta GFX+$500,x
  inx
  cpx #$43
  bne g3
  stz API_PARAMETERS        ; GFX -> $90:0000, $0600 octets
  lda #<GFX
  sta API_PARAMETERS+1
  lda #>GFX
  sta API_PARAMETERS+2
  lda #$90
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  stz API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #6
  sta API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  lda #<etapes
  sta pas
  lda #>etapes
  sta pas+1

; étape : fonction du groupe 6 puis 8 paramètres ; fonction 0 = ligne (lettre, adresse VRAM) ; $FF = fin
boucle:
  lda (pas)
  cmp #$FF
  bne b0
  jmp fin
b0:
  cmp #0
  bne b1
  jmp relire
b1:
  ldy #1
a1:
  lda (pas),y
  sta API_PARAMETERS-1,y
  iny
  cpy #9
  bne a1
  lda (pas)
  ldx #6
  jsr api
  lda API_ERROR
  sta err
  bra suivant
relire:
  ldy #1
  lda (pas),y
  jsr wchar
  lda #$80                  ; 6 octets de la VRAM -> LU
  sta API_PARAMETERS
  iny
  lda (pas),y
  sta API_PARAMETERS+1
  iny
  lda (pas),y
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<LU
  sta API_PARAMETERS+4
  lda #>LU
  sta API_PARAMETERS+5
  lda #6
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  lda #' '
  jsr wchar
  lda err
  jsr hex
  ldx #0
r1:
  lda #' '
  jsr wchar
  lda LU,x
  phx
  jsr hex
  plx
  inx
  cpx #6
  bne r1
  jsr cr
  stz err
suivant:
  clc
  lda pas
  adc #9
  sta pas
  bcc b2
  inc pas+1
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

entete: .byte 1, 0, 1, 0
; $0500.. : image 2 bits, puis à $10 sa table, à $20 l'image 1 bit, à $30 sa table, à $40 l'image 8 bits
page5:  .byte $6C, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0, 5, 6, 7, 0,0,0,0,0,0,0,0,0,0,0,0
        .byte $A0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0, 9, 0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte $10, $21, $00

Y196 = 196*320+100
Y200 = 200*320+100
Y202 = 202*320+100
Y204 = 204*320+100

etapes:
  .byte 7, 0, $00,$04, 5,3, 8, $FF,$FF             ; A
  .byte 2, 0, <100,>100, <196,>196, $80, 0, 7
  .byte 0, 'A', <Y196, >Y196, 0,0,0,0,0
  .byte 7, 1, $00,$05, 4,1, 2, $10,$05             ; B
  .byte 2, 1, <100,>100, <200,>200, $80, 0, 7
  .byte 0, 'B', <Y200, >Y200, 0,0,0,0,0
  .byte 7, 2, $20,$05, 4,1, 1, $30,$05             ; C
  .byte 2, 2, <100,>100, <202,>202, $80, 0, 7
  .byte 0, 'C', <Y202, >Y202, 0,0,0,0,0
  .byte 7, 3, $40,$05, 3,1, 8, $FF,$FF             ; D
  .byte 2, 3, <100,>100, <204,>204, $80, 0, 7
  .byte 0, 'D', <Y204, >Y204, 0,0,0,0,0
  .byte 7, 0, $00,$04, 5,3, 3, $FF,$FF             ; E
  .byte 0, 'E', <Y196, >Y196, 0,0,0,0,0
  .byte 7, 0, $FF,$7F, 2,1, 8, $FF,$FF             ; F
  .byte 0, 'F', <Y196, >Y196, 0,0,0,0,0
  .byte 2, 1, $00,$80, 0,0, $80, 1, $80            ; G
  .byte 0, 'G', <Y200, >Y200, 0,0,0,0,0
  .byte 7, 0, $00,$05, 4,1, 2, $10,$05             ; H
  .byte 0, 'H', <Y196, >Y196, 0,0,0,0,0
  .byte 2, 0, $00,$80, 0,0, 0, $80, $80            ; I
  .byte 0, 'I', <Y196, >Y196, 0,0,0,0,0
  .byte 6, 1, 0,0,0,0,0,0,0                        ; J
  .byte 0, 'J', <Y200, >Y200, 0,0,0,0,0
  .byte 3, 0, 0,0,0,0,0,0,0                        ; K
  .byte 0, 'K', <Y200, >Y200, 0,0,0,0,0
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
