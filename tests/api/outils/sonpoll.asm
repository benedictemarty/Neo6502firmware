; sonpoll.asm — T-110 : interroge 8,6 Sound Status (canal 0) en boucle serrée pendant environ une minute
; (16 × 65 536 appels), SANS jouer de note, puis rend la main. Reproduit le rythme d'appels de Civ (~15 000/s)
; sans aucun son. Si la carte s'éteint ici, la sortie audio n'y est pour rien.
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_PARAMETERS = $FF04

start:
  lda #16
  sta tours
boucle:
  stz API_PARAMETERS          ; canal 0
  lda #6                      ; 8,6 Sound Status
  sta API_FUNCTION
  lda #8
  sta API_COMMAND
w1:
  lda API_COMMAND
  bne w1
  inc cpt
  bne boucle
  inc cpt+1
  bne boucle
  dec tours
  bne boucle
  rts

cpt:   .word 0
tours: .byte 0
