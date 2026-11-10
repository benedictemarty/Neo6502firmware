; blitstr.asm — T-143 (demande Neo6502Tyrann D-01) : 12,3 copie étirée, bit d'action $80 (plus proche voisin).
; Taille d'arrivée = hauteur (octet 9) et largeur (octets 10-11) de la zone CIBLE. Table : table[v] = $FF - v ($3400).
; Source A en $3040 : 01 02 03 04 / 05 06 07 08 (4 x 2, pas 4). Cibles en $3100 + 16 n remplies de $55, pas 5 sauf
; mention. Chaque ligne : « Sn erreur » puis les 10 octets relus (2 lignes de 5).
;   S01  copie A -> 5 x 2                      -> 00 01 02 02 03 04 05 06 06 07 08
;   S02  copie A -> 2 x 1 (réduction)          -> 00 06 08 55 55 55 55 55 55 55 55
;   S03  masque (transparent 02) A -> 5 x 2    -> 00 01 55 55 03 04 05 06 06 07 08
;   S04  plein $AA (transparent 02) -> 5 x 2   -> 00 AA 55 55 AA AA AA AA AA AA AA
;   S05  traduction A -> 5 x 2                 -> 00 FE FD FD FC FB FA F9 F9 F8 F7
;   S06  traduction masquée (transp. 06)       -> 00 FE FD FD FC FB FA 55 55 F8 F7
;   S07  quartets $12 $34, 3 valeurs -> 6 x 1  -> 00 01 01 02 02 03 03 55 55 55 55
;   S08  copie A ligne 0 -> 4 x 1 format 5     -> 00 12 34 55 55 55 55 55 55 55 55
;   S09  largeur cible 0                       -> 01 55 ...
;   S0A cible en $FFF0, 32 de large (déborde) -> 01 55 ...
;   S0B copie A -> 4 x 2, pas cible -5        -> 00 05 06 07 08 55 01 02 03 04 55
;   S0C action $85 (inconnue)                 -> 01 55 ...
;   S0D bits $A0, 4 valeurs -> 8 x 1          -> 00 01 01 00 00 01 01 00 00 55 55
; Auteur : bmarty <bmarty@mailo.com>

NEO = 0
ptr2   = $F4
logLen = $1FFE

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
cas   = $F2                 ; pointeur dans la table des cas
num   = $F6
err   = $F7
SRC   = $3000               ; zones du blitter (12 octets chacune)
TGT   = $300C
DATA  = $3040               ; données source
CIBLE = $3100               ; cibles, remplies de $55
TABLE = $3400               ; table de traduction

start:
  stz logLen
  lda #$20
  sta logLen+1
  ldx #0                    ; table[v] = $FF - v
t1:
  txa
  eor #$FF
  sta TABLE,x
  inx
  bne t1
  ldx #0                    ; données source
d1:
  lda donnees,x
  sta DATA,x
  inx
  cpx #11
  bne d1
  ldx #0                    ; cibles = $55
  lda #$55
d2:
  sta CIBLE,x
  inx
  cpx #$D0
  bne d2
  stz API_PARAMETERS        ; table -> $90:$7F00 (12,2, 256 octets)
  lda #<TABLE
  sta API_PARAMETERS+1
  lda #>TABLE
  sta API_PARAMETERS+2
  lda #$90
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #$7F
  sta API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #1
  sta API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  lda #<cases
  sta cas
  lda #>cases
  sta cas+1
  lda #1
  sta num

boucle:
  ldy #0
  lda (cas),y               ; action, $FF = fin
  cmp #$FF
  bne b1
  jmp fin
b1:
  sta API_PARAMETERS
  iny
z1:                         ; zones : 12 + 12 octets
  lda (cas),y
  sta SRC-1,y
  iny
  cpy #25
  bne z1
  lda #<SRC
  sta API_PARAMETERS+1
  lda #>SRC
  sta API_PARAMETERS+2
  lda #<TGT
  sta API_PARAMETERS+3
  lda #>TGT
  sta API_PARAMETERS+4
  lda (cas),y               ; table : adresse et page
  sta API_PARAMETERS+5
  iny
  lda (cas),y
  sta API_PARAMETERS+6
  iny
  lda (cas),y
  sta API_PARAMETERS+7
  lda #3
  ldx #12
  jsr api
  lda API_ERROR
  sta err
  lda #'S'
  jsr wchar
  lda num
  jsr hex
  lda #' '
  jsr wchar
  lda err
  jsr hex
  ldy #28                   ; adresse à relire
  lda (cas),y
  sta ptr
  iny
  lda (cas),y
  sta ptr+1
  ldy #0
r1:
  lda #' '
  jsr wchar
  lda (ptr),y
  phy
  jsr hex
  ply
  iny
  cpy #10
  bne r1
  jsr cr
  inc num
  clc
  lda cas
  adc #30
  sta cas
  bcc b2
  inc cas+1
b2:
  jmp boucle

fin:
  ldx #<sfin
  ldy #>sfin
  jsr print
.if NEO
  jmp $FFFF
.endif
  rts

; données : $3040 A (8 octets) ; $3048 $12 $34 ; $304A $A0
donnees: .byte 1,2,3,4,5,6,7,8, $12,$34, $A0

; cas : action, zone source (adresse, page, pad, pas, format, transparent, plein, hauteur, largeur),
;       zone cible (adresse, page, pad, pas, format, 2 octets ignorés, hauteur, largeur), table (adresse, page), adresse relue
cases:
  .byte $80, $40,$30,0,0, 4,0, 0,0,0,   2, 4,0,   $00,$31,0,0, 5,0, 0,0,0,2,5,0,  <TABLE,>TABLE,0
  .word CIBLE+$00
  .byte $80, $40,$30,0,0, 4,0, 0,0,0,   2, 4,0,   $10,$31,0,0, 5,0, 0,0,0,1,2,0,  <TABLE,>TABLE,0
  .word CIBLE+$10
  .byte $81, $40,$30,0,0, 4,0, 0,2,0,   2, 4,0,   $20,$31,0,0, 5,0, 0,0,0,2,5,0,  <TABLE,>TABLE,0
  .word CIBLE+$20
  .byte $82, $40,$30,0,0, 4,0, 0,2,$AA, 2, 4,0,   $30,$31,0,0, 5,0, 0,0,0,2,5,0,  <TABLE,>TABLE,0
  .word CIBLE+$30
  .byte $83, $40,$30,0,0, 4,0, 0,0,0,   2, 4,0,   $40,$31,0,0, 5,0, 0,0,0,2,5,0,  <TABLE,>TABLE,0
  .word CIBLE+$40
  .byte $84, $40,$30,0,0, 4,0, 0,6,0,   2, 4,0,   $50,$31,0,0, 5,0, 0,0,0,2,5,0,  <TABLE,>TABLE,0
  .word CIBLE+$50
  .byte $80, $48,$30,0,0, 2,0, 1,0,0,   1, 3,0,   $60,$31,0,0, 8,0, 0,0,0,1,6,0,  <TABLE,>TABLE,0
  .word CIBLE+$60
  .byte $80, $40,$30,0,0, 4,0, 0,0,0,   1, 4,0,   $70,$31,0,0, 5,0, 5,0,0,1,4,0,  <TABLE,>TABLE,0
  .word CIBLE+$70
  .byte $80, $40,$30,0,0, 4,0, 0,0,0,   2, 4,0,   $80,$31,0,0, 5,0, 0,0,0,2,0,0,  <TABLE,>TABLE,0
  .word CIBLE+$80
  .byte $80, $40,$30,0,0, 4,0, 0,0,0,   2, 4,0,   $F0,$FF,0,0, 0,0, 0,0,0,1,32,0, <TABLE,>TABLE,0
  .word CIBLE+$90
  .byte $80, $40,$30,0,0, 4,0, 0,0,0,   2, 4,0,   $A5,$31,0,0, $FB,$FF, 0,0,0,2,4,0,  <TABLE,>TABLE,0
  .word CIBLE+$A0
  .byte $85, $40,$30,0,0, 4,0, 0,0,0,   2, 4,0,   $B0,$31,0,0, 5,0, 0,0,0,2,5,0,  <TABLE,>TABLE,0
  .word CIBLE+$B0
  .byte $80, $4A,$30,0,0, 1,0, 2,0,0,   1, 4,0,   $C0,$31,0,0, 8,0, 0,0,0,1,8,0,  <TABLE,>TABLE,0
  .word CIBLE+$C0
  .byte $FF

sfin:   .text "END", 0

cr:
  lda #13
  jmp wchar

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
