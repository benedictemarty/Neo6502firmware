; bgread.asm — 3,28 File Read Background (Trinity T-82, ADR-0002) : contrat de l'octet d'état.
; Fichier readpaged.bin : 1024 octets, octet i = i & $FF. Dans neo la lecture se fait d'un bloc :
; l'état est déjà à $80 au retour (sur la carte, le 6502 tourne pendant les transferts USB).
; Sortie attendue (console) :
;   OPEN 00 / BG 00 80 0200 00 01 02 03 FF / BG2 00 80 0200 00 FF / EOF 00 82 0000 / BAD 15 15 / CLOSE 00 / END
; (API_ERROR ne signale que des octets d'état invalides : la fin de fichier ne se lit que dans l'état, $82)
; Auteur : bmarty <bmarty@mailo.com>

NEO = 0
ptr2   = $F4
logLen = $1FFE

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
ST    = $3000                ; octets d'état
DEST  = $4000

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar

  ldx #<sopen               ; OPEN : 3,4 canal 0
  ldy #>sopen
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
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<sbg                 ; BG : 512 octets -> $4000, état $3000
  ldy #>sbg
  jsr print
  lda #<512
  ldx #>512
  jsr bgread
  jsr showst
  lda #' '
  jsr wchar
  ldx #0
b1:
  lda DEST,x
  jsr hex
  lda #' '
  jsr wchar
  inx
  cpx #4
  bne b1
  lda DEST+$1FF
  jsr hex
  jsr cr

  ldx #<sbg2                ; BG2 : 600 demandés, 512 restants
  ldy #>sbg2
  jsr print
  lda #<600
  ldx #>600
  jsr bgread
  jsr showst
  lda #' '
  jsr wchar
  lda DEST
  jsr hex
  lda #' '
  jsr wchar
  lda DEST+$1FF
  jsr hex
  jsr cr

  ldx #<seof                ; EOF : plus rien -> $82
  ldy #>seof
  jsr print
  lda #<16
  ldx #>16
  jsr bgread
  jsr showst
  jsr cr

  ldx #<sbad                ; BAD : état dans la destination, puis état au-delà de $FF00
  ldy #>sbad
  jsr print
  jsr setp
  lda #<(DEST+$10)
  sta API_PARAMETERS+6
  lda #>(DEST+$10)
  sta API_PARAMETERS+7
  jsr call28
  lda #' '
  jsr wchar
  jsr setp
  lda #$FE
  sta API_PARAMETERS+6
  lda #$FE
  sta API_PARAMETERS+7
  jsr call28
  jsr cr

  ldx #<sclose              ; CLOSE : 3,5
  ldy #>sclose
  jsr print
  stz API_PARAMETERS
  lda #5
  ldx #3
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

; --- paramètres de base : canal 0, page 0, DEST, taille 256, état ST
setp:
  stz API_PARAMETERS
  stz API_PARAMETERS+1
  lda #<DEST
  sta API_PARAMETERS+2
  lda #>DEST
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #1
  sta API_PARAMETERS+5
  lda #<ST
  sta API_PARAMETERS+6
  lda #>ST
  sta API_PARAMETERS+7
  rts

; --- 3,28 puis affiche l'erreur
call28:
  lda #28
  ldx #3
  jsr api
  lda API_ERROR
  jmp hex

; --- 3,28 de A/X octets vers DEST, état ST ; affiche l'erreur, attend le bit 7
bgread:
  pha
  phx
  jsr setp
  plx
  pla
  sta API_PARAMETERS+4
  stx API_PARAMETERS+5
  jsr call28
w7:
  bit ST                    ; attente de la fin, sans appel d'API
  bpl w7
  rts

; --- " état compte"
showst:
  lda #' '
  jsr wchar
  lda ST
  jsr hex
  lda #' '
  jsr wchar
  lda ST+2
  jsr hex
  lda ST+1
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

sopen:   .text "OPEN ", 0
sbg:     .text "BG ", 0
sbg2:    .text "BG2 ", 0
seof:    .text "EOF ", 0
sbad:    .text "BAD ", 0
sclose:  .text "CLOSE ", 0
sfin:    .text "END", 0
fname:   .ptext "readpaged.bin"
