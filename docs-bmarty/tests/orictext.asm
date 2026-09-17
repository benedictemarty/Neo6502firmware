; orictext.asm — mode 4 (ADR-04 tranche a) : ecran TEXT Oric rendu depuis la RAM 6502.
; Jeu de caracteres minimal a $B400 ('A' et 'B', 6x8, bits 5..0), ecran $BB80 :
;   ligne 0 : [encre rouge=1] "AB" [encre bleue=4] "A"        -> A rouge, B rouge, A bleu
;   ligne 1 : [papier vert=18] "A" puis 'A'|$80 (inverse)     -> A blanc/vert, puis A (7^7=0 noir)/(2^7=5 magenta)
;   ligne 2 : [attr double hauteur=10] "A"  ligne 3 : [10] "A" -> moitie haute puis basse du glyphe
; Puis 5,10 -> console texte "MODE 04 35 1C" ecrite... non : la console n'est pas affichee en
; mode 4 ; le programme repasse en mode 0 apres 60 trames et affiche "MODE 04 35 1C END"
; (valeurs relevees en mode 4 : mode, colonnes 53, lignes 28). La capture PPM au cycle
; 2 000 000 (encore en mode 4) est verifiee pixel par pixel (voir le script de test).
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=orictext.neo6502 orictext.asm

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $B00
SCREEN = $BB80
CHARSET = $B400

start:
  ; jeu de caracteres : 'A' ($41) et 'B' ($42)
  ldx #7
ca:
  lda glyphA,x
  sta CHARSET+$41*8,x
  lda glyphB,x
  sta CHARSET+$42*8,x
  dex
  bpl ca
  ; ecran : efface (espaces)
  lda #<SCREEN
  sta ptr
  lda #>SCREEN
  sta ptr+1
  ldx #5                    ; 5 x 256 > 1120
  ldy #0
  lda #' '
cl:
  sta (ptr),y
  iny
  bne cl
  inc ptr+1
  dex
  bne cl
  ; ligne 0
  lda #1
  sta SCREEN+0              ; encre rouge
  lda #'A'
  sta SCREEN+1
  lda #'B'
  sta SCREEN+2
  lda #4
  sta SCREEN+3              ; encre bleue
  lda #'A'
  sta SCREEN+4
  ; ligne 1
  lda #18
  sta SCREEN+40             ; papier vert (16 + 2)
  lda #'A'
  sta SCREEN+41
  lda #'A'+$80
  sta SCREEN+42             ; inverse
  ; lignes 2 et 3 : double hauteur
  lda #10
  sta SCREEN+80
  lda #'A'
  sta SCREEN+81
  lda #10
  sta SCREEN+120
  lda #'A'
  sta SCREEN+121
  ; mode 4
  lda #4
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda #10
  ldx #5
  jsr api
  lda API_PARAMETERS
  sta buf
  lda API_PARAMETERS+6
  sta buf+1
  lda API_PARAMETERS+7
  sta buf+2
  ; attend ~60 trames (5,37 Frame Count : P0-1)
  lda #37
  ldx #5
  jsr api
  lda API_PARAMETERS
  sta buf+3
wf:
  lda #37
  ldx #5
  jsr api
  lda API_PARAMETERS
  sec
  sbc buf+3
  cmp #70
  bcc wf
  ; retour mode 0 et affichage
  stz API_PARAMETERS
  lda #9
  ldx #5
  jsr api
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

glyphA: .byte %001100,%010010,%100001,%111111,%100001,%100001,%100001,%000000
glyphB: .byte %111110,%100001,%100001,%111110,%100001,%100001,%111110,%000000
smode: .text "MODE ", 0
send:  .text "END", 0
