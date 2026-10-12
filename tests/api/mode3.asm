; mode3.asm — mode 3, texte couleur 80 x 25 en cellules 8 x 8 (Trinity T-131).
; 5,9 P0=3 puis 5,10 ; la console écrit « A é Z » avec des couleurs ($81 $94 : encre 1, papier 4 ; $87 $90) ;
; 12,2 relit en $80 la grille (codes CP437 : é = $82) et le premier mot de chacun des 3 plans RGB222 (bleu,
; vert, rouge : 2 bits d'encre puis 2 de papier par cellule) ; refus du dessin (5,5, 6,2, 5,8, 5,33), page 0
; acceptée et page 1 refusée (5,11) ; 5,32 entrée 1 = rouge recolore l'écran ; retour au mode 0. Résultats
; affichés en mode 0 (journal), en hexadécimal.
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
R     = $3000               ; résultats

start:
  stz logLen
  lda #$20
  sta logLen+1

  lda #3                    ; 5,9 mode 3
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda API_ERROR
  sta R+$10
  lda #10                   ; 5,10 : P0-P7
  ldx #5
  jsr api
  ldx #7
inf:
  lda API_PARAMETERS,x
  sta R+$11,x
  dex
  bpl inf

  ldx #0                    ; console : CLS, encre 1 papier 4 « A é », encre 7 papier 0 « Z »
txt:
  lda texte,x
  beq txtf
  jsr raw
  inx
  bra txt
txtf:
  ldx #0                    ; grille et plans -> R+0..R+15
  jsr blitgrid

  lda #5                    ; refus : 5,5 pixel
  jsr refus5
  sta R+$20
  stz API_PARAMETERS        ; 6,2 sprite 0
  lda #2
  ldx #6
  jsr api
  lda API_ERROR
  sta R+$21
  lda #8                    ; 5,8 tilemap
  jsr refus5
  sta R+$22
  stz API_PARAMETERS        ; 5,11 page 0 (acceptée)
  lda #11
  ldx #5
  jsr api
  lda API_ERROR
  sta R+$23
  lda #1                    ; 5,11 page 1 (refusée)
  sta API_PARAMETERS
  lda #11
  ldx #5
  jsr api
  lda API_ERROR
  sta R+$24
  lda #33                   ; 5,33 lecture de pixel
  jsr refus5
  sta R+$25

  lda #1                    ; 5,32 : entrée 1 = rouge pur
  sta API_PARAMETERS
  lda #255
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #32
  ldx #5
  jsr api
  ldx #$30                  ; plans relus -> R+$30..
  jsr blitgrid

  stz API_PARAMETERS        ; retour au mode 0
  lda #9
  ldx #5
  jsr api
  lda API_ERROR
  sta R+$26

  lda #12
  jsr wchar
  ldx #<s1                  ; MODE3 err ; INFO 8 octets
  ldy #>s1
  jsr print
  ldx #$10
  ldy #9
  jsr dump
  ldx #<s2                  ; GRID 16 octets
  ldy #>s2
  jsr print
  ldx #$00
  ldy #16
  jsr dump
  ldx #<s3                  ; REFUS + MODE0
  ldy #>s3
  jsr print
  ldx #$20
  ldy #7
  jsr dump
  ldx #<s4                  ; PAL (grille et plans après 5,32)
  ldy #>s4
  jsr print
  ldx #$30
  ldy #16
  jsr dump
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- 5,A avec paramètres nuls ; rend l'erreur dans A
refus5:
  stz API_PARAMETERS
  stz API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  stz API_PARAMETERS+5
  stz API_PARAMETERS+6
  stz API_PARAMETERS+7
  ldx #5
  jsr api
  lda API_ERROR
  rts

; --- 4 x 12,2 de 4 octets : grille $80:0000, plans $80:07D0, $80:0BB8, $80:0FA0 -> R+X..
blitgrid:
  ldy #0
bg1:
  lda #$80
  sta API_PARAMETERS
  lda srclo,y
  sta API_PARAMETERS+1
  lda srchi,y
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  txa
  sta API_PARAMETERS+4
  lda #>R
  sta API_PARAMETERS+5
  lda #4
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  phx
  phy
  lda #2
  ldx #12
  jsr api
  ply
  plx
  inx
  inx
  inx
  inx
  iny
  cpy #4
  bne bg1
  rts
srclo:   .byte $00, $D0, $B8, $A0
srchi:   .byte $00, $07, $0B, $0F

; --- caractère sur la console sans journal (2,6)
raw:
  sta API_PARAMETERS
  lda #6
  sta API_FUNCTION
  lda #2
  sta API_COMMAND
  jmp wait

; --- Y octets de R+X en hexadécimal, puis retour
dump:
  lda R,x
  phx
  phy
  jsr hex
  lda #' '
  jsr wchar
  ply
  plx
  inx
  dey
  bne dump
  jmp cr

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

texte:   .byte 12, $81, $94, 'A', $E9, $87, $90, 'Z', 0
s1:      .text "MODE3 ", 0
s2:      .text "GRID ", 0
s3:      .text "REFUS ", 0
s4:      .text "PAL ", 0
sfin:    .text "END", 0
