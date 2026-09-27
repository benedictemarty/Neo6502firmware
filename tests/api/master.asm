; master.asm — volume général 8,15 / 8,16 (Trinity T-86). Les touches multimédia (Muet, Volume -, Volume +)
; n'existent pas dans neo : test-snd couvre leur effet, ceci couvre l'API.
; Sortie attendue (console) :
;   GET 10 00 / SET 00 / GET 08 00 / BAD 01 / MUTE 00 / GET 00 01 / SET 00 / GET 10 00 / END
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

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  jsr get
  lda #8                    ; SET 8, son
  ldy #0
  ldx #<sset
  jsr set
  jsr get
  lda #17                   ; BAD : 17 refusé
  ldy #0
  ldx #<sbad
  jsr set
  lda #0                    ; MUTE : 0, coupé
  ldy #1
  ldx #<smute
  jsr set
  jsr get
  lda #16                   ; SET 16, son : état de départ
  ldy #0
  ldx #<sset
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

; --- 8,15 : "GET vv mm"
get:
  ldx #<sget
  ldy #>sget
  jsr print
  lda #15
  ldx #8
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

; --- 8,16 : A = volume, Y = muet, X = étiquette (même page que sset) -> "ETIQ err"
set:
  pha
  phy
  ldy #>sset
  jsr print
  ply
  pla
  sta API_PARAMETERS
  sty API_PARAMETERS+1
  lda #16
  ldx #8
  jsr api
  lda API_ERROR
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

sget:    .text "GET ", 0
sset:    .text "SET ", 0
sbad:    .text "BAD ", 0
smute:   .text "MUTE ", 0
sfin:    .text "END", 0
