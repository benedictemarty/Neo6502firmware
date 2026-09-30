; csum.asm — T-81 : durée de 1,26 Bank Checksum sur la carte, sans sonde pour l'affichage.
; Pour chaque banque 0-31 : 5,41 $FF (remise à zéro), 1,26 banque, puis 5,42 P0=1 (plus longue commande API, µs).
; Puis les 32 banques d'affilée, mesurées au timer 1,1 (centièmes de seconde).
; Résultats en $0A00 : 32 mots (µs par banque), puis le mot du total (centièmes). Fin par RTS.
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

t0    = $F4
bank  = $F6
out   = $F7                 ; index dans RES
RES   = $0A00

start:
  stz out
  stz bank
b1:
  lda #$FF                  ; 5,41 $FF : compteurs à zéro
  sta API_PARAMETERS
  lda #41
  ldx #5
  jsr api
  lda bank                  ; 1,26 banque
  sta API_PARAMETERS
  lda #26
  ldx #1
  jsr api
  lda #1                    ; 5,42 P0=1 : plus longue commande (µs)
  sta API_PARAMETERS
  lda #42
  ldx #5
  jsr api
  ldx out
  lda API_PARAMETERS
  sta RES,x
  lda API_PARAMETERS+1
  sta RES+1,x
  inx
  inx
  stx out
  inc bank
  lda bank
  cmp #32
  bne b1

  jsr now                   ; les 32 d'affilée
  stz bank
b2:
  lda bank
  sta API_PARAMETERS
  lda #26
  ldx #1
  jsr api
  inc bank
  lda bank
  cmp #32
  bne b2
  lda #1
  ldx #1
  jsr api
  sec
  lda API_PARAMETERS
  sbc t0
  sta RES+64
  lda API_PARAMETERS+1
  sbc t0+1
  sta RES+65
  rts

now:
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta t0
  lda API_PARAMETERS+1
  sta t0+1
  rts

api:
  sta API_FUNCTION
  stx API_COMMAND
wait:
  lda API_COMMAND
  bne wait
  rts
