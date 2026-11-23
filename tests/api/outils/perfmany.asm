; perfmany.asm — T-160/T-161 : gain de 6,13 et coût du zoom. 6 sprites 24 x 32 (6,7, 4 bpp), 100 trames, chacun déplacé
; d'un pixel à chaque trame (x alterne), en centièmes (1,1) ; budget à 50 Hz = 200 cs.
; 6 cas, dans cet ordre : 0 six 6,2 x1 (OU exclusif) ; 1 un 6,13 x1 ; 2 six 6,2 x2 ; 3 un 6,13 x2 ;
; 4 six 6,2 x1 octet entier (zone $90:2000) ; 5 six 6,2 x2 octet entier. Écartés (dx 40, ou 50 en x2).
; Résultats : à l'écran et en $0C00 (6 mots). Outil carte.
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr = $F0
t0  = $F4
f   = $F6
si  = $F7
dx  = $F8
ri  = $FA
cs  = $FB
xl  = $FC
xh  = $FD
GFX = $3300
LST = $3500
RES = $0C00

start:
  stz ri
  stz cs
  lda #1
  ldx #6
  jsr api
  lda #12
  jsr wchar
  ldx #0
p0:
  lda #$12
  sta GFX,x
  lda #$34
  sta GFX+$100,x
  inx
  bne p0
  stz API_PARAMETERS        ; 12,2 : GFX -> $90:0100, $0180 octets
  lda #<GFX
  sta API_PARAMETERS+1
  lda #>GFX
  sta API_PARAMETERS+2
  lda #$90
  sta API_PARAMETERS+3
  lda #$00
  sta API_PARAMETERS+4
  lda #$01
  sta API_PARAMETERS+5
  lda #$80
  sta API_PARAMETERS+6
  lda #$01
  sta API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
cas:
  lda #1
  ldx #6
  jsr api
  lda #$90
  sta API_PARAMETERS
  lda #8
  ldx #6
  jsr api
  ldx cs
  lda tmode,x
  sta API_PARAMETERS
  lda #6
  ldx #6
  jsr api
  ldx cs
  lda tmode,x
  cmp #2
  bne nosave
  stz API_PARAMETERS
  lda #$20
  sta API_PARAMETERS+1
  lda #$00
  sta API_PARAMETERS+2
  lda #$50
  sta API_PARAMETERS+3
  lda #9
  ldx #6
  jsr api
nosave:
  ldx cs
  lda tdx,x
  sta dx
  jsr mklist                ; listes A et B en $90:0400 / $0420
  stz si
sp7:
  lda si
  sta API_PARAMETERS
  lda #$00
  sta API_PARAMETERS+1
  lda #$01
  sta API_PARAMETERS+2
  lda #24
  sta API_PARAMETERS+3
  lda #32
  sta API_PARAMETERS+4
  lda #4
  sta API_PARAMETERS+5
  lda #$FF
  sta API_PARAMETERS+6
  sta API_PARAMETERS+7
  lda #7
  ldx #6
  jsr api
  jsr place                 ; 6,2 : position de départ et zoom
  inc si
  lda si
  cmp #6
  bne sp7
  jsr tnow
  stz f
tr:
  ldx cs
  lda t13,x
  beq six
  lda f                     ; un seul 6,13 : liste A ou B selon la parité
  and #1
  tax
  lda #<$0400
  cpx #0
  beq la
  lda #<$0420
la:
  sta API_PARAMETERS
  lda #>$0400
  sta API_PARAMETERS+1
  lda #6
  sta API_PARAMETERS+2
  lda #13
  ldx #6
  jsr api
  bra suite
six:
  stz si
s2:
  jsr move
  inc si
  lda si
  cmp #6
  bne s2
suite:
  inc f
  lda f
  cmp #100
  bne tr
  lda ri
  asl a
  tax
  jsr tdelta
  inc ri
  stz si
h0:
  lda si
  sta API_PARAMETERS
  lda #3
  ldx #6
  jsr api
  inc si
  lda si
  cmp #6
  bne h0
  inc cs
  lda cs
  cmp #6
  beq fin
  jmp cas
fin:
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
  cpx #12
  bne aff
  lda #13
  jsr wchar
  rts

; x de base du sprite si : 16 + si*dx -> xl/xh
xbase:
  lda #16
  ldx si
xb:
  cpx #0
  beq xd
  clc
  adc dx
  dex
  bra xb
xd:
  sta xl
  stz xh
  rts

; 6,2 : sprite si, x = base + (f & 1), y = 10, image $80, retournement = zoom du cas, ancre 7
place:
  stz f
move:
  jsr xbase
  lda f
  and #1
  clc
  adc xl
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #10
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda si
  sta API_PARAMETERS
  lda #$80
  sta API_PARAMETERS+5
  ldx cs
  lda tzoom,x
  sta API_PARAMETERS+6
  lda #$47
  sta API_PARAMETERS+7
  lda #2
  ldx #6
  jmp api

; listes de 6 entrées (sprite, X bas, X haut, Y bas, Y haut) : A en LST, B en LST+$20 -> $90:0400 et $0420
mklist:
  stz si
  ldy #0
ml:
  jsr xbase
  lda si
  sta LST,y
  sta LST+$20,y
  lda xl
  sta LST+1,y
  clc
  adc #1
  sta LST+$21,y
  lda #0
  sta LST+2,y
  sta LST+$22,y
  lda #10
  sta LST+3,y
  sta LST+$23,y
  lda #0
  sta LST+4,y
  sta LST+$24,y
  tya
  clc
  adc #5
  tay
  inc si
  lda si
  cmp #6
  bne ml
  stz API_PARAMETERS        ; 12,2 : LST -> $90:0400, 30 octets
  lda #<LST
  sta API_PARAMETERS+1
  lda #>LST
  sta API_PARAMETERS+2
  lda #$90
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #4
  sta API_PARAMETERS+5
  lda #30
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  stz API_PARAMETERS
  lda #<(LST+$20)
  sta API_PARAMETERS+1
  lda #>(LST+$20)
  sta API_PARAMETERS+2
  lda #$90
  sta API_PARAMETERS+3
  lda #$20
  sta API_PARAMETERS+4
  lda #4
  sta API_PARAMETERS+5
  lda #30
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  stz si
  rts

tmode: .byte 0,0,0,0,2,2
tzoom: .byte 0,0,4,4,0,4
tdx:   .byte 40,40,50,50,40,50
t13:   .byte 0,1,0,1,0,0

tnow:
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta t0
  lda API_PARAMETERS+1
  sta t0+1
  rts
tdelta:
  lda #1
  phx
  ldx #1
  jsr api
  plx
  sec
  lda API_PARAMETERS
  sbc t0
  sta RES,x
  lda API_PARAMETERS+1
  sbc t0+1
  sta RES+1,x
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
