; text3.asm — 5,13 Text Write, 5,14 Text Scroll, 5,15 Text Clear (Trinity T-131, 0.16.89 : interface de NeoBBS).
; Mode 3 ; 5,13 en (78,0) attribut $1E de C9 CD CD BB (coupé : 78-79 seulement) ; en (0,1) attribut $4F de 00 1B FE
; (codes bruts = glyphes) ; 5,15 rectangle (10,2)-(12,3) attribut $21 ; lettres P Q R S T en colonne 0 des rangées
; 20-24 (attribut $07), 5,14 (20,24,+2,$70) puis (20,24,-1,$05) ; refus : 5,13 x=80, 5,14 haut > bas, 5,15 x1 > x2, 9,1 tortue ;
; 5,32 entrée 1 -> vert : le papier des cellules 78-79 change (plan vert) ; mode 0 puis 5,13 refusé.
; Relecture par 12,2 en $80 : glyphes, attributs ($80:1388 + cellule), mots de plans. Résultats en hexadécimal.
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
R     = $3000               ; relectures
E     = $3100               ; erreurs
ne    = $E0                 ; index dans E
nr    = $E1                 ; index dans R
k     = $E2

start:
  stz logLen
  lda #$20
  sta logLen+1
  stz ne
  stz nr

  lda #3                    ; mode 3
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  jsr err

  lda #78                   ; 5,13 (78,0) $1E  C9 CD CD BB
  ldx #0
  ldy #$1E
  jsr wstr
  .word s1
  lda #0                    ; 5,13 (0,1) $4F  00 1B FE
  ldx #1
  ldy #$4F
  jsr wstr
  .word s2
  ldx #4                    ; 5,15 (10,2)-(12,3) $21
cl:
  lda clr,x
  sta API_PARAMETERS,x
  dex
  bpl cl
  lda #15
  ldx #5
  jsr api
  jsr err

  lda #20                   ; P Q R S T en colonne 0, rangées 20-24
  sta k
let:
  lda k
  clc
  adc #'P'-20
  sta s3+1
  lda #0
  ldx k
  ldy #$07
  jsr wstr
  .word s3
  inc k
  lda k
  cmp #25
  bne let

  lda #2                    ; 5,14 (20,24,+2,$70)
  ldx #$70
  jsr scroll
  lda #$FF                  ; 5,14 (20,24,-1,$05)
  ldx #$05
  jsr scroll

  lda #80                   ; refus : 5,13 x = 80
  ldx #0
  ldy #$07
  jsr wstr
  .word s3
  lda #5                    ; 5,14 haut 5 > bas 3
  sta API_PARAMETERS
  lda #3
  sta API_PARAMETERS+1
  lda #1
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #14
  ldx #5
  jsr api
  jsr err
  lda #12                   ; 5,15 x1 12 > x2 10
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #10
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #15
  ldx #5
  jsr api
  jsr err

  lda #1                    ; 9,1 tortue : refusée en mode 3
  ldx #9
  jsr api
  jsr err

  ldx #0                    ; relectures avant 5,32
rd:
  lda rdtab,x
  ora rdtab+1,x
  beq rdf
  lda rdtab,x
  sta API_PARAMETERS+1
  lda rdtab+1,x
  sta API_PARAMETERS+2
  lda rdtab+2,x
  sta API_PARAMETERS+6
  phx
  jsr blit
  plx
  inx
  inx
  inx
  bra rd
rdf:

  lda #1                    ; 5,32 entrée 1 = vert ; plan vert, mot des cellules 72-79 (octets 3 : cellules 78-79)
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #255
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #32
  ldx #5
  jsr api
  lda #<($07D0+$3E8+36+3)
  sta API_PARAMETERS+1
  lda #>($07D0+$3E8+36+3)
  sta API_PARAMETERS+2
  lda #1
  sta API_PARAMETERS+6
  jsr blit

  stz API_PARAMETERS        ; mode 0, puis 5,13 refusé
  lda #9
  ldx #5
  jsr api
  jsr err
  lda #0
  ldx #0
  ldy #$07
  jsr wstr
  .word s3

  lda #12
  jsr wchar
  ldx #<serr
  ldy #>serr
  jsr print
  ldx #0
d1:
  cpx ne
  beq d1f
  lda E,x
  phx
  jsr hex
  lda #' '
  jsr wchar
  plx
  inx
  bra d1
d1f:
  jsr cr
  ldx #<sgr
  ldy #>sgr
  jsr print
  ldx #0
d2:
  cpx nr
  beq d2f
  lda R,x
  phx
  jsr hex
  lda #' '
  jsr wchar
  plx
  inx
  bra d2
d2f:
  jsr cr
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- 5,13 : A = x, X = y, Y = attribut, chaîne pointée par le mot qui suit l'appel
wstr:
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  pla
  sta ptr
  pla
  sta ptr+1
  ldy #1
  lda (ptr),y
  sta API_PARAMETERS+3
  iny
  lda (ptr),y
  sta API_PARAMETERS+4
  lda ptr                   ; retour après le mot
  clc
  adc #2
  tax
  lda ptr+1
  adc #0
  pha
  phx
  lda #13
  ldx #5
  jsr api
  jmp err

; --- 5,14 (20,24) : A = nombre, X = attribut
scroll:
  sta API_PARAMETERS+2
  stx API_PARAMETERS+3
  lda #20
  sta API_PARAMETERS
  lda #24
  sta API_PARAMETERS+1
  lda #14
  ldx #5
  jsr api
  jmp err

; --- erreur de l'appel -> E[ne++]
err:
  lda API_ERROR
  ldx ne
  sta E,x
  inc ne
  rts

; --- 12,2 $80:(P1,P2) -> R[nr], P6 octets ; nr += P6
blit:
  lda #$80
  sta API_PARAMETERS
  stz API_PARAMETERS+3
  lda nr
  sta API_PARAMETERS+4
  lda #>R
  sta API_PARAMETERS+5
  stz API_PARAMETERS+7
  lda API_PARAMETERS+6
  pha
  lda #2
  ldx #12
  jsr api
  pla
  clc
  adc nr
  sta nr
  rts

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

s1:      .byte 4, $C9, $CD, $CD, $BB
s2:      .byte 3, $00, $1B, $FE
s3:      .byte 1, 'P'
clr:     .byte 10, 2, 12, 3, $21
; relectures : déplacement dans $80 (16 bits), nombre d'octets ; 0,0 = fin
rdtab:   .word 78
         .byte 2                    ; glyphes (78,0)-(79,0)
         .word 5000+78
         .byte 2                    ; attributs
         .word 80
         .byte 3                    ; glyphes (0,1)-(2,1)
         .word 5000+80
         .byte 3
         .word 2*80+10
         .byte 3                    ; glyphes (10,2)-(12,2)
         .word 5000+3*80+10
         .byte 3                    ; attributs (10,3)-(12,3)
         .word 20*80
         .byte 1                    ; glyphes colonne 0, rangées 20-24
         .word 21*80
         .byte 1
         .word 22*80
         .byte 1
         .word 23*80
         .byte 1
         .word 24*80
         .byte 1
         .word 5000+20*80
         .byte 1                    ; attributs colonne 0, rangées 20 et 24
         .word 5000+24*80
         .byte 1
         .word 0
         .byte 0
serr:    .text "ERR ", 0
sgr:     .text "GRID ", 0
sfin:    .text "END", 0
