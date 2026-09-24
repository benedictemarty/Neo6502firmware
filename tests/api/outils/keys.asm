; keys.asm — T-63 : que reçoit vraiment le firmware quand on tape ?
; Affiche, pour chaque touche lue par 2,1, son code en hexadécimal et le caractère lui-même,
; précédés de l'état des verrous (2,23 : bit 0 Num, 1 Caps, 2 Scroll). De quoi voir d'un coup
; si Num Lock bascule, et ce que produisent les touches du pavé numérique.
;   L=01 5=35 '5'   signifie : Num Lock allumé, touche lue = $35 = le chiffre 5
; Échap : sortie.
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_PARAMETERS = $FF04

ptr   = $F0

start:
  lda #12                   ; CLS
  jsr wchar
  ldx #<shead
  ldy #>shead
  jsr print
  jsr cr

loop:
  lda #1                    ; 2,1 Read Character
  ldx #2
  jsr api
  lda API_PARAMETERS
  beq loop                  ; rien : on attend
  pha                       ; garder le code
  ldx #<slock               ; « L= » puis l'état des verrous
  ldy #>slock
  jsr print
  lda #23                   ; 2,23 Get Lock Keys
  ldx #2
  jsr api
  lda API_PARAMETERS
  jsr hex
  lda #' '
  jsr wchar
  pla                       ; le code lu
  pha
  jsr hex                   ; en hexadécimal
  lda #' '
  jsr wchar
  lda #39                   ; apostrophe
  jsr wchar
  pla
  pha
  cmp #' '                  ; affichable ?
  bcc npr
  jsr wchar
  bra nx
npr:
  lda #'.'                  ; non affichable : un point
  jsr wchar
nx:
  lda #39
  jsr wchar
  jsr cr
  pla
  cmp #27                   ; Échap : sortie
  beq bye
  jmp loop
bye:
  rts

api:
  sta API_FUNCTION
  stx API_COMMAND
wait:
  lda API_COMMAND
  bne wait
  rts

cr:
  lda #13
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

shead:  .text "KEYS : L=verrous, puis code et caractere (Echap sort)", 0
slock:  .text "L=", 0
