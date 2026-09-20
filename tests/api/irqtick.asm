; irqtick.asm — tick d'interruption (Trinity T-14, F-60 du fork) : 1,12 Set Interrupt Tick, 1,13 Get,
; WAI. Le gestionnaire IRQ compte les ticks ; 100 Hz pendant 1 s (timer 1,1) -> environ 100 ticks ;
; WAI rend la main sur le tick suivant ; arret -> compteur immobile ; 2000 Hz -> erreur.
; Sortie attendue (console) : "ON 00 0064 TICKS 0064 WAI 01 STOP 00 0000 STILL 01 BAD 01 END" (TICKS 005x-007x et
; WAI/STILL 02 admis : un tick déjà en attente est pris au CLI, la vitesse de l'hôte varie ; irqtick.expected.re)
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=irqtick.neo6502 irqtick.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F6
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr      = $F0
frames   = $F2              ; 16 bits, incremente par l'IRQ (ticks)
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
  lda #100                  ; 1,12 : 100 Hz
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #12
  ldx #1
  jsr api
  jsr perr
  lda #' '
  jsr wchar
  lda #13                   ; 1,13 -> 0064
  ldx #1
  jsr api
  lda API_PARAMETERS        ; P0 avant hex (wchar écrase P0)
  sta t0
  lda API_PARAMETERS+1
  jsr hex
  lda t0
  jsr hex
  lda #' '
  jsr wchar
  cli
  lda #100                  ; 1 s
  jsr waitcs
  sei
  ldx #<sticks
  ldy #>sticks
  jsr print
  lda frames+1
  jsr hex
  lda frames
  jsr hex
  lda #' '
  jsr wchar
  ldx #<swai                ; WAI : le tick suivant reveille -> frames a augmente de 1
  ldy #>swai
  jsr print
  stz frames
  stz frames+1
  cli
  wai
  sei
  lda frames
  jsr hex
  lda #' '
  jsr wchar
  ldx #<sstop
  ldy #>sstop
  jsr print
  stz API_PARAMETERS        ; 1,12 : arret
  stz API_PARAMETERS+1
  lda #12
  ldx #1
  jsr api
  jsr perr
  lda #' '
  jsr wchar
  lda #13                   ; 1,13 -> 0000
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta t0
  lda API_PARAMETERS+1
  jsr hex
  lda t0
  jsr hex
  lda #' '
  jsr wchar
  cli
  lda #50                   ; 0,5 s sans interruption
  jsr waitcs
  ldx #<sstill
  ldy #>sstill
  jsr print
  lda frames
  jsr hex
  lda #' '
  jsr wchar
  ldx #<sbad                ; 2000 Hz -> 01
  ldy #>sbad
  jsr print
  lda #<2000
  sta API_PARAMETERS
  lda #>2000
  sta API_PARAMETERS+1
  lda #12
  ldx #1
  jsr api
  jsr perr
  lda #' '
  jsr wchar
  ldx #<send
  ldy #>send
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  jmp halt

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
sticks:  .text "TICKS ", 0
swai:    .text "WAI ", 0
sbad:    .text "BAD ", 0
sframes: .text "FRAMES ", 0
sstop:   .text "STOP ", 0
sstill:  .text "STILL ", 0
send:    .text "END", 13, 0
