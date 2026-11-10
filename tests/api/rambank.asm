; rambank.asm — banque en RAM (Trinity T-124) : banque $80 en SRAM, mêmes fonctions que les banques en flash (1,18, 1,22, 1,26).
; Fenêtre $6000-$7FFF. Sortie attendue (console) :
;   RAM0 00 FF FF / FILL / WRITE 00 / ZERO 00 00 00 / SEL 00 41 5A / FLASH0 00 FF FF / CSUM 00 0FFA 5FFA (Fletcher 16 du motif) / 00 (1,18 $80) / INFO 80 6000 20 2000 /
;   BAD 01 01 01 / UNMAP 00 / END
; (banque RAM jamais écrite = $FF ; WRITE ne touche pas la flash : la banque 0 reste $FF ; BAD = banque $81, adresse non
;  alignée en écriture, fenêtre au-delà de $FF00 en lecture)
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=rambank.neo6502 rambank.asm

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
RAMB  = $80

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar

  ldx #<sram0               ; RAM0 : banque RAM jamais écrite -> 00 FF FF
  ldy #>sram0
  jsr print
  lda #RAMB
  jsr select
  jsr showwin

  ldx #<sfill               ; FILL : fenêtre = 'A' + (i & 31), dernier octet 'Z'
  ldy #>sfill
  jsr print
  jsr cr
  lda #<WIN
  sta ptr
  lda #>WIN
  sta ptr+1
  ldx #$20
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

  ldx #<swrite              ; WRITE : 1,22 banque $80 <- fenêtre -> 00
  ldy #>swrite
  jsr print
  lda #RAMB
  sta API_PARAMETERS
  jsr setwin
  lda #22
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<szero               ; ZERO : fenêtre effacée -> 00 00 00
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

  ldx #<ssel                ; SEL : 1,18 banque $80 -> 00 41 5A
  ldy #>ssel
  jsr print
  lda #RAMB
  jsr select
  jsr showwin

  ldx #<sflash0             ; FLASH0 : la banque 0 en flash n'a pas été touchée -> 00 FF FF
  ldy #>sflash0
  jsr print
  lda #0
  jsr select
  jsr showwin

  ldx #<scsum               ; CSUM : 1,26 banque $80
  ldy #>scsum
  jsr print
  lda #RAMB
  jsr csum

  lda #RAMB                 ; la fenêtre est celle de la banque $80 avant INFO
  jsr select
  jsr cr
  jsr info                  ; INFO 80 6000 20 2000

  ldx #<sbad                ; BAD : banque $81 / écriture adresse $6080 / lecture fenêtre $FE00 -> 01 01 01
  ldy #>sbad
  jsr print
  lda #$81
  sta API_PARAMETERS
  jsr setwin
  lda #18
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda #RAMB
  sta API_PARAMETERS
  lda #$80
  sta API_PARAMETERS+1
  lda #$60
  sta API_PARAMETERS+2
  lda #22
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda #RAMB
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #$FE
  sta API_PARAMETERS+2
  lda #18
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

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- 1,26 banque A : affiche "err s1 s2" puis CR
csum:
  sta API_PARAMETERS
  lda #26
  ldx #1
  jsr api
  ldx #3
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
sram0:   .text "RAM0 ", 0
sfill:   .text "FILL", 0
swrite:  .text "WRITE ", 0
szero:   .text "ZERO 00 ", 0
ssel:    .text "SEL ", 0
sflash0: .text "FLASH0 ", 0
scsum:   .text "CSUM ", 0
sbad:    .text "BAD ", 0
sunmap:  .text "UNMAP ", 0
sfin:    .text "END", 0
