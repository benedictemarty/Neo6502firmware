; good.asm — image de démarrage valide pour tests/boot (T-37) : affiche « BOOT OK » puis attend.
; Auteur : bmarty <bmarty@mailo.com>
* = $800
API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_PARAMETERS = $FF04
start:
  ldx #0
l1:
  lda msg,x
  beq idle
  sta API_PARAMETERS
  lda #6
  sta API_FUNCTION
  lda #2
  sta API_COMMAND
w1:
  lda API_COMMAND
  bne w1
  inx
  bra l1
idle:
  bra idle
msg:
  .text 13, "BOOT OK", 13, 0
