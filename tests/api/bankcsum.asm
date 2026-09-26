; bankcsum.asm — 1,26 Bank Checksum (Trinity T-81) : Fletcher 16 d'une banque en flash, sans copie.
; Fenêtre $6000-$7FFF remplie de (2y + page) & $FF sauf $12, $56, $34 en +0, +$1000, +$1FFF, écrite en banque 3 par 1,22, puis sommée par 1,26.
; Sortie attendue (console) :
;   WRITE3 00 / CS3 00 <s1> <s2> / CS5 00 E000 F000 / CS31 00 E000 F000 / BAD 01 0000 0000 / END
; (banques 5 et 31 jamais écrites = 8192 x $FF ; BAD = banque 32 ; attendus calculés par tools/assets.py fletcher16
;  de NeoDune2000, même algorithme)
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=bankcsum.neo6502 bankcsum.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $B00
WIN   = $6000

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar

  lda #<WIN                 ; fenêtre = (2y + page) & $FF
  sta ptr
  lda #>WIN
  sta ptr+1
  ldx #$20
  ldy #0
fl1:
  tya
  asl a
  clc
  adc ptr+1
  sta (ptr),y
  iny
  bne fl1
  inc ptr+1
  dex
  bne fl1
  lda #$12                  ; trois octets hors motif : la somme n'est plus symétrique
  sta WIN
  lda #$56
  sta WIN+$1000
  lda #$34
  sta WIN+$1FFF

  ldx #<swrite              ; WRITE3 : 1,22 banque 3 <- fenêtre
  ldy #>swrite
  jsr print
  lda #3
  sta API_PARAMETERS
  jsr setwin
  lda #22
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<scs3
  ldy #>scs3
  jsr print
  lda #3
  jsr csum
  ldx #<scs5
  ldy #>scs5
  jsr print
  lda #5
  jsr csum
  ldx #<scs31
  ldy #>scs31
  jsr print
  lda #31
  jsr csum
  ldx #<sbad
  ldy #>sbad
  jsr print
  lda #32
  jsr csum

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts

; --- 1,26 banque A : affiche "err s1 s2" puis CR
csum:
  sta API_PARAMETERS
  lda #26
  ldx #1
  jsr api
  ldx #3                    ; copie : wchar écrit dans API_PARAMETERS
cs1:
  lda API_PARAMETERS,x
  sta buf,x
  dex
  bpl cs1
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda buf+1
  jsr hex
  lda buf+0
  jsr hex
  lda #' '
  jsr wchar
  lda buf+3
  jsr hex
  lda buf+2
  jsr hex
  jmp cr

; --- P1-2 = WIN
setwin:
  lda #<WIN
  sta API_PARAMETERS+1
  lda #>WIN
  sta API_PARAMETERS+2
  rts

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

swrite:  .text "WRITE3 ", 0
scs3:    .text "CS3 ", 0
scs5:    .text "CS5 ", 0
scs31:   .text "CS31 ", 0
sbad:    .text "BAD ", 0
sfin:    .text "END", 0
