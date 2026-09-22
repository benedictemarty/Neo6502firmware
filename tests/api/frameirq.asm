; frameirq.asm — interruption de trame (F-10 du fork firmware) : 1,16 Set Frame Interrupt,
; 1,17 Get Frame Interrupt. Le gestionnaire IRQ compte les trames ; le programme attend 60
; trames, arrete l'interruption, attend 0,5 s (timer 1,1) et verifie que le compteur n'a plus
; bouge. Sortie attendue (console) : "ON 00 01 FRAMES 003C STOP 00 00 STILL 003C END"
; (01 = 1,17 renvoie « actif » ; second 00 apres STOP = 1,17 renvoie « inactif »).
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=frameirq.neo6502 frameirq.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr      = $F0
frames   = $F2              ; 16 bits, incremente par l'IRQ
t0       = $F4

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  sei
  stz frames
  stz frames+1
  lda #<handler             ; vecteur IRQ ($FFFE en RAM)
  sta $FFFE
  lda #>handler
  sta $FFFF
  lda #12
  jsr wchar
  ldx #<son
  ldy #>son
  jsr print
  lda #1                    ; 1,16 : interruption de trame active
  sta API_PARAMETERS
  lda #16
  ldx #1
  jsr api
  jsr perr
  lda #' '
  jsr wchar
  lda #17                   ; 1,17 -> 01
  ldx #1
  jsr api
  lda API_PARAMETERS
  jsr hex
  lda #' '
  jsr wchar
  cli
wait60:
  lda frames
  cmp #60
  bcc wait60
  sei
  ldx #<sframes
  ldy #>sframes
  jsr print
  lda frames+1
  jsr hex
  lda frames
  jsr hex
  lda #' '
  jsr wchar
  ldx #<sstop
  ldy #>sstop
  jsr print
  stz API_PARAMETERS        ; 1,16 : arret
  lda #16
  ldx #1
  jsr api
  jsr perr
  lda #' '
  jsr wchar
  lda #17                   ; 1,17 -> 00
  ldx #1
  jsr api
  lda API_PARAMETERS
  jsr hex
  lda #' '
  jsr wchar
  cli
  lda #50                   ; 0,5 s sans interruption
  jsr waitcs
  ldx #<sstill
  ldy #>sstill
  jsr print
  lda frames+1
  jsr hex
  lda frames
  jsr hex
  lda #' '
  jsr wchar
  ldx #<send
  ldy #>send
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts                       ; retour à l'appelant (sys du BASIC, ou NeoDOS pour un .NEO) ; reset sinon

handler:                    ; IRQ : compteur seulement (pas d'appel API)
  pha
  inc frames
  bne h1
  inc frames+1
h1:
  pla
  rti

; attend A centiemes de seconde (timer 100 Hz, 1,1 ; A < 128)
waitcs:
  sta t0+1
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta t0
wl:
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  sec
  sbc t0
  cmp t0+1
  bcc wl
  rts

perr:
  lda API_ERROR
  jmp hex

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

son:     .text "ON ", 0
sframes: .text "FRAMES ", 0
sstop:   .text "STOP ", 0
sstill:  .text "STILL ", 0
send:    .text "END", 13, 0
