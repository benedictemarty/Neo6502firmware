; fastdraw.asm — dessin des modes compacts (Trinity T-132, P2/P3 de l'étude, 0.16.92) : les chemins « un octet à la fois »
; doivent poser EXACTEMENT les mêmes pixels que l'ancien chemin pixel par pixel. Pour les modes 0, 1 et 2 : 5,9, puis
; 15 formes (5,1 et, ou exclusif, plein ; 5,2 lignes, 5,3 rectangles, 5,4 ellipses ; bords pairs et impairs, sens
; inverses, lignes horizontales et verticales, diagonale, débordement, XOR), puis du texte en couleur par la console
; (glyphes, P3), puis la somme de Fletcher 16 bits de la mémoire vidéo ($80, relue par 12,2 en blocs de 256 octets :
; 300 blocs en mode 0, 124 en mode 1, 150 en mode 2). Référence produite par la 0.16.91 (chemins lents).
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
BUF   = $5000
R     = $3000
mode  = $E0
sp    = $E2                 ; pointeur dans la table des formes
s1    = $E4
s2    = $E6
blk   = $E8                 ; bloc courant (16 bits)
nblk  = $EA
nr    = $EC

start:
  stz logLen
  lda #$20
  sta logLen+1
  stz nr
  stz mode
mloop:
  lda mode                  ; 5,9 mode
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda #<shapes
  sta sp
  lda #>shapes
  sta sp+1
  ldx #15
sh:
  phx
  ldy #0                    ; 5,1 : et, ou exclusif, plein, taille 1, retournement 0
  lda (sp),y
  sta API_PARAMETERS
  iny
  lda (sp),y
  sta API_PARAMETERS+1
  iny
  lda (sp),y
  sta API_PARAMETERS+2
  lda #1
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #1
  ldx #5
  jsr api
  ldy #4                    ; coordonnées
  ldx #0
co:
  lda (sp),y
  sta API_PARAMETERS,x
  iny
  inx
  cpx #8
  bne co
  ldy #3                    ; 5,cmd
  lda (sp),y
  ldx #5
  jsr api
  lda sp
  clc
  adc #12
  sta sp
  bcc sh1
  inc sp+1
sh1:
  plx
  dex
  bne sh

  ldx #0                    ; texte en couleur par la console (sans journal)
tx:
  lda texte,x
  beq txf
  jsr raw
  inx
  bra tx
txf:
  ldx mode                  ; nombre de blocs de 256 octets
  lda nblks,x
  sta nblk
  lda nblkh,x
  sta nblk+1
  stz s1
  stz s1+1
  stz s2
  stz s2+1
  stz blk
  stz blk+1
bl:
  lda #$80                  ; 12,2 $80+(blk>>8) : (blk & $FF) << 8 -> BUF, 256 octets
  clc
  adc blk+1
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda blk
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #>BUF
  sta API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #1
  sta API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  ldx #0
fl:
  lda BUF,x                 ; s1 += octet ; s2 += s1
  clc
  adc s1
  sta s1
  bcc fl1
  inc s1+1
fl1:
  lda s2
  clc
  adc s1
  sta s2
  lda s2+1
  adc s1+1
  sta s2+1
  inx
  bne fl
  inc blk
  bne bl1
  inc blk+1
bl1:
  lda blk
  cmp nblk
  bne bl
  lda blk+1
  cmp nblk+1
  bne bl
  ldx nr                    ; résultats : mode, s1, s2
  lda mode
  sta R,x
  lda s1+1
  sta R+1,x
  lda s1
  sta R+2,x
  lda s2+1
  sta R+3,x
  lda s2
  sta R+4,x
  txa
  clc
  adc #5
  sta nr
  inc mode
  lda mode
  cmp #3
  beq fin
  jmp mloop
fin:
  stz API_PARAMETERS        ; mode 0 et affichage
  lda #9
  ldx #5
  jsr api
  lda #12
  jsr wchar
  ldx #0
pr:
  cpx nr
  beq prf
  lda R,x
  phx
  jsr hex
  lda #' '
  jsr wchar
  plx
  lda R+1,x
  phx
  jsr hex
  plx
  lda R+2,x
  phx
  jsr hex
  lda #' '
  jsr wchar
  plx
  lda R+3,x
  phx
  jsr hex
  plx
  lda R+4,x
  phx
  jsr hex
  jsr cr
  pla
  clc
  adc #5
  tax
  bra pr
prf:
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- caractère sur la console sans journal (2,6)
raw:
  sta API_PARAMETERS
  lda #6
  sta API_FUNCTION
  lda #2
  sta API_COMMAND
  jmp wait

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

nblks:   .byte <300, <124, <150
nblkh:   .byte >300, >124, >150
texte:   .byte $83, $94, 'H', 'e', 'l', 'l', 'o', ' ', $E9, $8A, $91, ' ', 'Z', 'q', $87, $90, 13, 'O', 'k', 0
sfin:    .text "END", 0
shapes:
  .byte 0, 5, 1, 3
  .word 3, 5, 100, 40
  .byte 0, 9, 1, 3
  .word 10, 50, 11, 80
  .byte 0, 3, 0, 3
  .word 1, 1, 318, 238
  .byte 0, 7, 0, 2
  .word 5, 100, 200, 100
  .byte 0, 6, 0, 2
  .word 200, 110, 5, 110
  .byte 0, 2, 0, 2
  .word 50, 120, 50, 200
  .byte 0, 4, 0, 2
  .word 60, 200, 60, 120
  .byte 0, 1, 0, 2
  .word 0, 0, 300, 200
  .byte 0, 12, 1, 4
  .word 100, 120, 250, 220
  .byte 0, 13, 0, 4
  .word 20, 130, 90, 230
  .byte 0, 10, 0, 2
  .word 65516, 150, 400, 150
  .byte 255, 15, 1, 3
  .word 30, 20, 150, 180
  .byte 255, 1, 0, 2
  .word 7, 7, 7, 230
  .byte 255, 3, 1, 4
  .word 150, 60, 310, 170
  .byte 0, 11, 1, 3
  .word 101, 41, 102, 41
