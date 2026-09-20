; quickdraw2.asm — QuickDraw (F-41 incrément 1) : 32,11 Line To, 32,12 Set Pattern, 32,13 Fill Rect,
; 32,14 Copy Bits, tous clippés (clip (10,100)-(50,140), hors du texte console). Pixels relus par 12,2.
; Sortie attendue (console) :
;   HLINE 00 00 0F 00 / DIAG 00 0F 00 / FILL 00 03 04 04 / COPY 00 00 13 14 23 / OUT 00 00
;   SOLID 00 09 00 09 / BADACT 01 / END
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=quickdraw2.neo6502 quickdraw2.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
rect  = $C00
buf   = $C10
pat   = $C20
data  = $C30
area  = $C40

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  lda #1                    ; 32,1 InitGraf
  ldx #32
  jsr api
  jsr setrect               ; clip (10,100)-(50,140), sous le texte console
  .byte 10,0, 100,0, 50,0, 140,0
  lda #2
  jsr apirect

  ldx #<shline              ; HLINE : (0,110) -> (319,110)
  ldy #>shline
  jsr print
  lda #0
  ldx #0
  ldy #110
  jsr moveto
  lda #<319
  ldx #>319
  ldy #110
  jsr lineto
  lda API_ERROR
  jsr hex
  ldx #5
  ldy #110
  jsr pixel                 ; 00 (hors clip)
  ldx #20
  ldy #110
  jsr pixel                 ; 0F
  ldx #60
  ldy #110
  jsr pixel                 ; 00
  jsr cr

  ldx #<sdiag               ; DIAG : (20,110) -> (30,120)
  ldy #>sdiag
  jsr print
  lda #20
  ldx #0
  ldy #110
  jsr moveto
  lda #30
  ldx #0
  ldy #120
  jsr lineto
  lda API_ERROR
  jsr hex
  ldx #25
  ldy #115
  jsr pixel                 ; 0F
  ldx #26
  ldy #115
  jsr pixel                 ; 00
  jsr cr

  ldx #<sfill               ; FILL : damier $AA/$55, plume 3, fond 4, rect (16,106)-(32,122)
  ldy #>sfill
  jsr print
  ldx #0
fp:
  lda #$AA
  sta pat,x
  lda #$55
  sta pat+1,x
  inx
  inx
  cpx #8
  bne fp
  lda #<pat
  sta API_PARAMETERS
  lda #>pat
  sta API_PARAMETERS+1
  lda #12
  ldx #32
  jsr api
  lda #3
  sta API_PARAMETERS
  lda #4
  ldx #32
  jsr api
  jsr setrect
  .byte 16,0, 106,0, 32,0, 122,0
  lda #4
  sta API_PARAMETERS+2
  lda #13
  jsr apirect
  lda API_ERROR
  jsr hex
  ldx #16
  ldy #106
  jsr pixel                 ; 03
  ldx #17
  ldy #106
  jsr pixel                 ; 04
  ldx #16
  ldy #107
  jsr pixel                 ; 04
  jsr cr

  ; --- source 4x2 octets : 11 12 13 14 / 21 22 23 24 ; structure : $00:data, stride 4, format 0, transp $12, solid 9, h 2, w 4
  lda #$11
  sta data
  lda #$12
  sta data+1
  lda #$13
  sta data+2
  lda #$14
  sta data+3
  lda #$21
  sta data+4
  lda #$22
  sta data+5
  lda #$23
  sta data+6
  lda #$24
  sta data+7
  ldx #11
az:
  stz area,x
  dex
  bpl az
  lda #<data
  sta area
  lda #>data
  sta area+1
  lda #4
  sta area+4
  lda #$12
  sta area+7
  lda #9
  sta area+8
  lda #2
  sta area+9
  lda #4
  sta area+10

  ldx #<scopy               ; COPY en (8,102) : clip gauche -> octets 13 14 en x=10,11
  ldy #>scopy
  jsr print
  lda #0
  ldx #8
  ldy #102
  jsr copybits
  lda API_ERROR
  jsr hex
  ldx #9
  ldy #102
  jsr pixel                 ; 00
  ldx #10
  ldy #102
  jsr pixel                 ; 13
  ldx #11
  ldy #102
  jsr pixel                 ; 14
  ldx #10
  ldy #103
  jsr pixel                 ; 23
  jsr cr

  ldx #<sout                ; OUT : entièrement hors clip (100,190)
  ldy #>sout
  jsr print
  lda #0
  ldx #100
  ldy #190
  jsr copybits
  lda API_ERROR
  jsr hex
  ldx #100
  ldy #190
  jsr pixel                 ; 00
  jsr cr

  ldx #<ssolid              ; SOLID en (20,130) : transparent $12 -> 09 00 09
  ldy #>ssolid
  jsr print
  lda #2
  ldx #20
  ldy #130
  jsr copybits
  lda API_ERROR
  jsr hex
  ldx #20
  ldy #130
  jsr pixel
  ldx #21
  ldy #130
  jsr pixel
  ldx #22
  ldy #130
  jsr pixel
  jsr cr

  ldx #<sbadact             ; action 3 -> 01
  ldy #>sbadact
  jsr print
  lda #3
  ldx #20
  ldy #130
  jsr copybits
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  jmp halt

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

; --- 32,14 CopyBits : A = action, X = x, Y = y (8 bits)
copybits:
  sta API_PARAMETERS
  lda #<area
  sta API_PARAMETERS+1
  lda #>area
  sta API_PARAMETERS+2
  stx API_PARAMETERS+3
  stz API_PARAMETERS+4
  sty API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #14
  ldx #32
  jmp api

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

shline:  .text "HLINE ", 0
sdiag:   .text "DIAG ", 0
sfill:   .text "FILL ", 0
scopy:   .text "COPY ", 0
sout:    .text "OUT ", 0
ssolid:  .text "SOLID ", 0
sbadact: .text "BADACT ", 0
sfin:    .text "END", 13, 0
