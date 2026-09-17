; qdfont.asm — QuickDraw fontes (F-41 incrément 2) : 32,15 Set Font, 32,16 Draw String, 32,17 Text Width,
; 32,18 Get Font Info ; police système 6x8 puis dejavu9.nf1 chargée en RAM graphique par 3,27 ($90:0000).
; Sortie attendue (console) :
;   FONT 08 20 60 / WIDTH 0012 / DRAW 00 1C 0F 00 / LOAD 00 0968 / SETFONT 00 / FONT 0C 20 60
;   WIDTH 0016 / DRAW 00 20 00 0F / BAD 01 / RESET 00 / FONT 08 20 60 / END
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=qdfont.neo6502 qdfont.asm

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $C10

start:
  lda #12
  jsr wchar
  lda #1                    ; 32,1 InitGraf
  ldx #32
  jsr api
  jsr fontinfo              ; FONT 08 20 60
  jsr width                 ; WIDTH 0012 ("Neo" en 6x8)

  ldx #<sdraw               ; DRAW en (10,100) : plume -> 1C ; N : (10,100)=0F, (11,100)=00
  ldy #>sdraw
  jsr print
  lda #10
  ldx #0
  ldy #100
  jsr moveto
  jsr drawstr
  ldx #10
  ldy #100
  jsr pixel
  ldx #11
  ldy #100
  jsr pixel
  jsr cr

  ldx #<sload               ; 3,4 + 3,27 : dejavu9.nf1 -> $90:0000
  ldy #>sload
  jsr print
  stz API_PARAMETERS
  lda #<fname
  sta API_PARAMETERS+1
  lda #>fname
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #3
  jsr api
  stz API_PARAMETERS
  lda #$90
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #$10                  ; 4096 max
  sta API_PARAMETERS+5
  lda #27
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda API_PARAMETERS+5
  jsr hex
  lda API_PARAMETERS+4
  jsr hex
  jsr cr
  stz API_PARAMETERS
  lda #5
  ldx #3
  jsr api

  ldx #<ssetfont            ; SETFONT $90:0000
  ldy #>ssetfont
  jsr print
  lda #$90
  ldx #0
  ldy #0
  jsr setfont
  jsr cr
  jsr fontinfo              ; FONT 0C 20 60
  jsr width                 ; WIDTH 0016

  ldx #<sdraw               ; DRAW en (10,120) : plume -> 20 ; N ligne 2 : (10,122)=00, (11,122)=0F
  ldy #>sdraw
  jsr print
  lda #10
  ldx #0
  ldy #120
  jsr moveto
  jsr drawstr
  ldx #10
  ldy #122
  jsr pixel
  ldx #11
  ldy #122
  jsr pixel
  jsr cr

  ldx #<sbad                ; en-tête invalide ($00:$0800 = le programme) -> 01
  ldy #>sbad
  jsr print
  lda #0
  ldx #<$800
  ldy #>$800
  jsr setfont
  jsr cr

  ldx #<sreset              ; retour police système
  ldy #>sreset
  jsr print
  lda #0
  ldx #0
  ldy #0
  jsr setfont
  jsr cr
  jsr fontinfo              ; FONT 08 20 60

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
  jmp halt

; --- 32,15 : A = page, X/Y = adresse ; affiche l'erreur
setfont:
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  lda #15
  ldx #32
  jsr api
  lda API_ERROR
  jmp hex

; --- 32,18 : "FONT hh ff cc"
fontinfo:
  ldx #<sfont
  ldy #>sfont
  jsr print
  lda #18
  ldx #32
  jsr api
  ldx #2
fi:
  lda API_PARAMETERS,x
  sta buf,x
  dex
  bpl fi
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda buf+1
  jsr hex
  lda #' '
  jsr wchar
  lda buf+2
  jsr hex
  jmp cr

; --- 32,17 : "WIDTH nnnn" pour "Neo"
width:
  ldx #<swidth
  ldy #>swidth
  jsr print
  lda #<stext
  sta API_PARAMETERS
  lda #>stext
  sta API_PARAMETERS+1
  lda #17
  ldx #32
  jsr api
  lda API_PARAMETERS+3
  jsr hex
  lda API_PARAMETERS+2
  jsr hex
  jmp cr

; --- 32,16 "Neo" à la plume, puis affiche l'erreur et X de la plume (8 bits)
drawstr:
  lda #<stext
  sta API_PARAMETERS
  lda #>stext
  sta API_PARAMETERS+1
  lda #16
  ldx #32
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda #6
  ldx #32
  jsr api
  lda API_PARAMETERS
  jmp hex

; --- 32,5 MoveTo : A/X = x, Y = y
moveto:
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #5
  ldx #32
  jmp api

; --- 32,11 LineTo : A/X = x, Y = y
lineto:
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #11
  ldx #32
  jmp api

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

sfont:    .text "FONT ", 0
swidth:   .text "WIDTH ", 0
sdraw:    .text "DRAW ", 0
sload:    .text "LOAD ", 0
ssetfont: .text "SETFONT ", 0
sbad:     .text "BAD ", 0
sreset:   .text "RESET ", 0
sfin:     .text "END", 13, 0
stext:    .ptext "Neo"
fname:    .ptext "dejavu9.nf1"
