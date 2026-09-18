; latin1.asm — F-17 du fork firmware : locale FR (AZERTY PC, 1,6) et caractères Latin-1 ($A0-$BF en flash,
; $C0-$FF = police utilisateur initialisée aux lettres accentuées, 2,5 la remplace), console 2,6 et QuickDraw 32,16.
; Sortie attendue (console, après « Locale is now 'fr' » et CLS) :
;   ACC é è ç à ù ° ² £ § µ É Ç (glyphes) / PIX 02 00 / KEYS E9 E8 E7 E0 B2 33 6D / QD 0F 0F / END
; Touches injectées (scancodes US) : 2 7 9 0 ` # ;  -> é è ç à ² 3 m
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=latin1.neo6502 latin1.asm

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $E10
cnt   = $E18

start:
  lda #'F'                  ; 1,6 locale FR
  sta API_PARAMETERS
  lda #'R'
  sta API_PARAMETERS+1
  lda #6
  ldx #1
  jsr api
  lda #12
  jsr wchar
  ldx #<sacc                ; ligne 0 : "ACC " puis les caractères Latin-1 (colonne 4 = é)
  ldy #>sacc
  jsr print
  jsr cr

  ldx #<spix                ; PIX : é en colonne 4, ligne 0 : rangée 0 = "...X." -> pixel (1+24+3, 0) = encre ; (1+24, 0) = 0
  ldy #>spix
  jsr print
  ldx #28
  ldy #0
  jsr pixel
  ldx #25
  ldy #0
  jsr pixel
  jsr cr

  ldx #<skeys               ; KEYS : 7 touches lues par 2,1
  ldy #>skeys
  jsr print
  lda #7
  sta cnt
kloop:
  lda #1
  ldx #2
  jsr api
  lda API_PARAMETERS
  beq kloop
  jsr hex
  lda #' '
  jsr wchar
  dec cnt
  bne kloop
  jsr cr

  ; UDG : 2,5 redéfinit $E9 (rangée 0 = $F8), vérifié par QD ci-dessous
  lda #$E9
  sta API_PARAMETERS
  ldx #0
uloop:
  lda #$F8
  sta API_PARAMETERS+1,x
  inx
  cpx #7
  bne uloop
  lda #5
  ldx #2
  jsr api
  ldx #<sqd                 ; QD : 32,16 dessine è en (100,100), plume 15 : rangée 0 = ".X..." -> pixel (101,100) = 0F ; puis é redéfini
  ldy #>sqd
  jsr print
  lda #1
  ldx #32
  jsr api
  lda #15
  sta API_PARAMETERS
  lda #4
  ldx #32
  jsr api
  lda #100
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #100
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #5
  ldx #32
  jsr api
  lda #<sqdtxt
  sta API_PARAMETERS
  lda #>sqdtxt
  sta API_PARAMETERS+1
  lda #16
  ldx #32
  jsr api
  ldx #101
  ldy #100
  jsr pixel
  lda #100                  ; é redéfini ($F8 en rangée 0) en (100,120) -> pixel (100,120) = 0F
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #120
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #5
  ldx #32
  jsr api
  lda #<sqdtxt2
  sta API_PARAMETERS
  lda #>sqdtxt2
  sta API_PARAMETERS+1
  lda #16
  ldx #32
  jsr api
  ldx #100
  ldy #120
  jsr pixel
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
  jmp halt

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

sacc:   .text "ACC ", $E9, $E8, $E7, $E0, $F9, $B0, $B2, $A3, $A7, $B5, $C9, $C7, 0
spix:   .text "PIX", 0
skeys:  .text "KEYS ", 0
sudg:   .text "UDG", 0
sqd:    .text "QD", 0
sqdtxt: .ptext $E8
sqdtxt2: .ptext $E9
sfin:   .text "END", 13, 0
