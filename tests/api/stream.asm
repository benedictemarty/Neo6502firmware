; stream.asm — flux PCM 8,11-8,14 (Trinity T-79) : paramètres refusés, démarrage, état, arrêt, 8,1 qui arrête.
; Seuls les bits qui ne dépendent pas du temps réel sont affichés (bit 7 de 8,13 : le flux joue ; la sortie
; son de neo tourne à son propre rythme, les moitiés rendues ne sont pas déterministes ici — test-snd les couvre).
; Sortie attendue (console) :
;   BAD 01 01 01 01 / FIL 01 / START 00 80 / FIL 00 01 / STOP 00 / START 00 80 / RESET 00 / END
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=stream.neo6502 stream.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
BUF   = $4000                ; deux moitiés de 256 octets

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  ldx #0                    ; tampon = $80 (silence)
  lda #$80
fl:
  sta BUF,x
  sta BUF+$100,x
  inx
  bne fl

  ldx #<sbad                ; BAD : moitié 0, cadence 0, volume 101, tampon au-delà de $FF00
  ldy #>sbad
  jsr print
  jsr setok
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  jsr startp
  jsr setok
  stz API_PARAMETERS+4
  stz API_PARAMETERS+5
  jsr startp
  jsr setok
  lda #101
  sta API_PARAMETERS+6
  jsr startp
  jsr setok
  lda #$FE
  sta API_PARAMETERS+1
  jsr startp
  jsr cr

  ldx #<sfil                ; FIL : 8,14 flux arrêté -> 01
  ldy #>sfil
  jsr print
  stz API_PARAMETERS
  lda #14
  ldx #8
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<sstart              ; START : 8,11 -> 00, 8,13 bit 7 -> 80
  ldy #>sstart
  jsr print
  jsr setok
  jsr startp
  jsr status
  jsr cr

  ldx #<sfil                ; FIL : moitié 0 -> 00, moitié 2 -> 01
  ldy #>sfil
  jsr print
  stz API_PARAMETERS
  lda #14
  ldx #8
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda #2
  sta API_PARAMETERS
  lda #14
  ldx #8
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<sstop               ; STOP : 8,12 puis 8,13 -> 00
  ldy #>sstop
  jsr print
  lda #12
  ldx #8
  jsr api
  lda #13
  ldx #8
  jsr api
  lda API_PARAMETERS
  jsr hex
  jsr cr

  ldx #<sstart              ; START de nouveau, puis RESET : 8,1 -> 8,13 = 00
  ldy #>sstart
  jsr print
  jsr setok
  jsr startp
  jsr status
  jsr cr
  ldx #<sreset
  ldy #>sreset
  jsr print
  lda #1
  ldx #8
  jsr api
  lda #13
  ldx #8
  jsr api
  lda API_PARAMETERS
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

; --- paramètres valides : BUF, moitiés de 256, 22050 Hz, volume 50
setok:
  lda #<BUF
  sta API_PARAMETERS
  lda #>BUF
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #1
  sta API_PARAMETERS+3
  lda #<22050
  sta API_PARAMETERS+4
  lda #>22050
  sta API_PARAMETERS+5
  lda #50
  sta API_PARAMETERS+6
  rts

; --- 8,11 -> affiche "err "
startp:
  lda #11
  ldx #8
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jmp wchar

; --- 8,13 -> affiche P0 & $80
status:
  lda #13
  ldx #8
  jsr api
  lda API_PARAMETERS
  and #$80
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

sbad:    .text "BAD ", 0
sfil:    .text "FIL ", 0
sstart:  .text "START ", 0
sstop:   .text "STOP ", 0
sreset:  .text "RESET ", 0
sfin:    .text "END", 0
