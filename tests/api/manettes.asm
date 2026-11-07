; manettes.asm — groupe 7 : 7,2 Get Controller Count / 7,3 Read Controller (manettes USB).
; Sortie (console et journal $2000..) : "N cc" (nombre de manettes), "BAD ee" (7,3 avec l'index
; cc+1 : erreur 1 attendue), puis :
;   - aucune manette (cc = 00, cas de neo et de la carte sans manette) : "END" ;
;   - au moins une manette (test INTERACTIF, carte + manette branchee) : chaque changement d'etat de la
;     manette 1 est affiche (" hh", bits : 01 gauche, 02 droite, 04 haut, 08 bas, 10 rond, 20 croix,
;     40 triangle, 80 carre) ; le test finit par " END" quand les QUATRE boutons 10 20 40 80 sont
;     appuyes ensemble (etat F0). Pas de delai : a rejouer a la main (fichier .nocarte).
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=manettes.neo6502 manettes.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
cnt   = $B00
last  = $B01

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  ldx #<sn
  ldy #>sn
  jsr print
  lda #2                    ; 7,2 : nombre de manettes
  ldx #7
  jsr api
  lda API_PARAMETERS
  sta cnt
  jsr hex
  lda #' '
  jsr wchar
  ldx #<sbad
  ldy #>sbad
  jsr print
  lda cnt                   ; 7,3 avec l'index cnt+1 : erreur
  inc a
  sta API_PARAMETERS
  lda #3
  ldx #7
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda cnt
  beq fin                   ; aucune manette : fini
  lda #$FF
  sta last                  ; valeur impossible : le premier etat est affiche
poll:
  lda #1                    ; manette 1
  sta API_PARAMETERS
  lda #3
  ldx #7
  jsr api
  lda API_PARAMETERS
  cmp last
  beq poll
  sta last
  pha
  lda #' '
  jsr wchar
  pla
  pha
  jsr hex
  pla
  cmp #$F0
  bne poll
  lda #' '
  jsr wchar
fin:
  ldx #<send
  ldy #>send
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts                       ; retour à l'appelant (sys du BASIC, ou NeoDOS pour un .NEO) ; reset sinon

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

sn:    .text "N ", 0
sbad:  .text "BAD ", 0
send:  .text "END", 13, 0
