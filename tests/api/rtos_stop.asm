; rtos_stop.asm — vecteurs KTaskSelf (T-126) et KTaskStop (T-127) du noyau (demandes D-01 / D-02 de NeoWin).
;   tâche 0 : KTaskSelf avec X=$11, Y=$22 -> A = 00, X et Y inchangés
;   tâche 1 : KTaskSelf avec X=$5A, Y=$A5 -> A = 01, X et Y inchangés, puis KTaskExit
;   tâche 0 : le compteur de ticks avance pendant une attente (témoin), puis KTaskStop :
;             I=1 au retour, rtActive ($FF10) = 0, 1,13 rend 0 Hz, le compteur n'avance plus.
; Sortie attendue : SELF0 00 11 22 / SELF1 01 5A A5 / AVANT +  / STOP I=1 ACT=00 HZ=0000 APRES 0000 / END
; Résultats relevés en RAM pendant que l'ordonnanceur tourne, affichés après KTaskStop (aucun appel API avant).
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=rtos_stop.neo6502 rtos_stop.asm

NEO = 0
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_PARAMETERS = $FF04

KTaskStop   = $FFBB
KTaskSelf   = $FFBE
KTaskTicks  = $FFC7
KTaskExit   = $FFD0
KTaskYield  = $FFD6
KTaskCreate = $FFD9
KTaskInit   = $FFDC
rtActive    = $FF10          ; donnée interne, lue ici seulement pour le test

R = $3000                    ; résultats : self0 a,x,y / self1 a,x,y / fait1 / ticks avant lo,hi / après lo,hi / P / act / hz lo,hi

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  stz R+6
  lda #100                  ; tick 100 Hz
  ldx #0
  jsr KTaskInit
  ldx #$11
  ldy #$22
  jsr KTaskSelf
  sta R+0
  stx R+1
  sty R+2
  lda #<taskA
  ldx #>taskA
  jsr KTaskCreate
wait1:
  jsr KTaskYield            ; jusqu'à ce que la tâche 1 ait relevé son numéro
  lda R+6
  beq wait1
  jsr delta                 ; témoin : le compteur avance quand l'ordonnanceur tourne
  sta R+7
  stx R+8
  jsr KTaskStop
  php
  pla
  sta R+11
  lda rtActive
  sta R+12
  jsr delta                 ; arrêté : le compteur n'avance plus
  sta R+9
  stx R+10
  lda #13                   ; 1,13 Get Interrupt Tick
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta R+13
  lda API_PARAMETERS+1
  sta R+14

  ldx #<s0                  ; affichage (ordonnanceur arrêté : API libre)
  ldy #>s0
  jsr print
  lda R+0
  jsr hexsp
  lda R+1
  jsr hexsp
  lda R+2
  jsr hex
  jsr cr
  ldx #<s1
  ldy #>s1
  jsr print
  lda R+3
  jsr hexsp
  lda R+4
  jsr hexsp
  lda R+5
  jsr hex
  jsr cr
  ldx #<sav
  ldy #>sav
  jsr print
  lda R+7                   ; « + » si le compteur a avancé, « 0 » sinon
  ora R+8
  beq av0
  lda #'+'
  bra av1
av0:
  lda #'0'
av1:
  jsr wchar
  jsr cr
  ldx #<sst
  ldy #>sst
  jsr print
  lda R+11                  ; I (bit 2 de P)
  and #4
  lsr a
  lsr a
  ora #'0'
  jsr wchar
  ldx #<sact
  ldy #>sact
  jsr print
  lda R+12
  jsr hex
  ldx #<shz
  ldy #>shz
  jsr print
  lda R+14
  jsr hex
  lda R+13
  jsr hex
  ldx #<sap
  ldy #>sap
  jsr print
  lda R+10
  jsr hex
  lda R+9
  jsr hex
  jsr cr
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

taskA:
  ldx #$5A
  ldy #$A5
  jsr KTaskSelf
  sta R+3
  stx R+4
  sty R+5
  inc R+6
  jmp KTaskExit

; delta : A/X = compteur de ticks après une attente active (≈ 330 000 cycles) moins celui d'avant.
delta:
  jsr KTaskTicks
  sta R+15
  stx R+16
  ldx #0
  ldy #0
d1:
  dey
  bne d1
  dex
  bne d1
  jsr KTaskTicks
  sec
  sbc R+15
  pha
  txa
  sbc R+16
  tax
  pla
  rts

api:
  sta API_FUNCTION
  stx API_COMMAND
apiw:
  lda API_COMMAND
  bne apiw
  rts

print:
  stx ptr2+2
  sty ptr2+3
  ldy #0
p1:
  lda (ptr2+2),y
  beq pdone
  jsr wchar
  iny
  bra p1
pdone:
  rts

cr:
  lda #13
  jmp wchar

hexsp:
  jsr hex
  lda #' '
  jmp wchar

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
wait:
  lda API_COMMAND
  bne wait
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

s0:   .text "SELF0 ",0
s1:   .text "SELF1 ",0
sav:  .text "AVANT ",0
sst:  .text "STOP I=",0
sact: .text " ACT=",0
shz:  .text " HZ=",0
sap:  .text " APRES ",0
sfin: .text "END",13,0
