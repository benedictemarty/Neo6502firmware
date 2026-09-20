; evtimer.asm — Event Manager (F-42) : 33,5 Set Timer, masque de 33,2, 33,4 Flush, 33,6 Status.
; Déterministe dans Phosphoneo (TMRRead dérivé des cycles) ; dans neo TMRRead est l'horloge murale.
; Timer 1 toutes les 50 ticks (500 ms) ; on attend 3 événements timer, puis on vérifie le masque
; (les timers seuls : bit 7 = $0080) et le flush. Sortie attendue (Phosphoneo, 3 M cycles) :
;   SET 00 / BAD 01 / T 08 01 / T 08 01 / T 08 01 / MASKED 00 / STATUS 00 00 / END
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=evtimer.neo6502 evtimer.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
cnt   = $F2
rec   = $C00

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  stz API_PARAMETERS        ; 33,1 Init Events
  stz API_PARAMETERS+1
  lda #1
  ldx #33
  jsr api

  ldx #<sset                ; SET : timer 1, 50 ticks
  ldy #>sset
  jsr print
  lda #1
  sta API_PARAMETERS
  lda #50
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #5
  ldx #33
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<sbad                ; BAD : timer 4 -> 01
  ldy #>sbad
  jsr print
  lda #4
  sta API_PARAMETERS
  lda #5
  ldx #33
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  lda #3
  sta cnt
tloop:                      ; 3 événements timer
  jsr getevent
  lda API_PARAMETERS+4
  beq tloop
  ldx #<st
  ldy #>st
  jsr print
  lda rec
  jsr hex
  lda #' '
  jsr wchar
  lda rec+1
  jsr hex
  jsr cr
  dec cnt
  bne tloop

  stz API_PARAMETERS        ; timer 1 arrêté
  lda #1
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #5
  ldx #33
  jsr api

  ldx #<smasked             ; MASKED : masque clavier seul ($0007) -> rien (00)
  ldy #>smasked
  jsr print
  lda #<rec
  sta API_PARAMETERS
  lda #>rec
  sta API_PARAMETERS+1
  lda #7
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #2
  ldx #33
  jsr api
  lda API_PARAMETERS+4
  jsr hex
  jsr cr

  stz API_PARAMETERS        ; 33,4 flush tout
  stz API_PARAMETERS+1
  lda #4
  ldx #33
  jsr api
  ldx #<sstatus             ; STATUS 00 00
  ldy #>sstatus
  jsr print
  lda #6
  ldx #33
  jsr api
  lda API_PARAMETERS
  jsr hex
  lda #' '
  jsr wchar
  lda API_PARAMETERS+1
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

sset:    .text "SET ", 0
sbad:    .text "BAD ", 0
st:      .text "T ", 0
smasked: .text "MASKED ", 0
sstatus: .text "STATUS ", 0
sfin:    .text "END", 13, 0
