; datetime.asm — 1,20 Get Date Time / 1,21 Set Date Time (F-14 du fork firmware).
; Sequence : 1,20 -> "SRC ss" (source : 2 = PCF8563 present — cas de neo, qui modelise le RTC
; a $51 sur l'heure de l'hote ; 1 = horloge logicielle ; 0 = non reglee) ; 1,21 2026-09-17
; 12:34:56 -> "SET 00" ; 1,20 -> "GET 07EA 09 11 0C 22 38" (annee $07EA, mois, jour, heure,
; minute, seconde en hexa ; la seconde peut valoir 38 ou 39) ; 1,21 mois 13 -> "BAD 01" ; END.
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=datetime.neo6502 datetime.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $B00

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  ldx #<ssrc
  ldy #>ssrc
  jsr print
  lda #20
  ldx #1
  jsr api
  lda API_PARAMETERS+7
  jsr hex
  lda #' '
  jsr wchar
  ldx #<sset
  ldy #>sset
  jsr print
  jsr fill                  ; 2026-09-17 12:34:56
  lda #21
  ldx #1
  jsr api
  jsr perr
  lda #' '
  jsr wchar
  ldx #<sget
  ldy #>sget
  jsr print
  lda #20
  ldx #1
  jsr api
  ldx #7                    ; copie des parametres (wchar ecrase P0)
cp:
  lda API_PARAMETERS,x
  sta buf,x
  dex
  bpl cp
  lda buf+1
  jsr hex
  lda buf
  jsr hex
  ldx #2
gl:
  lda #' '
  jsr wchar
  lda buf,x
  jsr hex
  inx
  cpx #7
  bne gl
  lda #' '
  jsr wchar
  ldx #<sbad
  ldy #>sbad
  jsr print
  jsr fill
  lda #13                   ; mois 13
  sta API_PARAMETERS+2
  lda #21
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

fill:
  lda #<2026
  sta API_PARAMETERS
  lda #>2026
  sta API_PARAMETERS+1
  lda #9
  sta API_PARAMETERS+2
  lda #17
  sta API_PARAMETERS+3
  lda #12
  sta API_PARAMETERS+4
  lda #34
  sta API_PARAMETERS+5
  lda #56
  sta API_PARAMETERS+6
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

ssrc:  .text "SRC ", 0
sset:  .text "SET ", 0
sget:  .text "GET ", 0
sbad:  .text "BAD ", 0
send:  .text "END", 13, 0
