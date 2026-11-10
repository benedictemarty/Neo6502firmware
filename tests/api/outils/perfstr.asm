; perfstr.asm — T-143 (demande Neo6502Tyrann D-01) : durée de la copie étirée de 12,3 (bit $80), en centièmes (1,1),
; pour 100 appels. Outil carte. Texture 64 x 64 (8 bits) en $90:$4000, cible en mode 0 à (70,40) ($80:$3246, pas 320).
; Mesures (16 bits chacune, en $0A00..) : 179 x 143 copie ($80), masque ($81, transparent 0), traduction ($83),
; traduction masquée ($84), puis 53 x 43 copie. Temps d'un appel en ms = résultat / 10.
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
RES   = $0A00
TEX   = $4000               ; texture en RAM 6502, copiée en $90:$4000
TABLE = $5000
SRC   = $5100
TGT   = $510C

start:
  stz ri
  stz API_PARAMETERS        ; mode 0
  lda #9
  ldx #5
  jsr api
  lda #<TEX                 ; texture : octet = (ligne eor colonne) | 1 (jamais 0)
  sta ptr
  lda #>TEX
  sta ptr+1
  ldx #0                    ; x = ligne (0-63)
tx1:
  ldy #0
tx2:
  tya
  stx n
  eor n
  ora #1
  sta (ptr),y
  iny
  cpy #64
  bne tx2
  clc
  lda ptr
  adc #64
  sta ptr
  bcc tx3
  inc ptr+1
tx3:
  inx
  cpx #64
  bne tx1
  ldx #0                    ; table[v] = v eor $0F
tb1:
  txa
  eor #$0F
  sta TABLE,x
  inx
  bne tb1
  stz API_PARAMETERS        ; 12,2 : TEX (page 0) -> $90:$4000, 4096 octets
  lda #<TEX
  sta API_PARAMETERS+1
  lda #>TEX
  sta API_PARAMETERS+2
  lda #$90
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #$40
  sta API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #$10
  sta API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  ldx #23                   ; zones source et cible
zc:
  lda zones,x
  sta SRC,x
  dex
  bpl zc
  lda #$80
  jsr bench
  lda #$81
  jsr bench
  lda #$83
  jsr bench
  lda #$84
  jsr bench
  lda #43                   ; façade la plus petite : 53 x 43
  sta TGT+9
  lda #53
  sta TGT+10
  lda #$80
  jsr bench
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
  cpx #10
  bne aff
  lda #'E'
  jsr wchar
  lda API_ERROR
  jsr hex
  jsr cr
  rts

; --- A = action : 100 appels de 12,3, RES[ri] = durée en centièmes (16 bits)
bench:
  sta act
  jsr tnow
  lda #100
  sta n
bl:
  lda act
  sta API_PARAMETERS
  lda #<SRC
  sta API_PARAMETERS+1
  lda #>SRC
  sta API_PARAMETERS+2
  lda #<TGT
  sta API_PARAMETERS+3
  lda #>TGT
  sta API_PARAMETERS+4
  lda #<TABLE
  sta API_PARAMETERS+5
  lda #>TABLE
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #3
  ldx #12
  jsr api
  dec n
  bne bl
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

act:  .byte 0
; source : $90:$4000, pas 64, format 0, transparent 0, plein 0, 64 x 64
; cible  : $80:$3246, pas 320, format 0, -, -, hauteur 143, largeur 179
zones:
  .byte $00,$40,$90,0, 64,0, 0, 0,0, 64, 64,0
  .byte $46,$32,$80,0, $40,$01, 0, 0,0, 143, 179,0

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

stitre:  .text 12, "PERFSTR (cs/100) : copie, masque, trad, trad-m 179x143 ; copie 53x43", 13, 0
