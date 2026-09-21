; rtos.asm — ordonnanceur préemptif du noyau (F-61) : trois tâches.
;   tâche 0 (principale) : toutes les 50 ticks, affiche "T=" + compteur de ticks
;   tâche 1 : affiche 'A' puis dort 10 ticks ; tâche 2 : attend le sémaphore, affiche
;   'B' ; tâche 0 signale le sémaphore tous les 25 ticks. Les appels API sont entre
;   KTaskLock/KTaskUnlock. Déterministe dans les émulateurs (tick = cycles).
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=rtos.neo6502 rtos.asm

NEO = 0                     ; (pas de fin : démo infinie, run_neo.sh coupe par cycles: dans NOM.args)
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_PARAMETERS = $FF04

KSemSignal  = $FFC1
KSemWait    = $FFC4
KTaskTicks  = $FFC7
KTaskUnlock = $FFCA
KTaskLock   = $FFCD
KTaskExit   = $FFD0
KTaskSleep  = $FFD3
KTaskYield  = $FFD6
KTaskCreate = $FFD9
KTaskInit   = $FFDC

lastShown = $E0             ; fenêtre ZP privée de la tâche 0 ($E0-$EF)

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  stz lastShown
  stz lastShown+1
  lda #100                  ; tick 100 Hz (KTaskInit d'abord : il remet la table des tâches à zéro)
  ldx #0
  jsr KTaskInit
  lda #<taskA
  ldx #>taskA
  jsr KTaskCreate
  lda #<taskB
  ldx #>taskB
  jsr KTaskCreate
main:
  jsr KTaskTicks            ; A/X = ticks
  sec
  sbc lastShown
  tay
  txa
  sbc lastShown+1
  bne show
  cpy #50
  bcc main
show:
  jsr KTaskTicks
  sta lastShown
  stx lastShown+1
  jsr KTaskLock
  lda #'T'
  jsr wchar
  lda #'='
  jsr wchar
  lda lastShown+1
  jsr hex
  lda lastShown
  jsr hex
  lda #' '
  jsr wchar
  jsr KTaskUnlock
  lda #<sem                 ; réveille B (une fois sur deux : tous les 50 ticks ici)
  ldx #>sem
  jsr KSemSignal
  bra main

taskA:
  jsr KTaskLock
  lda #'A'
  jsr wchar
  jsr KTaskUnlock
  lda #10
  jsr KTaskSleep
  bra taskA

taskB:
  lda #<sem
  ldx #>sem
  jsr KSemWait
  jsr KTaskLock
  lda #'B'
  jsr wchar
  jsr KTaskUnlock
  bra taskB

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

sem: .byte 0
