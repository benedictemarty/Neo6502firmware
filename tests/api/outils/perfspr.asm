; perfspr.asm — T-157 : coût de 6 sprites 24 x 32 (6,7, 4 bits par pixel) déplacés pendant 100 trames, en centièmes (1,1).
; Budget à 50 Hz : 200 cs pour 100 trames. 6 cas : écartés (dx 40, dy 36) puis chevauchés (dx 8, dy 4),
; pour 6,6 = 0 (XOR), 1 (opaque), 2 (octet entier, sauvegarde en $90:2000, 6 000 o).
; Résultats : à l'écran et en $0A00 (6 mots, centièmes, petit-boutiste). Outil carte.
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
dy  = $F9
ri  = $FA
md  = $FB
GFX = $3300
RES = $0A00

start:
  stz ri
  lda #1                    ; 6,1 : sprites remis à zéro
  ldx #6
  jsr api
  lda #12
  jsr wchar
  ldx #0                    ; image 24 x 32 4 bpp = 384 o : motif $12 / $34
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
  lda #$90                  ; 6,8
  sta API_PARAMETERS
  lda #8
  ldx #6
  jsr api
  stz md
cas:
  lda #1                    ; 6,1 puis mode
  ldx #6
  jsr api
  lda #$90
  sta API_PARAMETERS
  lda #8
  ldx #6
  jsr api
  lda md
  sta API_PARAMETERS
  lda #6
  ldx #6
  jsr api
  lda md
  cmp #2
  bne nosave
  stz API_PARAMETERS        ; 6,9 : zone $2000, $1770 octets
  lda #$20
  sta API_PARAMETERS+1
  lda #$70
  sta API_PARAMETERS+2
  lda #$17
  sta API_PARAMETERS+3
  lda #9
  ldx #6
  jsr api
nosave:
  lda #40                   ; dx, dy : écartés d'abord
  sta dx
  lda #36
  sta dy
  jsr une
  lda #8                    ; puis chevauchés
  sta dx
  lda #4
  sta dy
  jsr une
  inc md
  lda md
  cmp #3
  bne cas
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

; une : 6 sprites (6,7), 100 trames de 6 x 6,2, temps -> RES[ri]
une:
  stz si
u0:
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
  lda API_ERROR
  beq u1
  ora #$80                  ; erreur 6,7 visible
  sta RES+$20
u1:
  inc si
  lda si
  cmp #6
  bne u0
  jsr tnow
  stz f
tr:
  stz si
  lda #16                   ; x de base = 16 + si*dx + (f & 31)
  sta ptr
sp:
  lda f
  and #31
  clc
  adc ptr
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda si                    ; y = 10 + si*dy
  tax
  lda #10
yl:
  cpx #0
  beq yd
  clc
  adc dy
  dex
  bra yl
yd:
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda si
  sta API_PARAMETERS
  lda #$80
  sta API_PARAMETERS+5
  stz API_PARAMETERS+6
  sta API_PARAMETERS+7
  lda #2
  ldx #6
  jsr api
  lda ptr
  clc
  adc dx
  sta ptr
  inc si
  lda si
  cmp #6
  bne sp
  inc f
  lda f
  cmp #100
  bne tr
  ldx ri
  jsr tdelta
  inc ri
  inc ri
  lda #3                    ; 6,3 : masque les 6 sprites
  sta si
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
