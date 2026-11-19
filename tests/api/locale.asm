; locale.asm — 1,28 Get Locale et erreur de 1,6 (Trinity T-149, suite de la demande D-07 de NeoWin).
; GET : lettres rendues par 1,28 (0,0 si aucune locale) ; SET : erreur de 1,6 (00 = bien, 01 = locale inconnue).
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
  jsr get                   ; avant : 00 00
  lda #'F'                  ; FR -> 00 puis 46 52
  ldx #'R'
  jsr set
  jsr get
  lda #'Z'                  ; ZZ inconnue -> 01 puis 00 00
  ldx #'Z'
  jsr set
  jsr get
  lda #'d'                  ; de en minuscules -> 00 puis 44 45
  ldx #'e'
  jsr set
  jsr get
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- 1,6 avec A, X = les deux lettres : "SET err"
set:
  pha
  phx
  ldx #<sset
  ldy #>sset
  jsr print
  plx
  stx API_PARAMETERS+1
  pla
  sta API_PARAMETERS
  lda #6
  ldx #1
  jsr api
  lda API_ERROR
  jsr hex
  jmp cr

; --- 1,28 : "GET xx yy"
get:
  ldx #<sget
  ldy #>sget
  jsr print
  lda #28
  ldx #1
  jsr api
  lda API_PARAMETERS+1
  pha
  lda API_PARAMETERS
  jsr hex
  lda #' '
  jsr wchar
  pla
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

sset:    .text "SET ", 0
sget:    .text "GET ", 0
sfin:    .text "END", 0
