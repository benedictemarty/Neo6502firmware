; locks.asm — T-41 : les touches de verrouillage rendent leur service. Caps Lock inverse la casse des
; lettres, Num Lock choisit entre les chiffres et la navigation sur le pavé numérique, et les opérateurs
; du pavé (/ * - +) et son Entrée sont enfin lus. Le firmware démarre Num Lock éteint.
; Scancodes HID injectés par le crochet hid: (locks.args) :
;   04 (a) 39 (Caps) 04 (a) 5D (KP5) 60 (KP8) 53 (Num) 5D (KP5) 57 (KP+) 54 (KP/) 58 (KP Entrée) 39 04
; Sortie attendue (7 touches lues, les verrous et les touches muettes n'entrent pas dans la file) :
;   KEYS 61 41 17 35 2B 2F 0D 61 / LOCK 01 / END
;   61 a, 41 A (Caps), 17 curseur haut (KP8, Num éteint), 35 '5' (Num allumé), 2B '+', 2F '/',
;   0D Entrée du pavé, 61 a (Caps relâché) ; LOCK 01 = Num Lock seul (2,23).
; KP5 avec Num Lock éteint n'a rien à donner : il ne produit aucune touche, d'où 8 lectures pour 12 frappes.
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=locks.neo6502 locks.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
cnt   = $E18

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar                 ; CLS

  ldx #<skeys               ; KEYS : les 8 touches lues par 2,1
  ldy #>skeys
  jsr print
  lda #8
  sta cnt
kloop:
  lda #1                    ; 2,1 Read Character (attend une touche)
  ldx #2
  jsr api
  lda API_PARAMETERS
  beq kloop
  jsr hex
  lda #' '
  jsr wchar
  dec cnt
  bne kloop
  jsr cr

  ldx #<slock               ; LOCK : 2,23 Get Lock Keys
  ldy #>slock
  jsr print
  lda #23
  ldx #2
  jsr api
  lda API_PARAMETERS
  jsr hex
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts

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

skeys:  .text "KEYS ", 0
slock:  .text "LOCK ", 0
sfin:   .text "END", 13, 0
