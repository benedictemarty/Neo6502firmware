; pcm.asm — flux PCM 8,11-8,14 (Trinity T-79) : une sinusoïde de 441 Hz à 22 050 Hz, pour l'oreille.
; 3 s de flux seul, puis 3 s de flux mêlé à une note de 220 Hz du canal 0 (8,4), puis arrêt.
; Affiche le nombre de retards (moitiés atteintes avant d'être remplies) : doit rester 00 00.
; Tampon $4000 : deux moitiés de 1 000 octets (20 périodes chacune), remplies par la boucle de sondage 8,13.
; Échap : sortie anticipée. Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
src   = $F2                 ; indice dans la table (0-49)
cnt   = $F3                 ; compteur 16 bits d'octets restants à écrire
tick0 = $F6                 ; 1,1 au départ (centièmes, octet bas)
phase = $F7                 ; 0 : flux seul, 1 : flux + note
HALF  = 1000
BUF   = $4000

start:
  lda #12
  jsr wchar
  ldx #<stitle
  ldy #>stitle
  jsr print
  stz src
  lda #<BUF                 ; les deux moitiés d'abord
  sta ptr
  lda #>BUF
  sta ptr+1
  jsr fillhalf
  jsr fillhalf
  lda #<BUF                 ; 8,11 : BUF, 1000, 22050 Hz, volume 100
  sta API_PARAMETERS
  lda #>BUF
  sta API_PARAMETERS+1
  lda #<HALF
  sta API_PARAMETERS+2
  lda #>HALF
  sta API_PARAMETERS+3
  lda #<22050
  sta API_PARAMETERS+4
  lda #>22050
  sta API_PARAMETERS+5
  lda #100
  sta API_PARAMETERS+6
  lda #11
  ldx #8
  jsr api
  lda API_ERROR
  beq ok
  ldx #<serr
  ldy #>serr
  jsr print
  rts
ok:
  stz phase
  jsr now
  sta tick0

loop:
  lda #13                   ; 8,13 : moitiés à remplir
  ldx #8
  jsr api
  lda API_PARAMETERS
  and #1
  beq h1
  lda #<BUF
  sta ptr
  lda #>BUF
  sta ptr+1
  jsr fillhalf
  stz API_PARAMETERS
  lda #14
  ldx #8
  jsr api
h1:
  lda #13
  ldx #8
  jsr api
  lda API_PARAMETERS
  and #2
  beq tm
  lda #<(BUF+HALF)
  sta ptr
  lda #>(BUF+HALF)
  sta ptr+1
  jsr fillhalf
  lda #1
  sta API_PARAMETERS
  lda #14
  ldx #8
  jsr api
tm:
  jsr esc                   ; Échap : fin
  bcs fin
  jsr now                   ; 3 s (300 centièmes, octet bas : 44 tours de 256 + ...) : on compte par 1,1 bas
  sec
  sbc tick0
  cmp #150                  ; 1,5 s par demi-phase, deux fois
  bcc loop
  jsr now
  sta tick0
  inc phase
  lda phase
  cmp #2
  bne p3
  jsr note                  ; début de la phase mêlée
p3:
  lda phase
  cmp #4
  bcc loop
fin:
  lda #12                   ; 8,12 puis 8,13 : retards
  ldx #8
  jsr api
  lda #13
  ldx #8
  jsr api
  lda API_PARAMETERS+2
  pha
  lda API_PARAMETERS+1
  pha
  ldx #<sunder
  ldy #>sunder
  jsr print
  pla
  jsr hex
  lda #' '
  jsr wchar
  pla
  jsr hex
  lda #13
  jsr wchar
  lda #1                    ; 8,1 : silence
  ldx #8
  jsr api
  rts

; --- remplit 1000 octets en (ptr) depuis la table, avance ptr
fillhalf:
  lda #<HALF
  sta cnt
  lda #>HALF
  sta cnt+1
fh1:
  ldx src
  lda sine,x
  sta (ptr)
  inx
  cpx #50
  bne fh2
  ldx #0
fh2:
  stx src
  inc ptr
  bne fh3
  inc ptr+1
fh3:
  lda cnt
  bne fh4
  dec cnt+1
fh4:
  dec cnt
  lda cnt
  ora cnt+1
  bne fh1
  rts

; --- 8,4 Queue Sound : canal 0, 220 Hz, 300 centièmes, glissement 0, carré, volume 60
note:
  stz API_PARAMETERS
  lda #<220
  sta API_PARAMETERS+1
  lda #>220
  sta API_PARAMETERS+2
  lda #<300
  sta API_PARAMETERS+3
  lda #>300
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5
  stz API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #4
  ldx #8
  jmp api

; --- 1,1 : horloge en centièmes, octet bas dans A
now:
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  rts

; --- C = 1 si Échap attend (2,1 Read Character, 0 si rien)
esc:
  lda #1
  ldx #2
  jsr api
  lda API_PARAMETERS
  cmp #27
  beq e1
  clc
  rts
e1:
  sec
  rts

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
pl:
  lda (ptr),y
  beq pd
  phy
  jsr wchar
  ply
  iny
  bne pl
pd:
  rts

hex:
  pha
  lsr a
  lsr a
  lsr a
  lsr a
  jsr nib
  pla
  and #15
nib:
  cmp #10
  bcc dg
  adc #6
dg:
  adc #'0'
  jmp wchar

stitle: .text "PCM T-79 : 441 Hz a 22050 Hz, puis + note 220 Hz", 13, 0
serr:   .text "8,11 refuse", 13, 0
sunder: .text "retards : ", 0
sine:
  .byte $80, $8D, $99, $A5, $B0, $BB, $C4, $CD, $D4, $DA
  .byte $DF, $E2, $E4, $E4, $E2, $DF, $DA, $D4, $CD, $C4
  .byte $BB, $B0, $A5, $99, $8D, $80, $73, $67, $5B, $50
  .byte $45, $3C, $33, $2C, $26, $21, $1E, $1C, $1C, $1E
  .byte $21, $26, $2C, $33, $3C, $45, $50, $5B, $67, $73
