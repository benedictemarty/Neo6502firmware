; fdate.asm — 3,29 Last Entry Date Time (Trinity T-91) : date et heure FAT de la dernière entrée lue.
; 3,16 File Stat de readpaged.bin puis 3,29 ; 3,17 Open Directory + 3,18 Read Directory puis 3,29.
; Affiche l'erreur, l'année - 1980 (bits 15-9) et le mois (bits 8-5). Les fichiers de test étant copiés au
; lancement, leur date est celle du jour : attendu en expressions régulières (fdate.expected.re).
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
NBUF  = $B00

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar

  ldx #<sstat               ; STAT : 3,16 readpaged.bin -> 00 ; puis 3,29
  ldy #>sstat
  jsr print
  lda #<fname
  sta API_PARAMETERS
  lda #>fname
  sta API_PARAMETERS+1
  lda #16
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  jsr showdate
  jsr cr

  ldx #<sdir                ; DIR : 3,17 "" puis 3,18 -> 00 ; puis 3,29
  ldy #>sdir
  jsr print
  lda #<empty
  sta API_PARAMETERS
  lda #>empty
  sta API_PARAMETERS+1
  lda #17
  ldx #3
  jsr api
  lda #<NBUF
  sta API_PARAMETERS
  lda #>NBUF
  sta API_PARAMETERS+1
  lda #32
  sta NBUF
  lda #18
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  jsr showdate
  jsr cr
  lda #19                   ; 3,19 Close Directory
  ldx #3
  jsr api

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- 3,29 : " err année-1980 mois"
showdate:
  lda #29
  ldx #3
  jsr api
  lda API_PARAMETERS+1      ; fdate haut : aaaaaaam
  pha
  lda API_PARAMETERS        ; fdate bas  : mmmjjjjj
  pha
  lda #' '
  jsr wchar
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  pla
  sta NBUF+64
  pla
  sta NBUF+65
  lsr a                     ; année - 1980
  jsr hex
  lda #' '
  jsr wchar
  lda NBUF+65               ; mois = bit 0 du haut : 3 bits hauts du bas
  and #1
  sta NBUF+66
  lda NBUF+64
  lsr a
  lsr a
  lsr a
  lsr a
  lsr a
  asl NBUF+66
  asl NBUF+66
  asl NBUF+66
  ora NBUF+66
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

sstat:   .text "STAT ", 0
sdir:    .text "DIR ", 0
sfin:    .text "END", 0
fname:   .ptext "readpaged.bin"
empty:   .byte 0
