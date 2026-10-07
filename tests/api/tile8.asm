; tile8.asm — T-118 : 5,43 Draw Tilemap 8x8 (tuiles 8 x 8, attribut par case). Tout en RAM 6502 (page 0).
; Tuile 1, 2 bits par pixel : ligne 0 = index 3 3 2 1 0 0 0 0 ($F9 $00), lignes 1-7 = index 1 ($55 $55).
; Palette : 16 banques de 4, palette[n] = $80 + n. Carte 2 x 1 : case 0 = tuile 1 banque 0, case 1 = tuile 1
; banque 2 retournée en x. Fenêtre en (100,200) ; on relit les 16 octets de la ligne 200 (12,2) : « lettre erreur octets ».
;   A palette, options 0                       -> 83 83 82 81 80 80 80 80 88 88 88 88 89 8A 8B 8B
;   B sans palette, quartet bas, défilement y 1 -> 81 x8 puis 89 x8 (quartets hauts de A gardés)
;   C palette, 0 transparent                    -> 83 83 82 81 81 81 81 81 89 89 89 89 89 8A 8B 8B
;   D case 0 retournée en y, défilement y 7     -> 83 83 82 81 80 80 80 80 89 x8
;   E défilement x 4 (la fenêtre dépasse la carte de 4 pixels, laissés tels quels)
;                                               -> 80 80 80 80 88 88 88 88 89 8A 8B 8B 89 89 89 89
;   F refus : bpp 3, options 4, largeur 0, palette hors de sa page, carte hors de sa page, descripteur sur $FF00
;                                               -> 01 01 01 01 01 01
;   Mode 2 (16 couleurs, deux pixels par octet : chemin empaqueté), sans palette, pixels 100-115 de la ligne 200 (5,33) :
;   G options 0                                 -> 3 3 2 1 0 0 0 0 8 8 8 8 9 A B B
;   H défilement y 1                            -> 1 x8 puis 9 x8
;   I 0 transparent, défilement y 0             -> 3 3 2 1 1 1 1 1 9 9 9 9 9 A B B
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
err   = $F7
LU    = $3200               ; 16 octets relus dans la VRAM
TILES = $3400               ; tuile 0 (vide) puis tuile 1, 16 octets chacune
PAL   = $3500               ; 64 octets
MAP   = $3600               ; 2 cases
DESC  = $3700               ; descripteur de 26 octets
PIX   = $FA64               ; 200 * 320 + 100

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  ldx #0                    ; tuiles : 0 vide, 1 = ligne 0 $F9 $00, lignes 1-7 $55 $55
t1:
  stz TILES,x
  lda #$55
  sta TILES+16,x
  inx
  cpx #16
  bne t1
  lda #$F9
  sta TILES+16
  stz TILES+17
  ldx #0                    ; palette[n] = $80 + n
p1:
  txa
  ora #$80
  sta PAL,x
  inx
  cpx #64
  bne p1
  lda #<1                   ; case 0 : tuile 1, banque 0
  sta MAP
  stz MAP+1
  lda #1                    ; case 1 : tuile 1, banque 2 (bits 10-13), retournée en x (bit 14)
  sta MAP+2
  lda #$48
  sta MAP+3
  ldx #0                    ; descripteur de base
d1:
  lda base,x
  sta DESC,x
  inx
  cpx #26
  bne d1

  jsr draw                  ; A : palette, options 0
  lda #'A'
  jsr ligne

  lda #$FF                  ; B : sans palette, quartet bas, défilement y 1
  sta DESC+12
  lda #1
  sta DESC+9
  sta DESC+16
  jsr draw
  lda #'B'
  jsr ligne

  stz DESC+12               ; C : palette, 0 transparent, défilement y 0
  lda #2
  sta DESC+9
  stz DESC+16
  jsr draw
  lda #'C'
  jsr ligne

  lda #$80                  ; D : case 0 retournée en y (bit 15), défilement y 7, options 0
  sta MAP+1
  stz DESC+9
  lda #7
  sta DESC+16
  jsr draw
  lda #'D'
  jsr ligne

  stz MAP+1                 ; E : défilement x 4, y 0
  stz DESC+16
  lda #4
  sta DESC+14
  jsr draw
  lda #'E'
  jsr ligne
  stz DESC+14

  lda #'F'                  ; F : refus
  jsr wchar
  lda #3                    ; bpp 3
  sta DESC+8
  jsr refus
  lda #2
  sta DESC+8
  lda #4                    ; options 4
  sta DESC+9
  jsr refus
  stz DESC+9
  stz DESC+3                ; largeur 0
  jsr refus
  lda #2
  sta DESC+3
  lda #$F0                  ; palette en $90:7FF0 (64 octets demandés, 16 disponibles)
  sta DESC+10
  lda #$7F
  sta DESC+11
  lda #$90
  sta DESC+12
  jsr refus
  lda #<PAL
  sta DESC+10
  lda #>PAL
  sta DESC+11
  stz DESC+12
  lda #$F0                  ; carte en $FFF0, 255 x 1 cases
  sta DESC+0
  lda #$FF
  sta DESC+1
  sta DESC+3
  jsr refus
  lda #$F0                  ; descripteur en $FEF0 : déborde sur $FF00
  sta API_PARAMETERS
  lda #$FE
  sta API_PARAMETERS+1
  lda #43
  ldx #5
  jsr api
  lda API_ERROR             ; lu avant tout affichage (2,6 le remet à 0)
  sta err
  lda #' '
  jsr wchar
  lda err
  jsr hex
  jsr cr

  lda #2                    ; mode 2
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  ldx #0                    ; descripteur de base, sans palette
d2:
  lda base,x
  sta DESC,x
  inx
  cpx #26
  bne d2
  lda #$FF
  sta DESC+12
  jsr draw                  ; G
  lda #'G'
  jsr ligne2
  lda #1                    ; H : défilement y 1
  sta DESC+16
  jsr draw
  lda #'H'
  jsr ligne2
  stz DESC+16               ; I : 0 transparent
  lda #2
  sta DESC+9
  jsr draw
  lda #'I'
  jsr ligne2
  stz API_PARAMETERS        ; retour au mode 0
  lda #9
  ldx #5
  jsr api

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; 5,43 avec DESC
draw:
  lda #<DESC
  sta API_PARAMETERS
  lda #>DESC
  sta API_PARAMETERS+1
  lda #43
  ldx #5
  jsr api
  lda API_ERROR
  sta err
  rts

; 5,43 puis « erreur »
refus:
  jsr draw
  lda #' '
  jsr wchar
  lda err
  jmp hex

; « lettre erreur octets » : relit les 16 octets du pixel par 12,2 vers LU
ligne:
  jsr wchar
  lda #$80
  sta API_PARAMETERS
  lda #<PIX
  sta API_PARAMETERS+1
  lda #>PIX
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<LU
  sta API_PARAMETERS+4
  lda #>LU
  sta API_PARAMETERS+5
  lda #16
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
l1:
  lda #' '
  jsr wchar
  lda LU,x
  jsr hex
  inx
  cpx #16
  bne l1
  jmp cr

; « lettre erreur pixels » : pixels 100 à 115 de la ligne 200 relus par 5,33 (mode 2)
ligne2:
  jsr wchar
  lda #' '
  jsr wchar
  lda err
  jsr hex
  ldx #100
lp2:
  lda #' '
  jsr wchar
  stx API_PARAMETERS
  stz API_PARAMETERS+1
  lda #200
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  phx
  lda #33
  ldx #5
  jsr api
  plx
  lda API_PARAMETERS
  jsr nibble
  inx
  cpx #116
  bne lp2
  jmp cr

; descripteur : carte MAP (page 0) 2 x 1, tuiles TILES (page 0) 2 bpp, options 0, palette PAL (page 0),
; défilement 0,0, fenêtre (100,200) 16 x 1
base:
  .word MAP
  .byte 0, 2, 1
  .word TILES
  .byte 0, 2, 0
  .word PAL
  .byte 0, 0
  .word 0, 0
  .word 100, 200, 16, 1

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

sfin:    .text "END", 0
