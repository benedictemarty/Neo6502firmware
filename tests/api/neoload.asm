; neoload.asm — 3,2 Load File d'un .neo à trois blocs (Trinity T-132 P1 : chargement par blocs au lieu
; d'un f_read par octet). neoload.bin (tests/tools/mkneoload.py) : $5000 3 000 octets, $FFFF (objets)
; 1 000 octets, $6000 300 octets avec un commentaire de 40 caractères. Affiche l'erreur de 3,2 puis les
; sommes 16 bits de chaque zone (D604, F024 via 12,2 depuis $90, 8F12) et de $5BB8-$5BFF juste après le
; bloc 1 (0000 : rien écrit au-delà). Attendu identique à celui de la 0.16.86 (chargement octet par octet).
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
sptr  = $E0
scnt  = $E2
ssum  = $E4

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar

  ldx #<sload               ; LOAD : 3,2 neoload.bin -> 00
  ldy #>sload
  jsr print
  lda #<fname
  sta API_PARAMETERS
  lda #>fname
  sta API_PARAMETERS+1
  lda #$00
  sta API_PARAMETERS+2
  lda #$80
  sta API_PARAMETERS+3
  lda #2
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  lda #$00                  ; B1 : $5000, 3000
  ldx #$50
  ldy #<3000
  phy
  ldy #>3000
  jsr sumshow
  lda #$B8                  ; APRES : $5BB8, 72
  ldx #$5B
  ldy #72
  phy
  ldy #0
  jsr sumshow
  lda #$00                  ; B3 : $6000, 300
  ldx #$60
  ldy #<300
  phy
  ldy #>300
  jsr sumshow

  lda #$90                  ; B2 : 12,2 $90:0000 -> $00:7000, 1000 octets
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #$70
  sta API_PARAMETERS+5
  lda #<1000
  sta API_PARAMETERS+6
  lda #>1000
  sta API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda #$00
  ldx #$70
  ldy #<1000
  phy
  ldy #>1000
  jsr sumshow

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- somme 16 bits de A:X (adresse), longueur (pile : bas, Y : haut) ; affiche "hhhh"
sumshow:
  sta sptr
  stx sptr+1
  sty scnt+1
  pla                       ; adresse de retour
  sta ssum
  pla
  sta ssum+1
  pla                       ; longueur, octet bas
  sta scnt
  lda ssum+1
  pha
  lda ssum
  pha
  stz ssum
  stz ssum+1
sloop:
  lda scnt
  ora scnt+1
  beq sdone
  lda (sptr)
  clc
  adc ssum
  sta ssum
  bcc s1
  inc ssum+1
s1:
  inc sptr
  bne s2
  inc sptr+1
s2:
  lda scnt
  bne s3
  dec scnt+1
s3:
  dec scnt
  bra sloop
sdone:
  lda ssum+1
  jsr hex
  lda ssum
  jsr hex
  jmp cr

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

sload:   .text "LOAD ", 0
sfin:    .text "END", 0
fname:   .ptext "neoload.bin"
