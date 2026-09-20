; events.asm — Toolbox groupe 33 Event Manager (F-42 du fork firmware) : 33,1 Init, 33,2 Get Next Event,
; 33,3 Event Available, 33,6 Status. Entrées injectées par les émulateurs : souris (200,80) bouton 1 au
; cycle 2 000 000, touches "ab" au cycle 15 000 000 (neo : mouse:/keys: ; Phosphoneo : --mouse-at/--type-keys).
; Sortie attendue (console, capture vers 19 M cycles) :
;   AVAIL 00 / NULL 00 0000 0000 / E 06 00 00 00 00C8 0050 / E 04 01 01 01 00C8 0050
;   E 01 61 04 00 00C8 0050 / E 02 61 04 00 00C8 0050 / E 01 62 05 00 00C8 0050 / E 02 62 05 00 00C8 0050
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=events.neo6502 events.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
rec   = $C00                ; EventRecord (8 octets)

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  stz API_PARAMETERS        ; 33,1 Init Events, tous types
  stz API_PARAMETERS+1
  lda #1
  ldx #33
  jsr api

  ldx #<savail              ; AVAIL 00
  ldy #>savail
  jsr print
  stz API_PARAMETERS
  stz API_PARAMETERS+1
  lda #3
  ldx #33
  jsr api
  lda API_PARAMETERS+2
  jsr hex
  jsr cr

  ldx #<snull               ; NULL : pas d'événement -> enregistrement nul avec la position
  ldy #>snull
  jsr print
  jsr getevent
  lda rec
  jsr hex
  jsr pxy
  jsr cr

loop:                       ; boucle de scrutation : affiche chaque événement
  jsr getevent
  lda API_PARAMETERS+4
  beq loop
  ldx #<se
  ldy #>se
  jsr print
  ldx #0
el:
  lda rec,x
  phx
  jsr hex
  lda #' '
  jsr wchar
  plx
  inx
  cpx #4
  bne el
  jsr pxy
  jsr cr
  bra loop

; --- 33,2 dans rec, tous types
getevent:
  lda #<rec
  sta API_PARAMETERS
  lda #>rec
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #2
  ldx #33
  jmp api

; --- " xxxx yyyy" depuis rec+4
pxy:
  lda #' '
  jsr wchar
  lda rec+5
  jsr hex
  lda rec+4
  jsr hex
  lda #' '
  jsr wchar
  lda rec+7
  jsr hex
  lda rec+6
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

savail: .text "AVAIL ", 0
snull:  .text "NULL ", 0
se:     .text "E ", 0
