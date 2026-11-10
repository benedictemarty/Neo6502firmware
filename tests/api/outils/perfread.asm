; perfread.asm — T-144 (proposition Neo6502AigleDorEngine) : durée de lecture d'une image de 64 000 o de engine.dat
; (L'Aigle d'Or, sur A:), 10 lectures par variante, en centièmes (1,1) : ms par image = résultat (décimal) x 10 / 10.
;   1  un seul 3,27 de 64 000 o vers $80:$0000, position 520 192 (alignée sur 4 096)
;   2  un seul 3,27 de 64 000 o, position 523 575 (celle de la salle 6, non alignée)
;   3  témoin : 15 x 3,27 de 4 096 + 2 560 o, position 520 192 (variante G de salle.asm ; une grappe par appel)
; Sortie : « PERFREAD » puis les trois durées en hexadécimal (16 bits) et l'erreur du dernier appel. Outil carte.
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
t0    = $F4
n     = $F6
ri    = $F8
k     = $F9
err   = $FA
RES   = $0A00

start:
  stz ri
  stz err
  stz API_PARAMETERS        ; mode 0
  lda #9
  ldx #5
  jsr api
  stz API_PARAMETERS        ; 3,4 : canal 0, engine.dat, lecture
  lda #<fname
  sta API_PARAMETERS+1
  lda #>fname
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #3
  jsr api
  lda API_ERROR
  beq ouvert
  ldx #<sopen
  ldy #>sopen
  jmp print
ouvert:
  lda #0                    ; variante 1
  jsr bench1
  lda #1                    ; variante 2
  jsr bench1
  jsr bench3                ; variante 3
  stz API_PARAMETERS        ; 3,5 : fermer
  lda #5
  ldx #3
  jsr api
  stz API_PARAMETERS        ; retour au mode 0 (écran effacé)
  lda #9
  ldx #5
  jsr api
  ldx #<stitre
  ldy #>stitre
  jsr print
  ldx #0
aff:
  lda RES+1,x
  phx
  jsr hex
  plx
  lda RES,x
  phx
  jsr hex
  lda #' '
  jsr wchar
  plx
  inx
  inx
  cpx #6
  bne aff
  lda #'E'
  jsr wchar
  lda err
  jsr hex
  jmp cr

; --- seek : A = 0 position alignée, 1 non alignée
seek:
  tax
  stz API_PARAMETERS
  lda posl,x
  sta API_PARAMETERS+1
  lda posm,x
  sta API_PARAMETERS+2
  lda #$07
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #6
  ldx #3
  jsr api
  lda API_ERROR
  ora err
  sta err
  rts

; --- 3,27 : canal 0, page $80, adresse $0000, taille (X bas, Y haut)
lire:
  stz API_PARAMETERS
  lda #$80
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  stx API_PARAMETERS+4
  sty API_PARAMETERS+5
  lda #27
  ldx #3
  jsr api
  lda API_ERROR
  ora err
  sta err
  rts

bench1:
  sta k
  jsr tnow
  lda #10
  sta n
b1:
  lda k
  jsr seek
  ldx #<64000
  ldy #>64000
  jsr lire
  dec n
  bne b1
  jmp tfin

bench3:
  jsr tnow
  lda #10
  sta n
b3:
  lda #0
  jsr seek
  lda #15
  sta k
b4:
  ldx #0
  ldy #$10
  jsr lire
  dec k
  bne b4
  ldx #0
  ldy #$0A
  jsr lire
  dec n
  bne b3

tfin:
  lda #1
  ldx #1
  jsr api
  ldx ri
  sec
  lda API_PARAMETERS
  sbc t0
  sta RES,x
  lda API_PARAMETERS+1
  sbc t0+1
  sta RES+1,x
  inx
  inx
  stx ri
  rts

tnow:
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta t0
  lda API_PARAMETERS+1
  sta t0+1
  rts

; 520 192 = $07F000 ; 523 575 = $07FD37
posl: .byte $00, $37
posm: .byte $F0, $FD
fname: .ptext "engine.dat"
sopen: .text "ERREUR : engine.dat introuvable", 13, 0

api:
  sta API_FUNCTION
  stx API_COMMAND
wait:
  lda API_COMMAND
  bne wait
  rts

cr:
  lda #13
  ; tombe dans wchar

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

stitre:  .text "PERFREAD (cs/10) : 64000 alignes, 64000 non alignes, 15x4096+2560 ; ", 0
