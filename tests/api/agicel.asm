; agicel.asm — T-109 : groupe 39, cels AGI (39,5 Draw Cel, 39,6 Restore Rect).
; Image synthétique de 25 octets (aucune donnée de jeu) qui ne pose que des priorités : colonne x = 50 en
; priorité 9 ; point (60,100) en ligne de contrôle 1 avec la priorité 6 juste en dessous ; colonne x = 70 en
; ligne de contrôle 3 de y = 160 jusqu'au bas. Visuel blanc (15) partout ; plan affiché en ligne 8 (39,2).
; Chaque cas « Xn ee vv » : erreur de 39,5 (ou 39,6), puis pixel écran relu par 5,33 (-- si non relu).
;   C1 cel A (1 pixel de couleur 5), priorité 5 sur le fond (4)                          -> 00 05
;   C2 priorité 8 sur la colonne de priorité 9                                          -> 00 0F
;   C3 priorité 9 sur la colonne de priorité 9 (hypothèse « >= »)                        -> 00 05
;   C4 relecture du pixel écran impair de C1 (pixels doublés)                            -> 00 05
;   C5 priorité 5 sur la ligne de contrôle (60,100) : 6 trouvé dessous                   -> 00 0F
;   C6 priorité 6 au même point                                                          -> 00 05
;   C7 priorité 14 sur la colonne de contrôle jusqu'au bas (hypothèse : 15)              -> 00 0F
;   C8 priorité 15 au même endroit                                                       -> 00 05
;   M1-M4 cel M (couleurs A puis B) sans miroir puis en miroir                           -> A B / B A
;   W1-W2 cel W (1, 2, 3) coupé au bord droit en x = 158                                 -> 01 02
;   R1 39,6 sur le pixel de C1                                                           -> 00 0F
;   E1 cel de largeur 0 ; E2 en-tête en $FFFE (hors RAM) ; E3 cel en $FFF7 dont les lignes sont coupées par la
;   fin de la RAM ; E4 y = 168 ; E5 39,6 de largeur 0 ; E6 plan en ligne 100 (déborde de l'écran)
;                                                                         -> 01, 01, 02, 01, 01, 01
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
cas   = $F2
num   = $F8
err   = $F9
val   = $FA
sauve = $3000               ; 3 octets de $FFF7 sauvegardés pendant E3

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #<image               ; 39,1 : décodage avec effacement
  sta API_PARAMETERS
  lda #>image
  sta API_PARAMETERS+1
  lda #<(fimage-image)
  sta API_PARAMETERS+2
  lda #>(fimage-image)
  sta API_PARAMETERS+3
  lda #1
  sta API_PARAMETERS+4
  lda #1
  jsr agi
  lda #8                    ; 39,2 : affichage en ligne 8
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #2
  jsr agi
  lda #<cases
  sta cas
  lda #>cases
  sta cas+1

boucle:                     ; cas : lettre, chiffre, cel (2), x, y, priorité, drapeaux, X écran (2), Y écran (2)
  ldy #0
  lda (cas),y
  beq fin_cas
  ldy #2
c1:
  lda (cas),y
  sta API_PARAMETERS-2,y    ; cel, x, y, priorité, drapeaux -> P0..P5
  iny
  cpy #8
  bne c1
  lda #8                    ; plan en ligne 8
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #5
  jsr agi
  lda API_ERROR
  sta err
  jsr relire
  jsr ligne_cas
  clc
  lda cas
  adc #12
  sta cas
  bcc boucle
  inc cas+1
  bra boucle

fin_cas:
; R1 : 39,6 sur (10,10,1,1), puis relecture de (20,18)
  lda #10
  sta API_PARAMETERS
  sta API_PARAMETERS+1
  lda #1
  sta API_PARAMETERS+2
  sta API_PARAMETERS+3
  lda #8
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #6
  jsr agi
  lda API_ERROR
  sta err
  lda #<r1
  sta cas
  lda #>r1
  sta cas+1
  jsr relire
  jsr ligne_cas

; E1, E2, E4, E6 : 39,5 en erreur (table) ; E3 : cel en $FFF7 ; E5 : 39,6 de largeur 0
  lda #<erreurs
  sta cas
  lda #>erreurs
  sta cas+1
e_boucle:
  ldy #0
  lda (cas),y
  beq e3
  ldy #2
e1:
  lda (cas),y
  sta API_PARAMETERS-2,y
  iny
  cpy #10
  bne e1
  lda #5
  jsr agi
  lda API_ERROR
  sta err
  lda #$FF
  sta val
  jsr ligne_err
  clc
  lda cas
  adc #10
  sta cas
  bcc e_boucle
  inc cas+1
  bra e_boucle

e3:                         ; cel en $FFF7 : largeur 1, hauteur 200 ; les lignes atteignent la fin de la RAM
  ldx #2
s1:
  lda $FFF7,x
  sta sauve,x
  lda e3cel,x
  sta $FFF7,x
  dex
  bpl s1
  lda #$F7
  sta API_PARAMETERS
  lda #$FF
  sta API_PARAMETERS+1
  lda #10
  sta API_PARAMETERS+2
  sta API_PARAMETERS+3
  lda #15
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #8
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #5
  jsr agi
  lda API_ERROR
  sta err
  ldx #2
s2:
  lda sauve,x
  sta $FFF7,x
  dex
  bpl s2
  lda #<e3nom
  sta cas
  lda #>e3nom
  sta cas+1
  lda #$FF
  sta val
  jsr ligne_err

  stz API_PARAMETERS        ; E5 : 39,6 de largeur 0
  stz API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #1
  sta API_PARAMETERS+3
  lda #8
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #6
  jsr agi
  lda API_ERROR
  sta err
  lda #<e5nom
  sta cas
  lda #>e5nom
  sta cas+1
  jsr ligne_err

  ldx #<sfin
  ldy #>sfin
  jsr print
.if NEO
  jmp $FFFF
.endif
  rts

; Relit le pixel écran (X écran, Y écran) aux octets 8-11 du cas dans val ($FF si X = $FFFF).
relire:
  ldy #9
  lda (cas),y
  cmp #$FF
  bne r0
  sta val
  rts
r0:
  ldy #8
r2:
  lda (cas),y
  sta API_PARAMETERS-8,y
  iny
  cpy #12
  bne r2
  lda #33
  ldx #5
  jsr api
  lda API_PARAMETERS
  sta val
  rts

; « Ln ee vv » (lettre et chiffre aux octets 0-1 du cas ; vv = -- si val = $FF)
ligne_cas:
ligne_err:
  ldy #0
  lda (cas),y
  jsr wchar
  iny
  lda (cas),y
  jsr wchar
  lda #' '
  jsr wchar
  lda err
  jsr hex
  lda val
  cmp #$FF
  beq lc1
  lda #' '
  jsr wchar
  lda val
  jsr hex
lc1:
  jmp cr

agi:
  ldx #39
  jmp api

image:
  .byte $F2,$09,$F6,$32,$00,$32,$A7
  .byte $F2,$01,$F6,$3C,$64
  .byte $F2,$06,$F6,$3C,$65
  .byte $F2,$03,$F6,$46,$A0,$46,$A7
  .byte $FF
fimage:

celA:   .byte 1, 1, $00, $51, 0                 ; 1 x 1, couleur 5, transparent 0
celM:   .byte 2, 1, $00, $A1, $B1, 0            ; 2 x 1, couleurs A puis B
celW:   .byte 3, 1, $00, $11, $21, $31, 0       ; 3 x 1, couleurs 1, 2, 3
celZ:   .byte 0, 1, $00, 0                      ; largeur 0
e3cel:  .byte 1, 200, $00                       ; en-tête copié en $FFF7

cases:  ; lettre, chiffre, cel, x, y, priorité, drapeaux, X écran, Y écran
  .byte 'C','1'
  .word celA
  .byte 10, 10, 5, 0
  .word 20, 18
  .byte 'C','2'
  .word celA
  .byte 50, 30, 8, 0
  .word 100, 38
  .byte 'C','3'
  .word celA
  .byte 50, 20, 9, 0
  .word 100, 28
  .byte 'C','4'
  .word celA
  .byte 10, 10, 5, 0
  .word 21, 18
  .byte 'C','5'
  .word celA
  .byte 60, 100, 5, 0
  .word 120, 108
  .byte 'C','6'
  .word celA
  .byte 60, 100, 6, 0
  .word 120, 108
  .byte 'C','7'
  .word celA
  .byte 70, 161, 14, 0
  .word 140, 169
  .byte 'C','8'
  .word celA
  .byte 70, 160, 15, 0
  .word 140, 168
  .byte 'M','1'
  .word celM
  .byte 100, 50, 5, 0
  .word 200, 58
  .byte 'M','2'
  .word celM
  .byte 100, 50, 5, 0
  .word 202, 58
  .byte 'M','3'
  .word celM
  .byte 100, 60, 5, 1
  .word 200, 68
  .byte 'M','4'
  .word celM
  .byte 100, 60, 5, 1
  .word 202, 68
  .byte 'W','1'
  .word celW
  .byte 158, 70, 5, 0
  .word 316, 78
  .byte 'W','2'
  .word celW
  .byte 158, 70, 5, 0
  .word 318, 78
  .byte 0

r1:     .byte 'R','1', 0,0,0,0,0,0
  .word 20, 18

erreurs:  ; lettre, chiffre, cel, x, y, priorité, drapeaux, ligne du plan (2)
  .byte 'E','1'
  .word celZ
  .byte 10, 10, 5, 0
  .word 8
  .byte 'E','2'
  .word $FFFE
  .byte 10, 10, 5, 0
  .word 8
  .byte 'E','4'
  .word celA
  .byte 10, 168, 5, 0
  .word 8
  .byte 'E','6'
  .word celA
  .byte 10, 10, 5, 0
  .word 100
  .byte 0
e3nom:  .byte 'E','3'
e5nom:  .byte 'E','5'

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
