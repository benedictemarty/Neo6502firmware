; mode2all.asm — T-90 : passe de compatibilité du mode 2 (320×240×16) dans l'émulateur.
; Console, rectangle, ligne, tilemap, image, sprite (XOR), QuickDraw, refus de 12,4 ; pixels relus par 5,33.
; mode2all.bin : tuile 0 (moitié gauche 0, droite 5), sprite 16×16 n° 0 plein de couleur 3.
; Résultats relevés avant tout affichage (la console écrit dans l'écran), affichés en mode 0.
; Sortie attendue : MODE 00 02 / CON 01 / RECT 09 / LINE 0A / TILE 00 05 / IMG 05 / SPR 0A 03 / QD 0C /
;                   BLIT 01 / BACK 00 / END
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
px    = $F6
py    = $F7
R     = $3000
RECTQ = $0C00

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #2                    ; mode 2
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda API_ERROR
  sta R
  lda #10
  ldx #5
  jsr api
  lda API_PARAMETERS
  sta R+1
  lda #<fname               ; 3,2 mode2all.bin -> mémoire graphique
  sta API_PARAMETERS
  lda #>fname
  sta API_PARAMETERS+1
  lda #$FF
  sta API_PARAMETERS+2
  sta API_PARAMETERS+3
  lda #2
  ldx #3
  jsr api

  lda #12                   ; CON : « X » en (0,0) ; au moins un pixel allumé dans la case 6×8
  jsr out
  lda #'X'
  jsr out
  stz R+2
  stz py
c1:
  stz px
c2:
  ldx px
  ldy py
  jsr read
  beq c3
  lda #1
  sta R+2
c3:
  inc px
  lda px
  cmp #6
  bne c2
  inc py
  lda py
  cmp #8
  bne c1

  lda #1                    ; RECT : plein, couleur 9, (20,20)-(30,30) ; lu (25,25)
  sta API_PARAMETERS
  lda #65
  ldx #5
  jsr api
  lda #9
  jsr colour
  lda #20
  ldx #20
  ldy #30
  jsr box
  ldx #25
  ldy #25
  jsr read
  sta R+3
  lda #10                   ; LINE : couleur 10, (40,20)-(60,20) ; lu (50,20)
  jsr colour
  lda #40
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #20
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #60
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #20
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #5
  jsr api
  ldx #50
  ldy #20
  jsr read
  sta R+4

  lda #<map                 ; TILE : carte 1×1 (tuile 0) sur (0,100)-(16,116) ; lu (2,102) (12,102)
  sta API_PARAMETERS
  lda #>map
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #35
  ldx #5
  jsr api
  stz API_PARAMETERS
  stz API_PARAMETERS+1
  lda #100
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #16
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #116
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #8
  ldx #5
  jsr api
  ldx #2
  ldy #102
  jsr read
  sta R+5
  ldx #12
  ldy #102
  jsr read
  sta R+6

  lda #40                   ; IMG : 5,7 image 0 (tuile 0) en (40,100) ; lu (52,102)
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #100
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #7
  ldx #5
  jsr api
  ldx #52
  ldy #102
  jsr read
  sta R+7

  stz API_PARAMETERS        ; SPR : 6,2 sprite 0 en (25,25), image 0 (16×16 n° 0) ; lu (25,25) sur le rectangle, (31,31) à côté
  lda #25
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #25
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  stz API_PARAMETERS+5
  stz API_PARAMETERS+6
  stz API_PARAMETERS+7
  stz API_PARAMETERS+8
  lda #2
  ldx #6
  jsr api
  ldx #25
  ldy #25
  jsr read
  sta R+8
  ldx #31
  ldy #31
  jsr read
  sta R+9

  lda #1                    ; QD : 32,1, encre 12, PaintRect (100,20)-(110,30) ; lu (105,25)
  ldx #32
  jsr api
  lda #12
  sta API_PARAMETERS
  lda #4
  ldx #32
  jsr api
  lda #100
  sta RECTQ
  stz RECTQ+1
  lda #20
  sta RECTQ+2
  stz RECTQ+3
  lda #110
  sta RECTQ+4
  stz RECTQ+5
  lda #30
  sta RECTQ+6
  stz RECTQ+7
  lda #<RECTQ
  sta API_PARAMETERS
  lda #>RECTQ
  sta API_PARAMETERS+1
  lda #8
  ldx #32
  jsr api
  ldx #105
  ldy #25
  jsr read
  sta R+10

  ldx #7                    ; BLIT : 12,4 refusé dans un mode compact
b1:
  stz API_PARAMETERS,x
  dex
  bpl b1
  lda #4
  ldx #12
  jsr api
  lda API_ERROR
  sta R+11

  stz API_PARAMETERS        ; mode 0
  lda #9
  ldx #5
  jsr api
  lda API_ERROR
  sta R+12

  lda #12                   ; affichage
  jsr wchar
  ldx #<smode
  ldy #>smode
  lda #2
  jsr outn
  ldx #<scon
  ldy #>scon
  lda #1
  jsr outn
  ldx #<srect
  ldy #>srect
  lda #1
  jsr outn
  ldx #<sline
  ldy #>sline
  lda #1
  jsr outn
  ldx #<stile
  ldy #>stile
  lda #2
  jsr outn
  ldx #<simg
  ldy #>simg
  lda #1
  jsr outn
  ldx #<sspr
  ldy #>sspr
  lda #2
  jsr outn
  ldx #<sqd
  ldy #>sqd
  lda #1
  jsr outn
  ldx #<sblit
  ldy #>sblit
  lda #1
  jsr outn
  ldx #<sback
  ldy #>sback
  lda #1
  jsr outn
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- libellé X/Y puis A octets de R (index courant idx) séparés par des espaces
idx:     .byte 0
outn:
  pha
  jsr print
  pla
  tax
o1:
  phx
  ldy idx
  lda R,y
  jsr hex
  inc idx
  plx
  dex
  beq o2
  phx
  lda #' '
  jsr wchar
  plx
  bra o1
o2:
  jmp cr

; --- A -> console seulement
out:
  sta API_PARAMETERS
  lda #6
  ldx #2
  jmp api

; --- 5,64 Set Color A
colour:
  sta API_PARAMETERS
  lda #64
  ldx #5
  jmp api

; --- 5,3 rectangle (A,X)-(Y,Y) : coin (A, X), coin opposé (Y, Y)
box:
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  stx API_PARAMETERS+2
  stz API_PARAMETERS+3
  sty API_PARAMETERS+4
  stz API_PARAMETERS+5
  sty API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #3
  ldx #5
  jmp api

; --- 5,33 Read Pixel (X, Y) -> A
read:
  stx API_PARAMETERS
  stz API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #33
  ldx #5
  jsr api
  lda API_PARAMETERS
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

smode:   .text "MODE ", 0
scon:    .text "CON ", 0
srect:   .text "RECT ", 0
sline:   .text "LINE ", 0
stile:   .text "TILE ", 0
simg:    .text "IMG ", 0
sspr:    .text "SPR ", 0
sqd:     .text "QD ", 0
sblit:   .text "BLIT ", 0
sback:   .text "BACK ", 0
sfin:    .text "END", 0
fname:   .ptext "mode2all.bin"
map:     .byte 1, 1, 1, 0
