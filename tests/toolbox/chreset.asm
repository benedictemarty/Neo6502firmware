; chreset.asm — 32,19 Toolbox Reset ferme les canaux de fichier de la base (groupe 40) et du fichier de ressources (groupe 38) (Trinity T-142).
; Avant : DBReset / RSReset oubliaient le numéro de canal sans fermer le fichier : un programme qui se terminait sans 40,3 / 38,2
; laissait le canal ouvert pour le programme suivant (sur la carte, FatFs : Create/Open rendaient l'erreur 2, constat de la session
; Neo6502Chess le 2026-11-10 avec la 0.16.98).
; Sortie attendue (console) :
;   CREATE 00 / OPEN 00 / ROPEN 00 / RESET 00 / INFO 01 / RCOUNT 01 / FCLOSE3 15 / FCLOSE4 15 / CREATE 00 / ROPEN 00 /
;   OPEN2 00 / ROPEN2 00 / CREATE2 00 / ROPEN3 00 / END   (phase 2 : sur la carte avant la correction CREATE2 et ROPEN3 rendaient 02)
;   (FCLOSE3 / FCLOSE4 : 3,5 sur un canal déjà fermé -> erreur $15 dans neo ; avant la correction : 00, canal encore ouvert)
; Auteur : bmarty <bmarty@mailo.com>
; SPDX-License-Identifier: EUPL-1.2
;   64tass --mw65c02 --nostart --output=chreset.neo6502 chreset.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800
  jmp start

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
cap   = $E40                ; copie du résultat : erreur

; --- appelle la fonction A du groupe X, copie l'erreur dans cap
callg:
  jsr api
  lda API_ERROR
  sta cap
  rts

; --- affiche le libellé X/Y puis l'erreur
label:
  jsr print
  lda cap
  jsr hex
  lda #13
  jmp wchar

; --- P0 = canal A, P1,2 = nom X/Y
setname:
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  sta API_PARAMETERS
  rts

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  lda #1                    ; 32,1
  ldx #32
  jsr api

  ldx #<fname               ; CREATE : t.db, enregistrements de 8 octets, canal 3
  ldy #>fname
  lda #3
  jsr setname
  lda #8
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #1
  ldx #40
  jsr callg
  ldx #<s_create
  ldy #>s_create
  jsr label

  ldx #<fname               ; OPEN : canal 3
  ldy #>fname
  lda #3
  jsr setname
  lda #2
  ldx #40
  jsr callg
  ldx #<s_open
  ldy #>s_open
  jsr label

  ldx #<fres                ; 38,1 : fichier de ressources test.res, canal 4
  ldy #>fres
  lda #4
  jsr setname
  lda #1
  ldx #38
  jsr callg
  ldx #<s_ropen
  ldy #>s_ropen
  jsr label

  lda #19                   ; 32,19 Toolbox Reset
  ldx #32
  jsr callg
  ldx #<s_reset
  ldy #>s_reset
  jsr label

  lda #4                    ; 40,4 Info : plus de base -> 01
  ldx #40
  jsr callg
  ldx #<s_info
  ldy #>s_info
  jsr label

  lda #3                    ; 38,3 Count : plus de fichier de ressources -> 01
  ldx #38
  jsr callg
  ldx #<s_rcount
  ldy #>s_rcount
  jsr label

  lda #3                    ; 3,5 File Close canal 3
  sta API_PARAMETERS
  lda #5
  ldx #3
  jsr callg
  ldx #<s_fc3
  ldy #>s_fc3
  jsr label

  lda #4                    ; 3,5 File Close canal 4
  sta API_PARAMETERS
  lda #5
  ldx #3
  jsr callg
  ldx #<s_fc4
  ldy #>s_fc4
  jsr label

  ldx #<fname               ; CREATE à nouveau sur le canal 3 -> 00
  ldy #>fname
  lda #3
  jsr setname
  lda #8
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #1
  ldx #40
  jsr callg
  ldx #<s_create
  ldy #>s_create
  jsr label

  ldx #<fres                ; 38,1 à nouveau sur le canal 4 -> 00
  ldy #>fres
  lda #4
  jsr setname
  lda #1
  ldx #38
  jsr callg
  ldx #<s_ropen
  ldy #>s_ropen
  jsr label

  ldx #<fname               ; phase 2 : OPEN canal 3 puis 38,1 canal 4, Toolbox Reset, puis Create / 38,1 DIRECTEMENT (sans 3,5)
  ldy #>fname               ; symptôme de la session Chess sur la carte avant la correction : erreur 02 (FatFs refuse le canal encore ouvert)
  lda #3
  jsr setname
  lda #2
  ldx #40
  jsr callg
  ldx #<s_open2
  ldy #>s_open2
  jsr label
  ldx #<fres
  ldy #>fres
  lda #4
  jsr setname
  lda #1
  ldx #38
  jsr callg
  ldx #<s_ropen2
  ldy #>s_ropen2
  jsr label
  lda #19
  ldx #32
  jsr callg
  ldx #<fname
  ldy #>fname
  lda #3
  jsr setname
  lda #8
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #1
  ldx #40
  jsr callg
  ldx #<s_create2
  ldy #>s_create2
  jsr label
  ldx #<fres
  ldy #>fres
  lda #4
  jsr setname
  lda #1
  ldx #38
  jsr callg
  ldx #<s_ropen3
  ldy #>s_ropen3
  jsr label

  ldx #<s_fin
  ldy #>s_fin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts

api:
  sta API_FUNCTION
  stx API_COMMAND
wait:
  lda API_COMMAND
  bne wait
  rts

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

s_create: .text "CREATE ", 0
s_open:   .text "OPEN ", 0
s_ropen:  .text "ROPEN ", 0
s_reset:  .text "RESET ", 0
s_info:   .text "INFO ", 0
s_rcount: .text "RCOUNT ", 0
s_fc3:    .text "FCLOSE3 ", 0
s_fc4:    .text "FCLOSE4 ", 0
s_open2:  .text "OPEN2 ", 0
s_ropen2: .text "ROPEN2 ", 0
s_create2:.text "CREATE2 ", 0
s_ropen3: .text "ROPEN3 ", 0
s_fin:    .text "END", 13, 0
fname:    .ptext "t.db"
fres:     .ptext "test.res"
