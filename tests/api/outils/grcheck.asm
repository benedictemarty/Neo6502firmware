; grcheck.asm — T-144 : les octets d'un 3,27 de 64 000 o (plusieurs grappes en un disk_read) sont-ils ceux du fichier ?
; Pour deux positions de engine.dat (520 192 alignée, 523 575 non alignée) : un 3,27 de 64 000 o vers $80:$0000, puis
; 16 x (3,6 + 3,8 de 4 000 o vers $3000, 12,2 de $80:i*4000 vers $4000, comparaison). Sortie : « GRCHECK p écarts »
; pour chaque position (écarts = nombre de morceaux de 4 000 o différents, en hexadécimal) puis l'erreur cumulée.
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
ptr2  = $F2
k     = $F6
pk    = $F7
err   = $FA
diff  = $FB
off   = $FC                 ; 16 bits : décalage dans l'image
A1    = $3000
A2    = $4000

start:
  stz err
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
  beq ok1
  ldx #<sopen
  ldy #>sopen
  jmp print
ok1:
  ldx #<stitre
  ldy #>stitre
  jsr print
  lda #0
  jsr check
  lda #1
  jsr check
  stz API_PARAMETERS
  lda #5
  ldx #3
  jsr api
  lda #'E'
  jsr wchar
  lda err
  jsr hex
  jmp cr

check:
  sta pk
  stz diff
  lda #0                    ; un seul 3,27 de 64 000 o
  sta off
  sta off+1
  jsr seek
  stz API_PARAMETERS
  lda #$80
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<64000
  sta API_PARAMETERS+4
  lda #>64000
  sta API_PARAMETERS+5
  lda #27
  ldx #3
  jsr api
  jsr erreur
  lda #16
  sta k
c1:
  jsr seek                  ; position + off
  stz API_PARAMETERS        ; 3,8 : 4 000 o vers A1
  lda #<A1
  sta API_PARAMETERS+1
  lda #>A1
  sta API_PARAMETERS+2
  lda #<4000
  sta API_PARAMETERS+3
  lda #>4000
  sta API_PARAMETERS+4
  lda #8
  ldx #3
  jsr api
  jsr erreur
  lda #$80                  ; 12,2 : $80:off -> page 0 A2, 4 000 o
  sta API_PARAMETERS
  lda off
  sta API_PARAMETERS+1
  lda off+1
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<A2
  sta API_PARAMETERS+4
  lda #>A2
  sta API_PARAMETERS+5
  lda #<4000
  sta API_PARAMETERS+6
  lda #>4000
  sta API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  jsr erreur
  lda #<A1                  ; comparaison de 4 000 octets
  sta ptr
  lda #>A1
  sta ptr+1
  lda #<A2
  sta ptr2
  lda #>A2
  sta ptr2+1
  ldx #16                   ; 16 pages = 4 096 >= 4 000 : compare 15 pages + 160 octets
  ldy #0
cmp1:
  lda (ptr),y
  cmp (ptr2),y
  bne diffe
  iny
  bne cmp1
  inc ptr+1
  inc ptr2+1
  dex
  cpx #1
  bne cmp1
cmp2:                       ; dernière page : 4000 - 15*256 = 160 octets
  lda (ptr),y
  cmp (ptr2),y
  bne diffe
  iny
  cpy #160
  bne cmp2
  bra suite
diffe:
  inc diff
suite:
  clc
  lda off
  adc #<4000
  sta off
  lda off+1
  adc #>4000
  sta off+1
  dec k
  beq c9
  jmp c1
c9:
  lda #'P'
  jsr wchar
  lda pk
  jsr hex
  lda #' '
  jsr wchar
  lda diff
  jsr hex
  lda #' '
  jmp wchar

erreur:
  lda API_ERROR
  ora err
  sta err
  rts

; seek à pos[pk] + off (pos sur 24 bits : $07F000 ou $07FD37)
seek:
  stz API_PARAMETERS
  ldx pk
  clc
  lda posl,x
  adc off
  sta API_PARAMETERS+1
  lda posm,x
  adc off+1
  sta API_PARAMETERS+2
  lda #$07
  adc #0
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #6
  ldx #3
  jsr api
  jmp erreur

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

stitre:  .text "GRCHECK ", 0
