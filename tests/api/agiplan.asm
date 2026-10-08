; agiplan.asm — T-122 : groupe 39, add.to.pic (39,7 Add Cel To Plane, 39,8 Fill Plane Priority).
; Même image synthétique que agicel.asm (priorités seules, visuel blanc 15) : colonne x = 50 en priorité 9 ;
; point (60,100) en ligne de contrôle 1 avec la priorité 6 juste en dessous ; colonne x = 70 en ligne de
; contrôle 3 de y = 160 jusqu'au bas. Aucun mode écran n'est utilisé : le plan est relu par 39,4.
; Chaque cas « Xn ee vv pp » : erreur de 39,7 ou 39,8, puis visuel et priorité du point relu par 39,4.
;   A1 cel A (couleur 5) en priorité 5 sur le fond (4)                         -> 00 05 05
;   A2 priorité 8 sur la colonne de priorité 9 : plan inchangé                 -> 00 0F 09
;   A3 priorité 9 sur la colonne (hypothèse « >= »)                            -> 00 05 09
;   A4 priorité 5 sur la ligne de contrôle (60,100) : 6 trouvé dessous         -> 00 0F 01
;   A5 priorité 6 au même point : la ligne de contrôle est remplacée           -> 00 05 06
;   A6 priorité 14 sur la colonne de contrôle jusqu'au bas (hypothèse : 15)    -> 00 0F 03
;   A7 priorité 15 au même endroit                                             -> 00 05 0F
;   M1-M2 cel M (A puis B) en miroir en (100,50)                               -> B, A
;   W1 cel W (1, 2, 3) coupé au bord droit en x = 158, relu en 159            -> 00 02 05
;   T1-T2 cel T (transparent puis 5) en (120,50)                               -> 0F 04, 05 05
;   H1-H2 cel H (1 x 3, couleur 7) en priorité 6, bas en y = 12               -> 07 06 en y = 10 et 12
;   P0, P3, PG priorité 0, 3, 16 : erreur 1, plan inchangé en (30,30)           -> 01 0F 04
;   E1 cel de largeur 0 ; E2 en-tête en $FFFE ; E4 y = 168                      -> 01
;   F1-F3 39,8 (20,40, 3 x 2) priorité 2 : dedans, à droite, dessous            -> 02, 04, 04
;   F4 39,8 sur le point de A1 en priorité 12 : couleur gardée                 -> 00 05 0C
;   F5 39,8 coupé au coin bas droit (158,166, 10 x 10) priorité 7              -> 00 0F 07
;   F6, F7, F8 largeur 0, hauteur 0, priorité 16 : erreur 1, plan inchangé      -> 01 0F 02
;   F9 rectangle hors du plan : rien d'écrit, pas d'erreur                      -> 00 0F 07
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
err   = $F9

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
  lda #<cases
  sta cas
  lda #>cases
  sta cas+1

boucle:                     ; cas : fonction (7 ou 8), lettre, chiffre, P0..P5, X et Y relus par 39,4
  ldy #0
  lda (cas),y
  beq fin
  ldy #3
c1:
  lda (cas),y
  sta API_PARAMETERS-3,y
  iny
  cpy #9
  bne c1
  ldy #0
  lda (cas),y
  jsr agi
  lda API_ERROR
  sta err
  ldy #9                    ; 39,4 sur (X, Y)
  lda (cas),y
  sta API_PARAMETERS
  iny
  lda (cas),y
  sta API_PARAMETERS+1
  lda #4
  jsr agi
  ldy #1
  lda (cas),y
  jsr wchar
  iny
  lda (cas),y
  jsr wchar
  lda #' '
  jsr wchar
  lda err
  jsr hex
  lda #' '
  jsr wchar
  lda API_PARAMETERS+2
  jsr hex
  lda #' '
  jsr wchar
  lda API_PARAMETERS+3
  jsr hex
  jsr cr
  clc
  lda cas
  adc #11
  sta cas
  bcc boucle
  inc cas+1
  bra boucle

fin:
  ldx #<sfin
  ldy #>sfin
  jsr print
.if NEO
  jmp $FFFF
.endif
  rts

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
celT:   .byte 2, 1, $00, $01, $51, 0            ; 2 x 1, transparent puis 5
celH:   .byte 1, 3, $00, $71, 0, $71, 0, $71, 0 ; 1 x 3, couleur 7
celZ:   .byte 0, 1, $00, 0                      ; largeur 0

cases:  ; fonction, lettre, chiffre, P0..P5, X, Y
  .byte 7,'A','1'
  .word celA
  .byte 10, 10, 5, 0, 10, 10
  .byte 7,'A','2'
  .word celA
  .byte 50, 30, 8, 0, 50, 30
  .byte 7,'A','3'
  .word celA
  .byte 50, 20, 9, 0, 50, 20
  .byte 7,'A','4'
  .word celA
  .byte 60, 100, 5, 0, 60, 100
  .byte 7,'A','5'
  .word celA
  .byte 60, 100, 6, 0, 60, 100
  .byte 7,'A','6'
  .word celA
  .byte 70, 161, 14, 0, 70, 161
  .byte 7,'A','7'
  .word celA
  .byte 70, 160, 15, 0, 70, 160
  .byte 7,'M','1'
  .word celM
  .byte 100, 50, 5, 1, 100, 50
  .byte 7,'M','2'
  .word celM
  .byte 100, 50, 5, 1, 101, 50
  .byte 7,'W','1'
  .word celW
  .byte 158, 70, 5, 0, 159, 70
  .byte 7,'T','1'
  .word celT
  .byte 120, 50, 5, 0, 120, 50
  .byte 7,'T','2'
  .word celT
  .byte 120, 50, 5, 0, 121, 50
  .byte 7,'H','1'
  .word celH
  .byte 80, 12, 6, 0, 80, 10
  .byte 7,'H','2'
  .word celH
  .byte 80, 12, 6, 0, 80, 12
  .byte 7,'P','0'
  .word celA
  .byte 30, 30, 0, 0, 30, 30
  .byte 7,'P','3'
  .word celA
  .byte 30, 30, 3, 0, 30, 30
  .byte 7,'P','G'
  .word celA
  .byte 30, 30, 16, 0, 30, 30
  .byte 7,'E','1'
  .word celZ
  .byte 30, 30, 5, 0, 30, 30
  .byte 7,'E','2'
  .word $FFFE
  .byte 30, 30, 5, 0, 30, 30
  .byte 7,'E','4'
  .word celA
  .byte 30, 168, 5, 0, 30, 30
  .byte 8,'F','1', 20, 40, 3, 2, 2, 0, 21, 41
  .byte 8,'F','2', 20, 40, 3, 2, 2, 0, 23, 41
  .byte 8,'F','3', 20, 40, 3, 2, 2, 0, 20, 42
  .byte 8,'F','4', 10, 10, 1, 1, 12, 0, 10, 10
  .byte 8,'F','5', 158, 166, 10, 10, 7, 0, 159, 167
  .byte 8,'F','6', 21, 41, 0, 1, 9, 0, 21, 41
  .byte 8,'F','7', 21, 41, 1, 0, 9, 0, 21, 41
  .byte 8,'F','8', 21, 41, 1, 1, 16, 0, 21, 41
  .byte 8,'F','9', 200, 200, 5, 5, 9, 0, 159, 167
  .byte 0

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
