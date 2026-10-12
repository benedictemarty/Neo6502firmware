; perfdraw.asm — T-132 (P2/P3) : durée du dessin des modes compacts, en centièmes de seconde (1,1). Outil carte.
; Pour le mode 2 puis le mode 1 : 20 rectangles pleins de tout l'écran (5,3, couleur alternée), puis 1 000 caractères
; écrits par la console (2,6, 20 lignes de 50). Résultats : à l'écran et en $0A00 (4 octets : rect m2, texte m2,
; rect m1, texte m1, en centièmes, 8 bits chacun saturés à 255). Comparer deux firmwares dans les mêmes conditions.
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
RES   = $0A00
t0    = $F4
n     = $F6
k     = $F7
ri    = $F8

start:
  stz ri
  lda #2
  jsr bench
  lda #1
  jsr bench
  stz API_PARAMETERS        ; mode 0 et affichage
  lda #9
  ldx #5
  jsr api
  ldx #<stitre
  ldy #>stitre
  jsr print
  ldx #0
aff:
  lda RES,x
  phx
  jsr hex
  lda #' '
  jsr wchar
  plx
  inx
  cpx #4
  bne aff
  jsr cr
  rts

; --- mode A : rectangles puis texte
bench:
  sta API_PARAMETERS
  sta k
  lda #9
  ldx #5
  jsr api
  jsr tnow
  lda #20
  sta n
rl:
  lda #0                    ; 5,1 : et 0, ou exclusif = couleur, plein
  sta API_PARAMETERS
  lda n
  and #7
  ora #1
  sta API_PARAMETERS+1
  lda #1
  sta API_PARAMETERS+2
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #1
  ldx #5
  jsr api
  stz API_PARAMETERS        ; 5,3 (0,0)-(719,349) : coupé à l'écran du mode
  stz API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<719
  sta API_PARAMETERS+4
  lda #>719
  sta API_PARAMETERS+5
  lda #<349
  sta API_PARAMETERS+6
  lda #>349
  sta API_PARAMETERS+7
  lda #3
  ldx #5
  jsr api
  dec n
  bne rl
  jsr tdelta
  lda #12
  jsr wchar
  jsr tnow
  lda #20
  sta n
tl:
  ldx #50
tc:
  phx
  txa
  and #31
  ora #$40
  jsr wchar
  plx
  dex
  bne tc
  lda #13
  jsr wchar
  dec n
  bne tl
  jmp tdelta

; --- timer 1,1 (centièmes) -> t0 ; tdelta : RES[ri++] = min(255, maintenant - t0)
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
  ldx #1
  jsr api
  sec
  lda API_PARAMETERS
  sbc t0
  tay
  lda API_PARAMETERS+1
  sbc t0+1
  beq td1
  ldy #255
td1:
  tya
  ldx ri
  sta RES,x
  inc ri
  rts

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

stitre:  .text 12, "PERFDRAW (cs) : rect m2, texte m2, rect m1, texte m1", 13, 0
