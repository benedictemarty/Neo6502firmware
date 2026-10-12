; clipimg.asm — dessin hors écran (étude T-132, défauts relevés : à confirmer par ce test).
; Mode 0. Une tuile 16 x 16 unie de couleur 1 est installée dans gfxObjectMemory par 12,2 (en-tête : 1 tuile).
; A : 5,7 image 0 en (312,100), échelle 1 : elle dépasse à droite de 8 pixels -> (0,101), début de la ligne
;     suivante, doit rester noir (00) ; (319,101) doit être 01.
; B : 5,7 image 0 en (100,-8) : les 8 premières rangées sont au-dessus de l'écran -> somme des 2 560 derniers
;     octets de gfxObjectMemory ($90:7600-$7FFF, relus par 12,2) inchangée (0000), (100,0) doit être 01.
; C : 5,2 ligne de (-10,50) à (100,50) : partiellement hors écran -> (0,50) et (99,50) doivent être 01 (tracée,
;     coupée au bord ; comme toujours, le point d'arrivée (100,50) n'est pas tracé : 00).
; D : 5,2 ligne de (300,60) à (400,60) : sort à droite -> (300,60) et (319,60), le bord, doivent être 01.
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
GFX   = $4000               ; image de gfxObjectMemory à installer : 256 octets d'en-tête + 128 de tuile
BUF   = $5000               ; relecture de la fin de gfxObjectMemory
sum   = $E0
cnt   = $E2
sp    = $E4

start:
  stz logLen
  lda #$20
  sta logLen+1
  stz API_PARAMETERS        ; mode 0
  lda #9
  ldx #5
  jsr api
  lda #12
  jsr wchar

  ldx #0                    ; en-tête : 01 01 00 00 puis zéros ; tuile : $11 x 128
hd:
  stz GFX,x
  stz BUF,x
  inx
  bne hd
  lda #1
  sta GFX
  sta GFX+1
  ldx #127
tl:
  lda #$11
  sta GFX+256,x
  dex
  bpl tl
  lda #$00                  ; 12,2 $00:4000 -> $90:0000, 384 octets
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #$40
  sta API_PARAMETERS+2
  lda #$90
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #<384
  sta API_PARAMETERS+6
  lda #>384
  sta API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api

  ldx #<sa                  ; A
  ldy #>sa
  jsr print
  lda #<312
  ldx #>312
  ldy #100
  jsr image
  lda #0
  ldx #0
  ldy #101
  jsr pixel
  lda #<319
  ldx #>319
  ldy #101
  jsr pixel
  jsr cr

  ldx #<sb                  ; B
  ldy #>sb
  jsr print
  jsr gsum                  ; somme avant
  lda sum
  pha
  lda sum+1
  pha
  lda #100
  ldx #0
  ldy #$F8                  ; y = -8
  jsr imagey
  jsr gsum                  ; somme après - avant
  pla
  sta sp+1
  pla
  sta sp
  sec
  lda sum
  sbc sp
  pha
  lda sum+1
  sbc sp+1
  jsr hex
  pla
  jsr hex
  lda #' '
  jsr wchar
  lda #100
  ldx #0
  ldy #0
  jsr pixel
  jsr cr

  ldx #<sc                  ; C
  ldy #>sc
  jsr print
  lda #1                    ; 5,64 couleur 1
  sta API_PARAMETERS
  lda #64
  ldx #5
  jsr api
  lda #<-10                 ; 5,2 (-10,50)-(100,50)
  sta API_PARAMETERS
  lda #>-10
  sta API_PARAMETERS+1
  lda #50
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #100
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #50
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #5
  jsr api
  lda #0
  ldx #0
  ldy #50
  jsr pixel
  lda #99
  ldx #0
  ldy #50
  jsr pixel
  lda #100
  ldx #0
  ldy #50
  jsr pixel
  jsr cr

  ldx #<sd                  ; D : 5,2 (300,60)-(400,60)
  ldy #>sd
  jsr print
  lda #<300
  sta API_PARAMETERS
  lda #>300
  sta API_PARAMETERS+1
  lda #60
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<400
  sta API_PARAMETERS+4
  lda #>400
  sta API_PARAMETERS+5
  lda #60
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #5
  jsr api
  lda #<300
  ldx #>300
  ldy #60
  jsr pixel
  lda #<319
  ldx #>319
  ldy #60
  jsr pixel
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- 5,7 image 0 en (A:X, Y) (y positif) / imagey : y négatif (Y = octet bas, haut = $FF)
image:
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  bra im1
imagey:
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  lda #$FF
  sta API_PARAMETERS+3
im1:
  stz API_PARAMETERS+4
  lda #7
  ldx #5
  jmp api

; --- 5,33 pixel (A:X, Y) : affiche "vv "
pixel:
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #33
  ldx #5
  jsr api
  lda API_PARAMETERS
  jsr hex
  lda #' '
  jmp wchar

; --- somme 16 bits de $90:7600-$7FFF (2 560 octets, relus en 10 blocs de 256 par 12,2)
gsum:
  stz sum
  stz sum+1
  lda #$76
  sta cnt
gs1:
  lda #$90
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda cnt
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #>BUF
  sta API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #1
  sta API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  ldx #0
gs2:
  lda BUF,x
  clc
  adc sum
  sta sum
  bcc gs3
  inc sum+1
gs3:
  inx
  bne gs2
  inc cnt
  lda cnt
  cmp #$80
  bne gs1
  rts

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

sa:      .text "A ", 0
sb:      .text "B ", 0
sc:      .text "C ", 0
sd:      .text "D ", 0
sfin:    .text "END", 0
