; confont.asm — 2,21 Set Console Font (F-95 du fork firmware) : police 8 lignes des
; caracteres $20-$7F lue dans la RAM 6502 (96 x 8 octets, MSB = pixel gauche).
; Sequence :  CLS ; "AB ab" ; FONT <adresse> (police de test : 'A' = bloc plein 6 px,
; 'B' = damier, autres = glyphe vide) -> tout le texte deja affiche est repeint ;
; "AB" reimprime ; BAD = adresse $FE00 (deborde la RAM) refusee ; RESET (adresse 0)
; -> police interne, "AB" reimprime ; FONT a nouveau (etat final = police de test,
; pour la capture).
; Sortie attendue (console texte) : ligne 0 "AB ab FONT 00 AB BAD 01 RESET 00 AB FONT 00 END"
; (le texte console garde les codes ASCII : seule la capture PPM montre les glyphes).
; Verification image : cellules 'A' pleines (6x8 allumes), autres lettres vides.
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=confont.neo6502 confont.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
font  = $1000              ; 768 octets : glyphes $20-$7F

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  ldx #<sab
  ldy #>sab
  jsr print
  jsr buildfont
  ldx #<sfont
  ldy #>sfont
  jsr print
  lda #<font                ; 2,21 P0,1 = adresse de la police
  sta API_PARAMETERS
  lda #>font
  sta API_PARAMETERS+1
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
  stz API_PARAMETERS        ; $FE00 : 768 octets deborderaient $FFFF
  lda #$FE
  sta API_PARAMETERS+1
  jsr setfont
  jsr perr
  lda #' '
  jsr wchar
  ldx #<sreset
  ldy #>sreset
  jsr print
  stz API_PARAMETERS        ; 0 = police interne
  stz API_PARAMETERS+1
  jsr setfont
  jsr perr
  lda #' '
  jsr wchar
  ldx #<sab2
  ldy #>sab2
  jsr print
  ldx #<sfont
  ldy #>sfont
  jsr print
  lda #<font                ; police de test a nouveau : etat final pour la capture
  sta API_PARAMETERS
  lda #>font
  sta API_PARAMETERS+1
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
  jmp halt

setfont:
  lda #21
  ldx #2
  jmp api

; police de test : tout vide sauf 'A' (bloc 6 px plein, 8 lignes) et 'B' (damier)
buildfont:
  lda #<font
  sta ptr
  lda #>font
  sta ptr+1
  ldx #3                    ; 3 x 256 octets
  ldy #0
  lda #0
bf0:
  sta (ptr),y
  iny
  bne bf0
  inc ptr+1
  dex
  bne bf0
  ldy #7
bfa:
  lda #$FC                  ; 'A' : 6 colonnes pleines
  sta font+('A'-$20)*8,y
  tya
  and #1
  beq bfb1
  lda #$A8                  ; 'B' : damier (lignes impaires)
  bne bfb2
bfb1:
  lda #$54                  ; lignes paires
bfb2:
  sta font+('B'-$20)*8,y
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
sfont:  .text "FONT ", 0
sab2:   .text "AB ", 0
sbad:   .text "BAD ", 0
sreset: .text "RESET ", 0
send:   .text "END", 13, 0
