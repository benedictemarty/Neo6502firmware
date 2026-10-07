; sprreset.asm — T-121 (décision bmarty 2026-10-07) : 6,1 Sprite Reset remet aussi 6,6 à 0 et 6,8 à $90.
; Données de sprbank.asm : banque 0, $0100 image 5 x 1 en 8 bits, valeur 4 ; RAM graphique $0100 même image, valeur 9.
; Chaque ligne : « lettre erreur » puis les 6 octets de la VRAM à partir de (100,196).
;   A 6,8 $A0, sprite 0 = $0100 de la banque, (100,196)            -> 00 40 40 40 40 40 00
;   B 6,6 mode 1 (opaque), 6,1, puis sprite 0 = $0100 sans 6,8     -> 00 90 90 90 90 90 00 (image de la RAM graphique)
;   C sprite 1 = $0100, même place : par OU exclusif, 9 ^ 9 = 0    -> 00 00 00 00 00 00 00 (opaque : 90, sprite 0 devant)
; Contre-essai : sur la 0.16.80 (6,1 garde 6,6 et 6,8), B rend 40 et C rend 40.
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
GFX   = $3300               ; RAM graphique $0000..$01FF préparée ici
WIN   = $6000               ; contenu de la banque 0

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12                   ; écran effacé : la console reste en haut
  jsr wchar
  lda #<WIN                 ; banque : 32 pages à 0
  sta ptr
  lda #>WIN
  sta ptr+1
  ldx #$20
  ldy #0
  lda #0
w0:
  sta (ptr),y
  iny
  bne w0
  inc ptr+1
  dex
  bne w0
  ldx #4
  lda #4
w1:
  sta WIN+$100,x
  dex
  bpl w1
  lda #$6C
  sta WIN+$200
  lda #6
  sta WIN+$1FFC
  sta WIN+$1FFD
  sta WIN+$1FFE
  sta WIN+$1FFF
  stz API_PARAMETERS        ; 1,22 : banque 0 <- WIN
  lda #<WIN
  sta API_PARAMETERS+1
  lda #>WIN
  sta API_PARAMETERS+2
  lda #22
  ldx #1
  jsr api
  ldx #0                    ; RAM graphique
g0:
  stz GFX,x
  stz GFX+$100,x
  inx
  bne g0
  ldx #3
g1:
  lda table,x
  sta GFX+$10,x
  dex
  bpl g1
  ldx #4
  lda #9
g2:
  sta GFX+$100,x
  dex
  bpl g2
  stz API_PARAMETERS        ; GFX -> $90:0000, $0200 octets
  lda #<GFX
  sta API_PARAMETERS+1
  lda #>GFX
  sta API_PARAMETERS+2
  lda #$90
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  stz API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #2
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

table:  .byte 0, 5, 6, 7

Y196 = 196*320+100
Y200 = 200*320+100
Y202 = 202*320+100

etapes:
  .byte 8, $A0, 0,0,0,0,0,0,0                      ; A
  .byte 7, 0, $00,$01, 5,1, 8, $FF,$FF
  .byte 2, 0, <100,>100, <196,>196, $80, 0, 7
  .byte 0, 'A', <Y196, >Y196, 0,0,0,0,0
  .byte 6, 1, 0,0,0,0,0,0,0                        ; B
  .byte 1, 0, 0,0,0,0,0,0,0
  .byte 7, 0, $00,$01, 5,1, 8, $FF,$FF
  .byte 2, 0, <100,>100, <196,>196, $80, 0, 7
  .byte 0, 'B', <Y196, >Y196, 0,0,0,0,0
  .byte 7, 1, $00,$01, 5,1, 8, $FF,$FF             ; C
  .byte 2, 1, <100,>100, <196,>196, $80, 0, 7
  .byte 0, 'C', <Y196, >Y196, 0,0,0,0,0
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
