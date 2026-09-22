; banks.asm — banques en flash (Trinity T-17) : 1,19 Get Bank Info, 1,22 Write Bank, 1,18 Select Bank.
; Fenêtre $6000-$7FFF. Sortie attendue (console) :
;   INFO FF 0000 20 2000 / FILL / WRITE0 00 / ZERO 00 00 / SEL0 00 41 5A / MOD / SEL1 00 FF / SEL0 00 41 5A
;   INFO 00 6000 20 2000 / BAD 01 01 01 01 / UNMAP 00 / INFO FF 0000 20 2000 / END
; (banque 1 jamais écrite = $FF ; SEL0 après MOD = pas de write-back ; BAD = banque 32, adresse non alignée,
;  fenêtre au-delà de $FF00, écriture banque 32)
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=banks.neo6502 banks.asm

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
  jsr info                  ; INFO FF 0000 20 2000

  ldx #<sfill               ; FILL : fenêtre = 'A' + (i & 31), dernier octet 'Z'
  ldy #>sfill
  jsr print
  jsr cr
  lda #<WIN
  sta ptr
  lda #>WIN
  sta ptr+1
  ldx #$20                  ; 32 pages
  ldy #0
fl1:
  tya
  and #31
  clc
  adc #'A'
  sta (ptr),y
  iny
  bne fl1
  inc ptr+1
  dex
  bne fl1
  lda #'Z'
  sta WIN+$1FFF

  ldx #<swrite              ; WRITE0 : 1,22 banque 0 <- fenêtre -> 00
  ldy #>swrite
  jsr print
  stz API_PARAMETERS
  jsr setwin
  lda #22
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<szero               ; ZERO : fenêtre effacée -> 00 00
  ldy #>szero
  jsr print
  lda #<WIN
  sta ptr
  lda #>WIN
  sta ptr+1
  ldx #$20
  ldy #0
  lda #0
z1:
  sta (ptr),y
  iny
  bne z1
  inc ptr+1
  dex
  bne z1
  jsr showwin

  ldx #<ssel0               ; SEL0 : 1,18 banque 0 -> 00 41 5A
  ldy #>ssel0
  jsr print
  lda #0
  jsr select
  jsr showwin

  ldx #<smod                ; MOD : on modifie la fenêtre (pas de write-back attendu)
  ldy #>smod
  jsr print
  jsr cr
  lda #'X'
  sta WIN
  sta WIN+$1FFF

  ldx #<ssel1               ; SEL1 : banque 1 jamais écrite -> 00 FF (FF)
  ldy #>ssel1
  jsr print
  lda #1
  jsr select
  lda WIN
  jsr hex
  jsr cr

  ldx #<ssel0               ; SEL0 : contenu d'origine -> 00 41 5A
  ldy #>ssel0
  jsr print
  lda #0
  jsr select
  jsr showwin

  jsr info                  ; INFO 00 6000 20 2000

  ldx #<sbad                ; BAD : banque 32 / adresse $6080 / fenêtre $FE00 / écriture banque 32 -> 01 01 01 01
  ldy #>sbad
  jsr print
  lda #32
  sta API_PARAMETERS
  jsr setwin
  lda #18
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  stz API_PARAMETERS
  lda #$80
  sta API_PARAMETERS+1
  lda #$60
  sta API_PARAMETERS+2
  lda #18
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  stz API_PARAMETERS
  stz API_PARAMETERS+1
  lda #$FE
  sta API_PARAMETERS+2
  lda #18
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda #32
  sta API_PARAMETERS
  jsr setwin
  lda #22
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<sunmap              ; UNMAP : 1,18 $FF -> 00
  ldy #>sunmap
  jsr print
  lda #$FF
  sta API_PARAMETERS
  lda #18
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr
  jsr info                  ; INFO FF 0000 20 2000

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts                       ; retour à l'appelant (sys du BASIC, ou NeoDOS pour un .NEO) ; reset sinon

; --- P1-2 = WIN
setwin:
  lda #<WIN
  sta API_PARAMETERS+1
  lda #>WIN
  sta API_PARAMETERS+2
  rts

; --- 1,18 banque A dans WIN, affiche " err"
select:
  sta API_PARAMETERS
  jsr setwin
  lda #18
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jmp wchar

; --- " premier dernier" octets de la fenêtre puis CR
showwin:
  lda WIN
  jsr hex
  lda #' '
  jsr wchar
  lda WIN+$1FFF
  jsr hex
  jmp cr

; --- "INFO bank addr count size" : 1,19
info:
  ldx #<sinfo
  ldy #>sinfo
  jsr print
  lda #19
  ldx #1
  jsr api
  ldx #5
in1:
  lda API_PARAMETERS,x
  sta buf,x
  dex
  bpl in1
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda buf+2
  jsr hex
  lda buf+1
  jsr hex
  lda #' '
  jsr wchar
  lda buf+3
  jsr hex
  lda #' '
  jsr wchar
  lda buf+5
  jsr hex
  lda buf+4
  jsr hex
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

sinfo:   .text "INFO ", 0
sfill:   .text "FILL", 0
swrite:  .text "WRITE0 ", 0
szero:   .text "ZERO 00 ", 0
ssel0:   .text "SEL0 ", 0
ssel1:   .text "SEL1 ", 0
smod:    .text "MOD", 0
sbad:    .text "BAD ", 0
sunmap:  .text "UNMAP ", 0
sfin:    .text "END", 0
