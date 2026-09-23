; busstall.asm — 5,41 Get Bus Stalls (Trinity T-49). Sous `neo` il n'y a pas de machine PIO
; à affamer : les deux compteurs valent 0, et la remise à zéro ($FF en P0) les laisse à 0.
; Sortie attendue : BUS 00000000 00000000 / RESET 00000000 00000000 / END
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $E10

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar

  ldx #<sbus                ; BUS : lecture simple
  ldy #>sbus
  jsr print
  stz API_PARAMETERS
  jsr read41

  ldx #<sreset              ; RESET : $FF en P0 remet à zéro, puis relit
  ldy #>sreset
  jsr print
  lda #$FF
  sta API_PARAMETERS
  jsr read41

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; read41 — appelle 5,41 puis journalise les deux compteurs 32 bits
read41:
  lda #41
  ldx #5
  jsr api
  ldx #7
r1:
  lda API_PARAMETERS,x
  sta buf,x
  dex
  bpl r1
  lda buf+3
  jsr hex
  lda buf+2
  jsr hex
  lda buf+1
  jsr hex
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda buf+7
  jsr hex
  lda buf+6
  jsr hex
  lda buf+5
  jsr hex
  lda buf+4
  jsr hex
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
  pha
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

sbus:   .text "BUS ", 0
sreset: .text "RESET ", 0
sfin:   .text "END", 13, 0
