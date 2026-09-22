; confont14.asm — 2,21 Set Console Font, 2e adresse (F-95 2/2) : police 14 lignes des
; cellules 9x14 du mode Hercules (5,9 mode 1). Police de test a $1000 : 'A' = 8 colonnes
; pleines sur 14 lignes, 'B' = damier, autres glyphes vides.
; Sequence : mode 1 ; "AB ab " ; FONT14 (P0-1 = 0 : police 8 lignes interne, P2-3 = $1000)
; -> repeint ; "AB " ; BAD (P2-3 = $FB00 : 1344 octets deborderaient) ; END.
; Sortie attendue (console texte, 80 colonnes) : "AB ab FONT14 00 AB BAD 01 END"
; Verification image (720x350, cellule 9x14, marge (720-80*9)/2 = 0) : cellule (0,0)
; 'A' = 8 colonnes allumees x 14 lignes, 9e colonne eteinte ; cellule (4,0) espace vide.
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=confont14.neo6502 confont14.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
font  = $1000              ; 1344 octets : glyphes $20-$7F, 14 lignes

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #1                    ; 5,9 mode 1 (Hercules 720x350, 80x25)
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  ldx #<sab
  ldy #>sab
  jsr print
  jsr buildfont
  ldx #<sfont
  ldy #>sfont
  jsr print
  stz API_PARAMETERS        ; P0-1 = 0 : police 8 lignes inchangee
  stz API_PARAMETERS+1
  lda #<font                ; P2-3 = police 14 lignes
  sta API_PARAMETERS+2
  lda #>font
  sta API_PARAMETERS+3
  jsr setfont
  jsr perr
  lda #' '
  jsr wchar
  ldx #<sab2
  ldy #>sab2
  jsr print
  ldx #<sbad
  ldy #>sbad
  jsr print
  stz API_PARAMETERS
  stz API_PARAMETERS+1
  stz API_PARAMETERS+2      ; $FB00 + 1344 > $FFFF
  lda #$FB
  sta API_PARAMETERS+3
  jsr setfont
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
  rts                       ; retour à l'appelant (sys du BASIC, ou NeoDOS pour un .NEO) ; reset sinon

setfont:
  lda #21
  ldx #2
  jmp api

; police de test 14 lignes : tout vide sauf 'A' (8 colonnes pleines) et 'B' (damier)
buildfont:
  lda #<font
  sta ptr
  lda #>font
  sta ptr+1
  ldx #6                    ; 6 x 256 = 1536 >= 1344
  ldy #0
  lda #0
bf0:
  sta (ptr),y
  iny
  bne bf0
  inc ptr+1
  dex
  bne bf0
  ldy #13
bfa:
  lda #$FF
  sta font+('A'-$20)*14,y
  tya
  and #1
  beq bfb1
  lda #$AA
  bne bfb2
bfb1:
  lda #$55
bfb2:
  sta font+('B'-$20)*14,y
  dey
  bpl bfa
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

sab:    .text "AB ab ", 0
sfont:  .text "FONT14 ", 0
sab2:   .text "AB ", 0
sbad:   .text "BAD ", 0
send:   .text "END", 13, 0
