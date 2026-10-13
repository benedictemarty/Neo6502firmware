; wmtitle.asm — T-133 (D-06 de NeoWin, option b) : 34,9 Set Window Title sur une fenêtre recouverte ne dessine que ses
; parties visibles. Alpha (10,20)-(150,100) derrière, Beta (60,10)-(200,90) devant : la barre de titre d'Alpha passe sous
; Beta. Somme 16 bits de la zone de Beta (lignes 10-89, colonnes 60-199) et de la partie visible de la barre d'Alpha
; (lignes 21-30, colonnes 11-59), relues en $80 par 12,2, avant et après 34,9 « WWWWWWWWWWWWWWWWWWWW » sur Alpha.
; Attendu : BETA 0000 (aucun octet de Beta n'a changé), ALPHA non nul (le nouveau titre est visible dans la partie libre).
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
BUF   = $5000
sum   = $E0
y     = $E2                 ; ligne courante (16 bits)
yend  = $E4
xl    = $E6                 ; colonne de départ
wid   = $E8                 ; largeur
idA   = $EA
b0    = $EC                 ; sommes avant (Beta, Alpha)
a0    = $EE

start:
  stz logLen
  lda #$20
  sta logLen+1
  stz API_PARAMETERS        ; mode 0
  lda #9
  ldx #5
  jsr api
  lda #1                    ; 32,1 InitGraf
  ldx #32
  jsr api
  lda #<ra                  ; 34,1 Alpha
  ldx #>ra
  ldy #<ta
  jsr neww
  lda API_PARAMETERS+5
  sta idA
  lda #<rb                  ; 34,1 Beta (devant)
  ldx #>rb
  ldy #<tb
  jsr neww

  jsr sbeta
  lda sum
  sta b0
  lda sum+1
  sta b0+1
  jsr salpha
  lda sum
  sta a0
  lda sum+1
  sta a0+1

  lda idA                   ; 34,9 nouveau titre d'Alpha
  sta API_PARAMETERS
  lda #<tw
  sta API_PARAMETERS+1
  lda #>tw
  sta API_PARAMETERS+2
  lda #9
  ldx #34
  jsr api
  lda API_ERROR
  sta $3000
  jsr sbeta                 ; sommes après, AVANT tout affichage (2,6 écrirait sur l'écran)
  sec
  lda sum
  sbc b0
  sta $3002
  lda sum+1
  sbc b0+1
  sta $3003
  jsr salpha
  sec
  lda sum
  sbc a0
  sta $3001
  lda sum+1
  sbc a0+1
  ora $3001
  beq z0
  lda #1
z0:
  sta $3004

  lda #12
  jsr wchar
  ldx #<st
  ldy #>st
  jsr print
  lda $3000
  jsr hex
  jsr cr
  ldx #<sb                  ; BETA : différence avant/après
  ldy #>sb
  jsr print
  lda $3003
  jsr hex
  lda $3002
  jsr hex
  jsr cr
  ldx #<sa                  ; ALPHA : 01 si la partie visible a changé
  ldy #>sa
  jsr print
  lda $3004
  jsr hex
  jsr cr
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- 34,1 : rect en A:X (adresse), titre en Y (octet bas, page de ta), drapeaux 1 (barre de titre)
neww:
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  lda #>ta
  sta API_PARAMETERS+3
  lda #1
  sta API_PARAMETERS+4
  lda #1
  ldx #34
  jmp api

; --- sommes de zones (mode 0 : 320 octets par ligne)
sbeta:
  lda #10
  sta y
  stz y+1
  lda #90
  sta yend
  lda #60
  sta xl
  lda #140
  sta wid
  bra szone
salpha:
  lda #21
  sta y
  stz y+1
  lda #31
  sta yend
  lda #11
  sta xl
  lda #49
  sta wid
szone:
  stz sum
  stz sum+1
sz1:
  lda y                     ; adresse = y * 320 + xl  (y < 256)
  sta ptr
  lda #0
  sta ptr+1
  asl ptr                   ; y * 64
  rol ptr+1
  asl ptr
  rol ptr+1
  asl ptr
  rol ptr+1
  asl ptr
  rol ptr+1
  asl ptr
  rol ptr+1
  asl ptr
  rol ptr+1
  lda ptr                   ; + y * 256 -> y * 320
  sta API_PARAMETERS+1
  lda ptr+1
  clc
  adc y
  sta API_PARAMETERS+2
  lda API_PARAMETERS+1
  clc
  adc xl
  sta API_PARAMETERS+1
  bcc sz2
  inc API_PARAMETERS+2
sz2:
  lda #$80
  sta API_PARAMETERS
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #>BUF
  sta API_PARAMETERS+5
  lda wid
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  ldx #0
sz3:
  lda BUF,x
  clc
  adc sum
  sta sum
  bcc sz4
  inc sum+1
sz4:
  inx
  cpx wid
  bne sz3
  inc y
  lda y
  cmp yend
  bne sz1
  rts

api:
  sta API_FUNCTION
  stx API_COMMAND
wait:
  lda API_COMMAND
  bne wait
  rts

wchar:
  pha                       ; journal en RAM $C00.. (longueur 16 bits en $BFE) : tests/toolbox/run_neo.sh
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

cr:
  lda #13
  jmp wchar

st:      .text "SETTITLE ", 0
sb:      .text "BETA ", 0
sa:      .text "ALPHA ", 0
sfin:    .text "END", 0
ra:      .word 10, 20, 150, 100
rb:      .word 60, 10, 200, 90
ta:      .ptext "Alpha"
tb:      .ptext "Beta"
tw:      .ptext "WWWWWWWWWWWWWWWWWWWW"
