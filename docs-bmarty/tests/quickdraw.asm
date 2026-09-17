; quickdraw.asm — Toolbox groupe 32 QuickDraw (F-40 du fork firmware, ADR-01) : port, clip,
; plume, rectangles clippés. Lecture des pixels par 12,2 (VRAM $80).
; Sortie attendue (console) :
;   CLIP 0000 0000 0140 00F0 / CLIP 000A 000A 0032 0032 / PAINT 00 00 05 / FRAME 00 07 05
;   INVERT 00 FA / ERASE 00 00 / PEN 0064 00C8 07 / BAD 01 / ADDR 01 / END
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=quickdraw.neo6502 quickdraw.asm

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
rect  = $B00
buf   = $B10

start:
  lda #12
  jsr wchar
  lda #1                    ; 32,1 InitGraf
  ldx #32
  jsr api
  jsr getclip               ; CLIP 0000 0000 0140 00F0

  jsr setrect               ; clip (10,10)-(50,50)
  .byte 10,0, 10,0, 50,0, 50,0
  lda #2
  jsr apirect
  jsr getclip               ; CLIP 000A 000A 0032 0032

  ldx #<spaint              ; PaintRect plein écran, couleur 5 : seul le clip est peint
  ldy #>spaint
  jsr print
  lda #5
  sta API_PARAMETERS
  lda #4
  ldx #32
  jsr api
  jsr setrect
  .byte 0,0, 0,0, 64,1, 240,0
  lda #8
  jsr apirect
  lda API_ERROR
  jsr hex
  ldx #5                    ; (5,100) hors clip = 00 (hors du texte console)
  ldy #100
  jsr pixel
  ldx #20                   ; (20,20) = 05
  ldy #20
  jsr pixel
  jsr cr

  ldx #<sframe              ; FrameRect (15,15)-(45,45) couleur 7
  ldy #>sframe
  jsr print
  lda #7
  sta API_PARAMETERS
  lda #4
  ldx #32
  jsr api
  jsr setrect
  .byte 15,0, 15,0, 45,0, 45,0
  lda #7
  jsr apirect
  lda API_ERROR
  jsr hex
  ldx #15                   ; bord = 07
  ldy #15
  jsr pixel
  ldx #16                   ; intérieur inchangé = 05
  ldy #16
  jsr pixel
  jsr cr

  ldx #<sinvert             ; InvertRect (16,16)-(20,20) : 05 -> FA
  ldy #>sinvert
  jsr print
  jsr setrect
  .byte 16,0, 16,0, 20,0, 20,0
  lda #10
  jsr apirect
  lda API_ERROR
  jsr hex
  ldx #16
  ldy #16
  jsr pixel
  jsr cr

  ldx #<serase              ; EraseRect (20,20)-(30,30) couleur 0
  ldy #>serase
  jsr print
  jsr setrect
  .byte 20,0, 20,0, 30,0, 30,0
  stz API_PARAMETERS+2
  lda #9
  jsr apirect
  lda API_ERROR
  jsr hex
  ldx #20
  ldy #20
  jsr pixel
  jsr cr

  ldx #<spen                ; MoveTo (100,200) puis GetPen
  ldy #>spen
  jsr print
  lda #100
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #200
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #5
  ldx #32
  jsr api
  lda #6
  ldx #32
  jsr api
  ldx #4                    ; copie P0..P4 (wchar écrase P0)
pc:
  lda API_PARAMETERS,x
  sta buf,x
  dex
  bpl pc
  lda buf+1
  jsr hex
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda buf+3
  jsr hex
  lda buf+2
  jsr hex
  lda #' '
  jsr wchar
  lda buf+4
  jsr hex
  jsr cr

  ldx #<sbad                ; rectangle inversé -> 01
  ldy #>sbad
  jsr print
  jsr setrect
  .byte 50,0, 10,0, 10,0, 50,0
  lda #8
  jsr apirect
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<saddr               ; adresse dans la page API -> 01
  ldy #>saddr
  jsr print
  stz API_PARAMETERS
  lda #$FF
  sta API_PARAMETERS+1
  lda #8
  ldx #32
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
  jmp halt

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

; --- 32,3 GetClip -> "CLIP l t r b"
getclip:
  ldx #<sclip
  ldy #>sclip
  jsr print
  lda #3
  jsr apirect
  ldx #0
gc1:
  lda #' '
  jsr wchar
  lda rect+1,x
  phx
  jsr hex
  plx
  lda rect,x
  phx
  jsr hex
  plx
  inx
  inx
  cpx #8
  bne gc1
  jmp cr

; --- lit le pixel (X, Y) de la VRAM par 12,2 et l'affiche " nn" ; adresse = Y*256 + Y*64 + X
pixel:
  stx ptr
  stz ptr+1
  sty buf                   ; buf = Y*64 (16 bits)
  stz buf+1
  ldx #6
p64:
  asl buf
  rol buf+1
  dex
  bne p64
  lda buf
  clc
  adc ptr
  sta ptr
  lda buf+1
  adc ptr+1
  sta ptr+1
  tya                       ; + Y*256
  clc
  adc ptr+1
  sta ptr+1
  lda #$80
  sta API_PARAMETERS
  lda ptr
  sta API_PARAMETERS+1
  lda ptr+1
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<buf
  sta API_PARAMETERS+4
  lda #>buf
  sta API_PARAMETERS+5
  lda #1
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  lda #' '
  jsr wchar
  lda buf
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

sclip:   .text "CLIP", 0
spaint:  .text "PAINT ", 0
sframe:  .text "FRAME ", 0
sinvert: .text "INVERT ", 0
serase:  .text "ERASE ", 0
spen:    .text "PEN ", 0
sbad:    .text "BAD ", 0
saddr:   .text "ADDR ", 0
sfin:    .text "END", 13, 0
