; mode2.asm — T-90 : mode 2 d'essai, 320×240×16 couleurs à deux pages.
; 5,9 mode 2 puis 5,10 ; 5,11 et 5,12 page 1 acceptées, page 2 refusée ; pixels écrits en 16 couleurs (5,3 Plot,
; 5,39 Write Pixel) relus par 5,33 dans la page de dessin, y compris une abscisse impaire ; retour au mode 0.
; Le journal est tenu en RAM (la console redessine l'écran, les pixels sont lus avant tout affichage).
; Sortie attendue : MODE 00 02 / PAGE 00 00 01 / PIX 07 0C 00 / BACK 00 / END
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
R     = $3000               ; résultats, affichés à la fin (hors du code)

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #2                    ; 5,9 mode 2
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda API_ERROR
  sta R
  lda #10                   ; 5,10 -> mode
  ldx #5
  jsr api
  lda API_PARAMETERS
  sta R+1
  lda #1                    ; 5,11 page 1, 5,12 page 1, 5,11 page 2 (refusée)
  sta API_PARAMETERS
  lda #11
  ldx #5
  jsr api
  lda API_ERROR
  sta R+2
  lda #1
  sta API_PARAMETERS
  lda #12
  ldx #5
  jsr api
  lda API_ERROR
  sta R+3
  lda #2
  sta API_PARAMETERS
  lda #11
  ldx #5
  jsr api
  lda API_ERROR
  sta R+4
  lda #1                    ; retour au dessin en page 1 (la page 2 a été refusée)
  sta API_PARAMETERS
  lda #11
  ldx #5
  jsr api
  lda #7                    ; 5,39 Write Pixel : couleur 7 en (10,10), 12 en (11,10)
  ldx #10
  jsr wpix
  lda #12
  ldx #11
  jsr wpix
  ldx #10                   ; relecture (10,10) (11,10) (12,10)
  jsr read
  sta R+5
  ldx #11
  jsr read
  sta R+6
  ldx #12
  jsr read
  sta R+7
  stz API_PARAMETERS        ; 5,9 mode 0
  lda #9
  ldx #5
  jsr api
  lda API_ERROR
  sta R+8

  lda #12
  jsr wchar
  ldx #<smode
  ldy #>smode
  jsr print
  lda R
  jsr hexsp
  lda R+1
  jsr hex
  jsr cr
  ldx #<spage
  ldy #>spage
  jsr print
  lda R+2
  jsr hexsp
  lda R+3
  jsr hexsp
  lda R+4
  jsr hex
  jsr cr
  ldx #<spix
  ldy #>spix
  jsr print
  lda R+5
  jsr hexsp
  lda R+6
  jsr hexsp
  lda R+7
  jsr hex
  jsr cr
  ldx #<sback
  ldy #>sback
  jsr print
  lda R+8
  jsr hex
  jsr cr
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- 5,39 Write Pixel (X, 10) couleur A
wpix:
  sta API_PARAMETERS+4
  stx API_PARAMETERS
  stz API_PARAMETERS+1
  lda #10
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #39
  ldx #5
  jmp api

; --- 5,33 Read Pixel (X, 10) -> A
read:
  stx API_PARAMETERS
  stz API_PARAMETERS+1
  lda #10
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #33
  ldx #5
  jsr api
  lda API_PARAMETERS
  rts

hexsp:
  jsr hex
  lda #' '
  jmp wchar

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
spage:   .text "PAGE ", 0
spix:    .text "PIX ", 0
sback:   .text "BACK ", 0
sfin:    .text "END", 0
