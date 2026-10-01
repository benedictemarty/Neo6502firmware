; blitpack.asm — T-90 (besoin n° 1 de Neo6502POP) : 12,3 vers une cible « deux pixels par octet »
; (format 5 : premier pixel dans le quartet haut, x pair ; format 6 : dans le quartet bas, x impair).
; Cibles en RAM du 6502 ($3100.., remplie de $55 : un quartet non écrit doit rester à 5).
; Pour chaque cas : « Cn erreur o0 o1 o2 o3 o4 » (cinq octets relus à partir de l'adresse du cas).
;   C1 solidmasked, bits $B1 (10110001), plein $A, format 5 -> A5 AA 55 5A 55
;   C2 idem, format 6                                    -> 5A 5A A5 55 A5
;   C3 copy, quartets $12 $34, format 5                  -> 12 34 55 55 55
;   C4 idem, format 6                                    -> 51 23 45 55 55
;   C5 copymasked (transparent 0), quartets $10 $02, f.5 -> 15 52 55 55 55
;   C6 copy, octets $1F $02 (4 bits bas gardés), f.5    -> F2 55 55 55 55
;   C7 solidmasked, bits $80, plein $FA (4 bits bas), f.5 -> A5 55 55 55 55
;   C8 deux lignes, pas 4 en cible, bits $FF puis $80, plein 3, f.6 -> 53 33 33 33 33 (la 2e ligne,
;      un seul pixel, écrit le quartet bas de $313C : 35 -> 33 ; avec un pas faux elle n'y serait pas)
;   C9 bornes : format 6, 8 pixels, cible à $7FFB de la page $90 (5 octets, fin à $7FFF) : 00
;   CA bornes : la même à $7FFC (déborde d'un octet) : 01
;   CB format 5 en SOURCE : 01
;   (C9-CB relisent $3044.. : 00 00 00 00 12, données inchangées)
;   M2 mode 2, bits $B1 plein 7 en (3,100), format 6, pixels 3 à 10 relus par 5,33 -> 00 7 0 7 7 0 0 0 7
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
err   = $F7                 ; API_ERROR de 12,3, sauvé avant tout affichage (2,6 le remet à 0)
SRC   = $3000               ; zones du blitter (12 octets chacune)
TGT   = $300C               ; juste après SRC : la boucle copie les 24 octets d'un bloc
DATA  = $3040               ; données source
CIBLE = $3100               ; cibles, remplies de $55

start:
  stz logLen
  lda #$20
  sta logLen+1
  ldx #0                    ; données source
d1:
  lda donnees,x
  sta DATA,x
  inx
  cpx #18
  bne d1
  ldx #0                    ; cibles = $55
  lda #$55
d2:
  sta CIBLE,x
  inx
  cpx #$40
  bne d2
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
  beq fin
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
  lda #3
  ldx #12
  jsr api
  lda API_ERROR
  sta err
  lda #'C'
  jsr wchar
  lda num
  jsr nib
  lda #' '
  jsr wchar
  lda err
  jsr hex
  ldy #25                   ; adresse à relire
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
  adc #27
  sta cas
  bcc boucle
  inc cas+1
  bra boucle

fin:
  lda #2                    ; M2 : de bout en bout en mode 2 (5,9), page de dessin = début de la VRAM
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  ldy #0                    ; zones : bits $B1, plein 7, vers (3,100) : octet 16001 de la VRAM, format 6 (x impair)
m1:
  lda mode2,y
  sta SRC,y
  iny
  cpy #24
  bne m1
  lda #2
  sta API_PARAMETERS
  lda #<SRC
  sta API_PARAMETERS+1
  lda #>SRC
  sta API_PARAMETERS+2
  lda #<TGT
  sta API_PARAMETERS+3
  lda #>TGT
  sta API_PARAMETERS+4
  lda #3
  ldx #12
  jsr api
  lda API_ERROR
  sta err
  lda #'M'
  jsr wchar
  lda #'2'
  jsr wchar
  lda #' '
  jsr wchar
  lda err
  jsr hex
  lda #3                    ; pixels 3 à 10 de la ligne 100 (5,33)
  sta num
m2:
  lda #' '
  jsr wchar
  lda num
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #100
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #33
  ldx #5
  jsr api
  lda API_PARAMETERS
  jsr nib
  inc num
  lda num
  cmp #11
  bne m2
  jsr cr
  stz API_PARAMETERS        ; retour au mode 0
  lda #9
  ldx #5
  jsr api
  ldx #<sfin
  ldy #>sfin
  jsr print
.if NEO
  jmp $FFFF
.endif
  rts

; données : $3040 bits $B1 ; $3041 bits $80 ; $3042-3 bits $FF $80 (deux lignes) ; $3048 $12 $34 ; $304C $10 $02 ; $3050 $1F $02
donnees: .byte $B1, $80, $FF, $80, 0, 0, 0, 0, $12, $34, 0, 0, $10, $02, 0, 0, $1F, $02

; cas : action, zone source (adresse, page, pad, pas, format, transparent, plein, hauteur, largeur),
;       zone cible (adresse, page, pad, pas, format, 5 octets ignorés), adresse relue
cases:
  .byte 2, $40,$30,0,0, 1,0, 2,0,$0A, 1, 8,0,   $00,$31,0,0, 8,0, 5,0,0,0,0,0
  .word CIBLE+$00
  .byte 2, $40,$30,0,0, 1,0, 2,0,$0A, 1, 8,0,   $08,$31,0,0, 8,0, 6,0,0,0,0,0
  .word CIBLE+$08
  .byte 0, $48,$30,0,0, 2,0, 1,0,0,   1, 4,0,   $10,$31,0,0, 8,0, 5,0,0,0,0,0
  .word CIBLE+$10
  .byte 0, $48,$30,0,0, 2,0, 1,0,0,   1, 4,0,   $18,$31,0,0, 8,0, 6,0,0,0,0,0
  .word CIBLE+$18
  .byte 1, $4C,$30,0,0, 2,0, 1,0,0,   1, 4,0,   $20,$31,0,0, 8,0, 5,0,0,0,0,0
  .word CIBLE+$20
  .byte 0, $50,$30,0,0, 2,0, 0,0,0,   1, 2,0,   $28,$31,0,0, 8,0, 5,0,0,0,0,0
  .word CIBLE+$28
  .byte 2, $41,$30,0,0, 1,0, 2,0,$FA, 1, 8,0,   $30,$31,0,0, 8,0, 5,0,0,0,0,0
  .word CIBLE+$30
  .byte 2, $42,$30,0,0, 1,0, 2,0,$03, 2, 8,0,   $38,$31,0,0, 4,0, 6,0,0,0,0,0
  .word CIBLE+$38
  .byte 2, $40,$30,0,0, 1,0, 2,0,$0A, 1, 8,0,   $FB,$7F,$90,0, 8,0, 6,0,0,0,0,0
  .word DATA+4
  .byte 2, $40,$30,0,0, 1,0, 2,0,$0A, 1, 8,0,   $FC,$7F,$90,0, 8,0, 6,0,0,0,0,0
  .word DATA+4
  .byte 0, $48,$30,0,0, 2,0, 5,0,0,   1, 4,0,   $00,$31,0,0, 8,0, 0,0,0,0,0,0
  .word DATA+4
  .byte $FF

; M2 : source bits $B1 (1 ligne, 8 pixels, plein 7) ; cible VRAM octet 16001 = (3,100), page $80, pas 160, format 6
; (ligne 100 : la console écrit « M2 … » sur les premières lignes, en mode 2 aussi)
mode2: .byte $40,$30,0,0, 1,0, 2,0,7, 1, 8,0,  <16001,>16001,$80,0, 160,0, 6,0,0,0,0,0

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
