; hercules.asm — Toolbox en mode 1 Hercules (Trinity T-12) : QuickDraw (rectangles, gris = damier,
; CopyBits pixel par pixel) et Window Manager en 1 bpp. Pixels lus par 5,33 Read Pixel.
; Sortie attendue (console) :
;   MODE 00 01 / PAINT 00 01 00 / GREY 00 00 01 / COPY 00 01 00 00 01 / WIN 00 01 01 01 00 / BACK 00 00 / END
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=hercules.neo6502 hercules.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
rect  = $B00
buf   = $B10
area  = $B20
bits  = $B30

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  ldx #<smode               ; MODE : 5,9 mode 1 -> 00 ; 5,10 -> 01
  ldy #>smode
  jsr print
  lda #1
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda API_ERROR
  jsr hex
  lda #10
  ldx #5
  jsr api
  lda API_PARAMETERS
  sta buf
  lda #' '
  jsr wchar
  lda buf
  jsr hex
  jsr cr
  lda #1                    ; 32,1 InitGraf
  ldx #32
  jsr api

  ldx #<spaint              ; PAINT (10,10)-(50,50) encre 15 : (20,20)=01 (5,300)=00
  ldy #>spaint
  jsr print
  lda #15
  sta API_PARAMETERS
  lda #4
  ldx #32
  jsr api
  jsr setrect
  .byte 10,0, 10,0, 50,0, 50,0
  lda #8
  jsr apirect
  lda API_ERROR
  jsr hex
  ldx #20
  ldy #20
  jsr pixel
  ldx #5                    ; (5,300) : hors du rectangle et du texte console
  ldy #<300
  jsr pixel300y
  jsr cr

  ldx #<sgrey               ; GREY : EraseRect (60,10)-(70,20) couleur 9 -> damier : (60,10)=00 (61,10)=01
  ldy #>sgrey
  jsr print
  jsr setrect
  .byte 60,0, 10,0, 70,0, 20,0
  lda #9
  sta API_PARAMETERS+2
  lda #9
  jsr apirect
  lda API_ERROR
  jsr hex
  ldx #60
  ldy #10
  jsr pixel
  ldx #61
  ldy #10
  jsr pixel
  jsr cr

  ldx #<scopy               ; COPY : source BITS 8x2 = $A5,$5A en (100,100) -> (100,100)=01 (101,100)=00 (100,101)=00 (101,101)=01
  ldy #>scopy
  jsr print
  lda #$A5
  sta bits
  lda #$5A
  sta bits+1
  lda #<bits                ; area : adresse, page 0, pad, stride 1, format 2 (BITS), transp 0, solid 0, hauteur 2, largeur 8
  sta area
  lda #>bits
  sta area+1
  stz area+2
  stz area+3
  lda #1
  sta area+4
  stz area+5
  lda #2
  sta area+6
  stz area+7
  stz area+8
  lda #2
  sta area+9
  lda #8
  sta area+10
  stz area+11
  stz API_PARAMETERS        ; action copy
  lda #<area
  sta API_PARAMETERS+1
  lda #>area
  sta API_PARAMETERS+2
  lda #100
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #100
  sta API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #14
  ldx #32
  jsr api
  lda API_ERROR
  jsr hex
  ldx #100
  ldy #100
  jsr pixel
  ldx #101
  ldy #100
  jsr pixel
  ldx #100
  ldy #101
  jsr pixel
  ldx #101
  ldy #101
  jsr pixel
  jsr cr

  ldx #<swin                ; WIN : fenêtre (200,50)-(400,150) titre "Herc" -> id 01 ; cadre (200,50)=01 ; barre (250,55)=01 ; contenu (300,100)=00
  ldy #>swin
  jsr print
  jsr setrect
  .byte 200,0, 50,0, 144,1, 150,0
  lda #<rect
  sta API_PARAMETERS
  lda #>rect
  sta API_PARAMETERS+1
  lda #<title
  sta API_PARAMETERS+2
  lda #>title
  sta API_PARAMETERS+3
  lda #3                    ; titre + fermeture
  sta API_PARAMETERS+4
  lda #1
  ldx #34
  jsr api
  lda API_PARAMETERS+5
  sta buf+1
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda buf+1
  jsr hex
  ldx #200
  ldy #50
  jsr pixel
  ldx #250
  ldy #55
  jsr pixel
  ldx #<300
  ldy #100
  jsr pixel300
  jsr cr

  ldx #<sback               ; BACK : 5,9 mode 0 -> 00 ; 5,10 -> 00
  ldy #>sback
  jsr print
  stz API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda API_ERROR
  jsr hex
  lda #10
  ldx #5
  jsr api
  lda API_PARAMETERS
  sta buf
  lda #' '
  jsr wchar
  lda buf
  jsr hex
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts                       ; retour à l'appelant (sys du BASIC, ou NeoDOS pour un .NEO) ; reset sinon

; --- 5,33 Read Pixel (X, Y) -> " nn" (X < 256)
pixel:
  stx API_PARAMETERS
  stz API_PARAMETERS+1
  bra pixel2
pixel300:                   ; X = 300
  lda #<300
  sta API_PARAMETERS
  lda #>300
  sta API_PARAMETERS+1
pixel2:
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  bra pixel3
pixel300y:                  ; (X, 300)
  stx API_PARAMETERS
  stz API_PARAMETERS+1
  lda #<300
  sta API_PARAMETERS+2
  lda #>300
  sta API_PARAMETERS+3
pixel3:
  lda #33
  ldx #5
  jsr api
  lda API_PARAMETERS        ; P0 avant l'espace (wchar écrase P0)
  sta buf
  lda #' '
  jsr wchar
  lda buf
  jmp hex

; --- copie les 8 octets qui suivent l'appel dans rect
setrect:
  pla
  sta ptr
  pla
  sta ptr+1
  ldy #1
sr1:
  lda (ptr),y
  sta rect-1,y
  iny
  cpy #9
  bne sr1
  lda ptr
  clc
  adc #8                    ; retour sur le dernier octet des données (RTS ajoute 1)
  sta ptr
  lda ptr+1
  adc #0
  pha
  lda ptr
  pha
  rts

; --- 32,A avec P0-1 = rect
apirect:
  ldx #<rect
  stx API_PARAMETERS
  ldx #>rect
  stx API_PARAMETERS+1
  ldx #32
  jmp api

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
  pha                       ; journal en RAM $C00.. (longueur 16 bits en $BFE) : tests/toolbox/run_neo.sh
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
spaint:  .text "PAINT ", 0
sgrey:   .text "GREY ", 0
scopy:   .text "COPY ", 0
swin:    .text "WIN ", 0
sback:   .text "BACK ", 0
sfin:    .text "END", 0
title:   .ptext "Herc"
