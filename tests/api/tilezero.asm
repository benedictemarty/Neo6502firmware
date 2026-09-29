; tilezero.asm — T-92 : pixel 0 transparent dans une tilemap dont l'octet 0 a le bit 7 ($81).
; tilezero.bin (chargé en mémoire graphique par 3,2 $FFFF) : une tuile 16×16, moitié gauche couleur 0,
; moitié droite couleur 5. Fond : carte A (2 tuiles $F3, couleur 3) sur (0,0)-(32,16). Puis carte de 1 tuile
; (tuile 0) sur la même fenêtre : $81 -> 0 transparent et le reste de la fenêtre au-delà de la carte intact ;
; 1 -> comportement actuel (0 écrit, au-delà effacé). Pixels lus (2,2) (12,2) (20,2) par 5,33.
; Sortie attendue : LOAD 00 / A 03 03 03 / B 03 05 03 / C 00 05 00 / END
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
tmp   = $F6

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  ldx #<sload               ; LOAD : 3,2 tilezero.bin -> $FFFF
  ldy #>sload
  jsr print
  lda #<fname
  sta API_PARAMETERS
  lda #>fname
  sta API_PARAMETERS+1
  lda #$FF
  sta API_PARAMETERS+2
  sta API_PARAMETERS+3
  lda #2
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<mapa                ; A : fond couleur 3
  ldy #>mapa
  jsr draw
  ldx #<sa
  ldy #>sa
  jsr probe
  ldx #<mapb                ; B : $81, tuile 0
  ldy #>mapb
  jsr draw
  ldx #<sb
  ldy #>sb
  jsr probe
  ldx #<mapa                ; A de nouveau, puis C : 1, tuile 0
  ldy #>mapa
  jsr draw
  ldx #<mapc
  ldy #>mapc
  jsr draw
  ldx #<sc
  ldy #>sc
  jsr probe
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- 5,35 carte X/Y (décalage 0,0) puis 5,8 sur (0,0)-(32,16)
draw:
  stx API_PARAMETERS
  sty API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #35
  ldx #5
  jsr api
  stz API_PARAMETERS
  stz API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #32
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #16
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #8
  ldx #5
  jmp api

; --- libellé X/Y puis pixels (2,2) (12,2) (20,2)
probe:
  jsr print
  lda #2
  jsr pix
  lda #12
  jsr pix
  lda #20
  jsr pix
  jmp cr
pix:
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #2
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #33
  ldx #5
  jsr api
  lda API_PARAMETERS
  sta tmp
  lda #' '
  jsr wchar
  lda tmp
  jmp hex

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

sload:   .text "LOAD ", 0
sa:      .text "A", 0
sb:      .text "B", 0
sc:      .text "C", 0
sfin:    .text "END", 0
fname:   .ptext "tilezero.bin"
mapa:    .byte 1, 2, 1, $F3, $F3
mapb:    .byte $81, 1, 1, 0
mapc:    .byte 1, 1, 1, 0
