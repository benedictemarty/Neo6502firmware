; agipic.asm — T-107 : groupe 39, images Sierra AGI.
; Image PICTURE synthétique (102 octets, aucune donnée de jeu) qui passe par toutes les commandes : couleurs
; et désactivations, lignes absolues, relatives, en escalier x et y, remplissages (visuel seul, priorité seule,
; les deux), pinceaux carré, cercle, « splatter », découpage aux bords. Attendu calculé par le décodeur hôte de
; Neo6502AGI (tools/agipic, identique au pixel près aux références de Sarien).
;   D1 erreur de 39,1 (effacement puis décodage)                          -> 00
;   E1 39,1 adresse $FFF0, longueur $20 (sort de la RAM 6502)              -> 01
;   E2 39,1 longueur 0                                                     -> 01
;   E3 39,4 x = 160                                                        -> 01
;   K  somme de contrôle des 26 880 points (octet priorité<<4 | visuel, lus par 39,4) : a += v, b += a (mod 256)
;   P  visuel et priorité de 8 points
;   S  39,2 en ligne 64 (mode 0), puis 5,33 sur les deux pixels écran de 3 points
;   E4 39,2 en ligne 100 (déborde de l'écran)                             -> 01
;   T  39,3 : durée du dernier 39,1 (0 dans les émulateurs)
;   E5 39,1 sur une ressource tronquée juste après une commande à liste (F0 0E F6) : doit se terminer -> 00
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
ka    = $F2                 ; somme de contrôle
kb    = $F3
px    = $F6
py    = $F7
num   = $F8
ecran = $3000               ; 6 pixels écran lus juste après 39,2 (avant que la console n'écrive par-dessus)

start:
  stz logLen
  lda #$20
  sta logLen+1

; D1 : décodage avec effacement
  lda #<image
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
  ldx #'D'
  ldy #'1'
  jsr ligne_err

; E1 : hors de la RAM 6502
  lda #$F0
  sta API_PARAMETERS
  lda #$FF
  sta API_PARAMETERS+1
  lda #$20
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #1
  jsr agi
  ldx #'E'
  ldy #'1'
  jsr ligne_err

; E2 : longueur 0
  lda #<image
  sta API_PARAMETERS
  lda #>image
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #1
  jsr agi
  ldx #'E'
  ldy #'2'
  jsr ligne_err

; E3 : point hors du plan
  lda #160
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #4
  jsr agi
  ldx #'E'
  ldy #'3'
  jsr ligne_err

; K : somme de contrôle du plan entier
  stz ka
  stz kb
  stz py
k1:
  stz px
k2:
  lda px
  sta API_PARAMETERS
  lda py
  sta API_PARAMETERS+1
  lda #4
  jsr agi
  lda API_PARAMETERS+3      ; priorité << 4 | visuel
  asl a
  asl a
  asl a
  asl a
  ora API_PARAMETERS+2
  clc
  adc ka
  sta ka
  clc
  adc kb
  sta kb
  inc px
  lda px
  cmp #160
  bne k2
  inc py
  lda py
  cmp #168
  bne k1
  lda #'K'
  jsr wchar
  lda #' '
  jsr wchar
  lda ka
  jsr hex
  lda #' '
  jsr wchar
  lda kb
  jsr hex
  jsr cr

; P : 8 points
  lda #'P'
  jsr wchar
  ldx #0
p1:
  lda points,x
  sta API_PARAMETERS
  lda points+1,x
  sta API_PARAMETERS+1
  phx
  lda #4
  jsr agi
  lda #' '
  jsr wchar
  lda API_PARAMETERS+2
  jsr nib
  lda API_PARAMETERS+3
  jsr nib
  plx
  inx
  inx
  cpx #16
  bne p1
  jsr cr

; S : affichage en ligne 64, relecture immédiate de l'écran
  lda #64
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #2
  jsr agi
  lda API_ERROR
  sta num
  ldx #0
s1:
  lda pecran,x              ; x écran (16 bits), y écran (16 bits)
  sta API_PARAMETERS
  lda pecran+1,x
  sta API_PARAMETERS+1
  lda pecran+2,x
  sta API_PARAMETERS+2
  lda pecran+3,x
  sta API_PARAMETERS+3
  phx
  lda #33
  ldx #5
  jsr api
  plx
  txa
  lsr a
  lsr a
  tay
  lda API_PARAMETERS
  sta ecran,y
  inx
  inx
  inx
  inx
  cpx #24
  bne s1
  lda #'S'
  jsr wchar
  lda #' '
  jsr wchar
  lda num
  jsr hex
  ldy #0
s2:
  lda #' '
  jsr wchar
  lda ecran,y
  phy
  jsr hex
  ply
  iny
  cpy #6
  bne s2
  jsr cr

; E4 : l'image ne tient pas à partir de la ligne 100
  lda #100
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #2
  jsr agi
  ldx #'E'
  ldy #'4'
  jsr ligne_err

; T : durée du dernier 39,1
  lda #3
  jsr agi
  ldx #3                    ; copie avant tout affichage (wchar écrit dans les paramètres)
t0:
  lda API_PARAMETERS,x
  sta ecran,x
  dex
  bpl t0
  lda #'T'
  jsr wchar
  ldx #0
t1:
  lda #' '
  jsr wchar
  lda ecran,x
  phx
  jsr hex
  plx
  inx
  cpx #4
  bne t1
  jsr cr

; E5 : ressource tronquée (le premier décodeur bouclait sans fin)
  lda #<tronque
  sta API_PARAMETERS
  lda #>tronque
  sta API_PARAMETERS+1
  lda #3
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #1
  jsr agi
  ldx #'E'
  ldy #'5'
  jsr ligne_err

  ldx #<sfin
  ldy #>sfin
  jsr print
.if NEO
  jmp $FFFF
.endif
  rts

; « X Y erreur » : X, Y = caractères, erreur = API_ERROR
ligne_err:
  lda API_ERROR
  sta num
  txa
  jsr wchar
  tya
  jsr wchar
  lda #' '
  jsr wchar
  lda num
  jsr hex
  jmp cr

; appel du groupe 39, A = fonction
agi:
  ldx #39
  jmp api

points: .byte 30,30, 10,10, 60,50, 0,0, 159,167, 80,20, 130,10, 70,140
; (30,30) (159,167) (80,20) du plan -> pixels écran (2x, 64+y) et (2x+1, 64+y)
pecran: .word 60,94, 61,94, 318,231, 319,231, 160,84, 161,84

image:
  .byte $F0,$01,$F2,$02,$F6,$0A,$0A,$3C,$0A,$3C,$32,$0A,$32,$0A,$0A
  .byte $F1,$F2,$05,$F8,$1E,$1E
  .byte $F0,$0C,$F3,$F8,$1E,$1E
  .byte $F2,$03,$F0,$04,$F7,$50,$14,$33,$B3,$3B,$BB,$70,$07
  .byte $F5,$64,$64,$78,$6E,$8C,$82
  .byte $F4,$14,$64,$78,$28,$8C
  .byte $F9,$13,$FA,$46,$8C
  .byte $F9,$22,$FA,$10,$5A,$96,$20,$64,$9B
  .byte $F9,$07,$FA,$9A,$A0,$00,$00
  .byte $F9,$35,$FA,$44,$82,$0A
  .byte $F0,$0E,$F2,$07,$F8,$00,$00,$9F,$A7
  .byte $F0,$0A,$F2,$0B,$F6,$EF,$05,$00,$A7
  .byte $F6,$05,$EE,$FF
fimage:
tronque: .byte $F0,$0E,$F6

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
