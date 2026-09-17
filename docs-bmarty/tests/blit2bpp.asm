; blit2bpp.asm — blitter 2 bpp (F-11 du fork firmware) : format source 5 (4 valeurs de 2 bits par
; octet, MSB en premier) dans 12,3 / 12,4, et doublage horizontal (octet 3 de la source, bit 0).
; Source : 2 lignes de 2 octets, $1B $E4 (0 1 2 3 3 2 1 0) puis $E4 $1B.
; Sortie attendue (console) :
;   COPY 00 00 01 02 03 03 02 01 00 / L1 03 02 01 00 00 01 02 03
;   MASK 00 FF 01 02 03 03 02 01 FF / SOLID 00 55 55 55 AA AA 55 55 55
;   DBL 00 00 00 01 01 02 02 03 03 / 03 03 02 02 01 01 00 00
;   DBLBITS 00 01 01 00 00 01 01 00 00 / 00 00 01 01 00 00 01 01   ($A5 = 1 0 1 0 0 1 0 1)
;   DBLSOLID 00 11 11 07 07 07 07 07 07 / 07 07 07 07 07 07 11 11
;   IMGDBL 01 / WIDE 01 / IMG 00 00 00 03 02 01 00 00 00 / END
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=blit2bpp.neo6502 blit2bpp.asm

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
cnt   = $F2
data  = $B00                ; source 2 bpp
bits  = $B10                ; source 1 bpp
tgt   = $C00                ; cible (16 octets par ligne)
srcA  = $D00                ; structure source (12 octets)
tgtA  = $D10                ; structure cible

start:
  lda #12
  jsr wchar
  lda #$1B                  ; données source
  sta data
  lda #$E4
  sta data+1
  sta data+2
  lda #$1B
  sta data+3
  lda #$A5
  sta bits

  ; --- structure cible : $00:tgt, stride 16, format 0
  ldx #11
tz:
  stz tgtA,x
  stz srcA,x
  dex
  bpl tz
  lda #<tgt
  sta tgtA
  lda #>tgt
  sta tgtA+1
  lda #16
  sta tgtA+4
  ; --- structure source : $00:data, stride 2, format 5, transp 0, solid $55, h 2, w 8
  lda #<data
  sta srcA
  lda #>data
  sta srcA+1
  lda #2
  sta srcA+4
  lda #5
  sta srcA+6
  lda #$55
  sta srcA+8
  lda #2
  sta srcA+9
  lda #8
  sta srcA+10

  ldx #<scopy               ; COPY
  ldy #>scopy
  jsr print
  jsr fillff
  lda #0
  jsr blit3
  lda #8
  jsr dump
  ldx #<sl1
  ldy #>sl1
  jsr print
  ldx #16                   ; ligne 1
  ldy #8
  jsr dumpxy

  ldx #<smask               ; MASK (transparent 0) sur $FF
  ldy #>smask
  jsr print
  jsr fillff
  lda #1
  jsr blit3
  lda #8
  jsr dump

  ldx #<ssolid              ; SOLID (transparent 3, solid $55) sur $AA
  ldy #>ssolid
  jsr print
  lda #$AA
  jsr fill
  lda #3
  sta srcA+7
  lda #2
  jsr blit3
  lda #8
  jsr dump
  stz srcA+7

  ldx #<sdbl                ; DBL : doublage, copie
  ldy #>sdbl
  jsr print
  lda #1
  sta srcA+3
  jsr fillff
  lda #0
  jsr blit3
  lda #8
  jsr dump
  ldx #8                    ; suite sur la ligne suivante (console de 53 colonnes)
  ldy #8
  jsr dumpxy

  ldx #<sdblbits            ; DBLBITS : doublage d'une source 1 bpp ($A5)
  ldy #>sdblbits
  jsr print
  lda #<bits
  sta srcA
  lda #>bits
  sta srcA+1
  lda #2
  sta srcA+6
  lda #1
  sta srcA+9
  jsr fillff
  lda #0
  jsr blit3
  lda #8
  jsr dump
  ldx #8                    ; suite sur la ligne suivante (console de 53 colonnes)
  ldy #8
  jsr dumpxy
  lda #<data
  sta srcA
  lda #>data
  sta srcA+1
  lda #5
  sta srcA+6
  lda #2
  sta srcA+9

  ldx #<sdblsolid           ; DBLSOLID : doublage + solid 7 (transparent 0) sur $11
  ldy #>sdblsolid
  jsr print
  lda #$11
  jsr fill
  lda #7
  sta srcA+8
  lda #2
  jsr blit3
  lda #8
  jsr dump
  ldx #8                    ; suite sur la ligne suivante (console de 53 colonnes)
  ldy #8
  jsr dumpxy

  ldx #<simgdbl             ; IMGDBL : 12,4 refuse le doublage
  ldy #>simgdbl
  jsr print
  lda #0
  ldx #10
  ldy #10
  jsr blit4
  jsr cr

  ldx #<swide               ; WIDE : doublage de 2048 valeurs refusé (max 360)
  ldy #>swide
  jsr print
  stz srcA+10
  lda #8
  sta srcA+11
  lda #0
  jsr blit3
  jsr cr
  lda #8
  sta srcA+10
  stz srcA+11
  stz srcA+3

  ldx #<simg                ; IMG : 12,4 en (-2, 230) : clip gauche arrondi à 4 valeurs
  ldy #>simg
  jsr print
  lda #0
  ldx #<-2
  ldy #230
  jsr blit4
  lda #' '
  jsr wchar
  lda #$81                  ; relecture VRAM $81:$1F80 (ligne 230), 8 octets -> tgt
  sta API_PARAMETERS
  lda #$80
  sta API_PARAMETERS+1
  lda #$1F
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<tgt
  sta API_PARAMETERS+4
  lda #>tgt
  sta API_PARAMETERS+5
  lda #8
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  ldx #0
  ldy #8
  jsr dumpxy

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
  jmp halt

; --- 12,3 action A ; affiche l'erreur
blit3:
  sta API_PARAMETERS
  lda #<srcA
  sta API_PARAMETERS+1
  lda #>srcA
  sta API_PARAMETERS+2
  lda #<tgtA
  sta API_PARAMETERS+3
  lda #>tgtA
  sta API_PARAMETERS+4
  lda #3
  ldx #12
  jsr api
  lda API_ERROR
  jmp hex

; --- 12,4 action A en (X, Y) 8 bits signés ; affiche l'erreur
blit4:
  sta API_PARAMETERS
  lda #<srcA
  sta API_PARAMETERS+1
  lda #>srcA
  sta API_PARAMETERS+2
  stx API_PARAMETERS+3
  stz API_PARAMETERS+4
  cpx #$80
  bcc b4p
  dec API_PARAMETERS+4      ; $FF : négatif
b4p:
  sty API_PARAMETERS+5
  stz API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #4
  ldx #12
  jsr api
  lda API_ERROR
  jmp hex

fillff:
  lda #$FF
fill:                       ; remplit les 32 octets de tgt avec A
  ldx #31
fl:
  sta tgt,x
  dex
  bpl fl
  rts

dump:                       ; " " puis A octets de tgt, puis CR
  tay
  ldx #0
dumpxy:                     ; Y octets depuis tgt+X, puis CR
  sty cnt
dl:
  lda #' '
  jsr wchar
  lda tgt,x
  phx
  jsr hex
  plx
  inx
  dec cnt
  bne dl
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

scopy:     .text "COPY ", 0
sl1:       .text "L1", 0
smask:     .text "MASK ", 0
ssolid:    .text "SOLID ", 0
sdbl:      .text "DBL ", 0
sdblbits:  .text "DBLBITS ", 0
sdblsolid: .text "DBLSOLID ", 0
simgdbl:   .text "IMGDBL ", 0
swide:     .text "WIDE ", 0
simg:      .text "IMG ", 0
sfin:      .text "END", 13, 0
