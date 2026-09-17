; mode3.asm — mode video 3 (F-57 du fork firmware) : bitmap Hercules 720x350 du mode 1 avec
; console 80x43 en cellules 9x8 (police 8 lignes). Sequence : 5,9 mode 3 ; 5,10 -> "MODE 03 50 2B"
; (mode, colonnes $50 = 80, lignes $2B = 43) ; curseur cache (2,19) ; 'A' en (0,0) et 'Z' en
; (78,42) par 2,7 + 2,6 (pas (79,42) : ecrire la derniere cellule fait defiler la console) ;
; puis "END" en (0,1). Verification image : capture 720x350, cellule (0,0) = glyphe 'A'
; (police interne, 6 colonnes) aux lignes 0-7, cellule (78,42) = 'Z' aux lignes 336-343.
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=mode3.neo6502 mode3.asm

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $B00

start:
  lda #3                    ; 5,9 mode 3
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda #10                   ; 5,10 : P0 mode, P6 colonnes, P7 lignes
  ldx #5
  jsr api
  lda API_PARAMETERS
  sta buf
  lda API_PARAMETERS+6
  sta buf+1
  lda API_PARAMETERS+7
  sta buf+2
  stz API_PARAMETERS        ; 2,19 : curseur cache (capture deterministe)
  lda #19
  ldx #2
  jsr api
  stz API_PARAMETERS        ; 'A' en (0,0)
  stz API_PARAMETERS+1
  jsr setcur
  lda #'A'
  jsr wchar
  lda #78                   ; 'Z' en (78,42)
  sta API_PARAMETERS
  lda #42
  sta API_PARAMETERS+1
  jsr setcur
  lda #'Z'
  jsr wchar
  stz API_PARAMETERS        ; (0,1)
  lda #1
  sta API_PARAMETERS+1
  jsr setcur
  ldx #<smode
  ldy #>smode
  jsr print
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda buf+1
  jsr hex
  lda #' '
  jsr wchar
  lda buf+2
  jsr hex
  lda #' '
  jsr wchar
  ldx #<send
  ldy #>send
  jsr print
halt:
  jmp halt

setcur:
  lda #7
  ldx #2
  jmp api

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

smode: .text "MODE ", 0
send:  .text "END", 0
