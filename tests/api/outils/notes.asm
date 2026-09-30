; notes.asm — T-95 : justesse des notes à l'oreille (ou à l'accordeur), sur la carte.
; File sur le canal 0 (8,4 Queue Sound) : la 440 Hz, la 880 Hz, la 1 760 Hz, 3 s chacun, puis rend la main
; (la file joue en arrière-plan). Un accordeur doit afficher « la » juste pour les trois ; avant la 0.16.59,
; 1 760 Hz sortait environ un demi-ton trop bas (1 000 Hz mesuré à 965 Hz).
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_PARAMETERS = $FF04

start:
  ldx #0
n1:
  stz API_PARAMETERS          ; canal 0
  lda freq,x
  sta API_PARAMETERS+1
  lda freq+1,x
  sta API_PARAMETERS+2
  lda #<300                   ; 3 s
  sta API_PARAMETERS+3
  lda #>300
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5        ; sans glissement
  stz API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #4
  sta API_FUNCTION
  lda #8
  sta API_COMMAND
w1:
  lda API_COMMAND
  bne w1
  inx
  inx
  cpx #6
  bne n1
  rts

freq: .word 440, 880, 1760
