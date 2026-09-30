; copie.asm — T-90 : reproduit la mesure 3c de BLITBENCH (Neo6502POP) sur la carte, sans sonde pour l'affichage.
; Pour chaque mode 0, 1, 2 : 5,9 mode, 5,41 $FF (remise à zéro), copie 12,2 de 8 Ko ($90:0000 -> $80:0000), puis
; tout de suite 5,42 P0=1 (plus longue commande, µs), 2 (plus long rappel de ligne), 3 (pire écart), 4 (rappels en
; retard) ; puis 2 s d'attente et les quatre mêmes mesures. Question : pourquoi BLITBENCH lisait 0, 0, 0 en mode 0.
; Résultats en $0A00 : par mode, 8 mots (juste après la copie, puis après 2 s). Retour au mode 0, fin par RTS.
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

t0    = $F4
mode  = $F6
out   = $F7                 ; index dans RES
RES   = $0A00

start:
  stz out
  stz mode
mloop:
  lda mode                  ; 5,9 mode
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda #$FF                  ; 5,41 $FF : compteurs à zéro
  sta API_PARAMETERS
  lda #41
  ldx #5
  jsr api
  lda #$90                  ; 12,2 : $90:0000 -> $80:0000, 8192 octets (comme BLITBENCH)
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #$80
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  stz API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #$20
  sta API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  jsr grab                  ; tout de suite
  jsr now                   ; puis 2 s
w1:
  jsr since
  cpx #0
  bne w2
  cmp #200
  bcc w1
w2:
  jsr grab
  inc mode
  lda mode
  cmp #3
  bne mloop
  stz API_PARAMETERS        ; retour au mode 0
  lda #9
  ldx #5
  jmp api

; --- relève 5,42 P0=1,2,3,4 dans RES+out (4 mots)
grab:
  lda #1
  jsr t42
  lda #2
  jsr t42
  lda #3
  jsr t42
  lda #4
t42:
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
since:                      ; A/X = horloge - t0 (centièmes)
  lda #1
  ldx #1
  jsr api
  sec
  lda API_PARAMETERS
  sbc t0
  pha
  lda API_PARAMETERS+1
  sbc t0+1
  tax
  pla
  rts

api:
  sta API_FUNCTION
  stx API_COMMAND
wait:
  lda API_COMMAND
  bne wait
  rts
