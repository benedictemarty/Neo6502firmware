; blitbank.asm — T-97 (signalé par Neo6502POP) : 12,3 depuis une banque, source 1 bit près de la fin.
; Banque 0 remplie de $FF (1,22 depuis $6000). Image 1 bit : 24 pixels (3 octets), 12 lignes, pas 3, à l'octet
; 8154 de la banque (fin à 8190 : DANS la banque) ; 12,3 solidmasked (couleur 5) vers la VRAM en (10,10).
; Avant le correctif le contrôle exigeait 24 OCTETS par ligne en source : erreur à la dernière ligne.
; Contre-essai : la même image à l'octet 8160 déborde vraiment (dernière ligne à 8193) : refusée.
; Sortie attendue : WRITE 00 / BLIT 00 05 05 / OVER 01 / END
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
WIN   = $6000
SRC   = $3000               ; zones du blitter (12 octets chacune)
TGT   = $3010
R     = $3020

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #<WIN                 ; fenêtre = $FF
  sta ptr
  lda #>WIN
  sta ptr+1
  ldx #$20
  ldy #0
  lda #$FF
f1:
  sta (ptr),y
  iny
  bne f1
  inc ptr+1
  dex
  bne f1
  stz API_PARAMETERS        ; 1,22 banque 0 <- $6000
  lda #<WIN
  sta API_PARAMETERS+1
  lda #>WIN
  sta API_PARAMETERS+2
  lda #22
  ldx #1
  jsr api
  lda API_ERROR
  sta R

  ldx #11                   ; zones : copie des modèles
z1:
  lda srcarea,x
  sta SRC,x
  lda tgtarea,x
  sta TGT,x
  dex
  bpl z1
  jsr blit
  sta R+1
  ldx #33                   ; (33,21) : dernier pixel de la dernière ligne ; (10,10) : premier
  ldy #21
  jsr read
  sta R+2
  ldx #10
  ldy #10
  jsr read
  sta R+3
  lda #<8160                ; OVER : source à 8160, la dernière ligne sort de la banque
  sta SRC
  lda #>8160
  sta SRC+1
  jsr blit
  sta R+4

  ldx #<swrite
  ldy #>swrite
  jsr print
  lda R
  jsr hex
  jsr cr
  ldx #<sblit
  ldy #>sblit
  jsr print
  lda R+1
  jsr hex
  lda #' '
  jsr wchar
  lda R+2
  jsr hex
  lda #' '
  jsr wchar
  lda R+3
  jsr hex
  jsr cr
  ldx #<sover
  ldy #>sover
  jsr print
  lda R+4
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

; --- 12,3 action 2 (solidmasked), SRC -> TGT ; A = erreur
blit:
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
  rts

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

; zone source : adresse 8154 page $A0 (banque 0), pad, pas 3, format 2 (bits), transparent 0, plein 5, 12 lignes, 24 valeurs
srcarea: .byte <8154, >8154, $A0, 0, 3, 0, 2, 0, 5, 12, 24, 0
; zone cible : VRAM (10,10) = 3210, page $80, pad, pas 320, format 0 (octets)
tgtarea: .byte <3210, >3210, $80, 0, <320, >320, 0, 0, 0, 0, 0, 0

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

swrite:  .text "WRITE ", 0
sblit:   .text "BLIT ", 0
sover:   .text "OVER ", 0
sfin:    .text "END", 0
