; son4.asm — T-110 : quatre canaux à plein volume, sans interroger 8,6. Met trois notes dans la file de chaque
; canal (100/200/300/400 Hz, puis 600, puis 1 200 Hz ; 10 s chacune, 1 000 cs), puis rend
; la main : le son joue 30 s pendant que NeoDOS attend au clavier. Somme des quatre canaux écrêtée à ±127,
; c'est l'amplitude maximale de la sortie PWM. Si la carte s'éteint ici, le rythme des appels n'y est pour rien.
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_PARAMETERS = $FF04

start:
  ldy #0                      ; numéro de la note (0, 1, 2)
note:
  ldx #0                      ; canal
canal:
  stx API_PARAMETERS          ; canal
  phx
  tya
  asl
  asl
  asl                         ; y * 8 : ligne de la table
  sta tmp
  txa
  asl                         ; canal * 2
  clc
  adc tmp
  tax
  lda freq,x
  sta API_PARAMETERS+1
  lda freq+1,x
  sta API_PARAMETERS+2
  plx
  lda #<1000                  ; 10 s
  sta API_PARAMETERS+3
  lda #>1000
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5        ; sans glissement
  stz API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #4                      ; 8,4 Queue Sound
  sta API_FUNCTION
  lda #8
  sta API_COMMAND
w1:
  lda API_COMMAND
  bne w1
  inx
  cpx #4
  bne canal
  iny
  cpy #3
  bne note
  rts

tmp: .byte 0
freq: .word 100, 200, 300, 400
      .word 600, 600, 600, 600
      .word 1200, 1200, 1200, 1200
