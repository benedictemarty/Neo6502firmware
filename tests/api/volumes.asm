; volumes.asm — 3,24 Volume Info / 3,25 Select Volume / 3,26 Get Current Volume (Trinity T-18, F-102 du fork).
; neo : volume 0 = storage/, volume 1 = storage1/ (le lanceur le crée avec un fichier vol1.txt "un").
; Sortie attendue (console) :
;   CUR 00 / INFO0 00 01 HOST0 / INFO2 13 / INFO1 00 01 HOST1 / SEL1 00 / CUR 01 / OPEN 00 / READ 00 02 un
;   SEL0 00 / CUR 00 / OPEN1 00 / END   (13 = FIOERROR_INVALID_DRIVE)
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=volumes.neo6502 volumes.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $B00
rbuf  = $B40

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  jsr curvol                ; CUR 00
  ldx #0                    ; INFO0 00 01 HOST0
  jsr info
  ldx #2                    ; INFO2 13 (absent)
  jsr info
  ldx #1                    ; INFO1 00 01 HOST1
  jsr info
  ldx #<ssel1               ; SEL1 : 3,25 volume 1 -> 00
  ldy #>ssel1
  jsr print
  lda #1
  sta API_PARAMETERS
  lda #25
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr
  jsr curvol                ; CUR 01
  ldx #<sopen               ; OPEN : 3,4 canal 0 "vol1.txt" (sans préfixe : volume courant = 1) -> 00
  ldy #>sopen
  jsr print
  ldx #<fvol1
  ldy #>fvol1
  jsr open0
  lda API_ERROR
  jsr hex
  jsr cr
  ldx #<sread               ; READ : 3,8 canal 0, 2 octets -> 00 02 "un"
  ldy #>sread
  jsr print
  stz API_PARAMETERS
  lda #<rbuf
  sta API_PARAMETERS+1
  lda #>rbuf
  sta API_PARAMETERS+2
  lda #2
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #8
  ldx #3
  jsr api
  lda API_PARAMETERS+3
  sta buf
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda rbuf
  jsr wchar
  lda rbuf+1
  jsr wchar
  jsr cr
  stz API_PARAMETERS        ; 3,5 close canal 0
  lda #5
  ldx #3
  jsr api
  ldx #<ssel0               ; SEL0 : 3,25 volume 0 -> 00
  ldy #>ssel0
  jsr print
  stz API_PARAMETERS
  lda #25
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr
  jsr curvol                ; CUR 00
  ldx #<sopen1              ; OPEN1 : 3,4 "1:vol1.txt" depuis le volume 0 -> 00
  ldy #>sopen1
  jsr print
  ldx #<fpref
  ldy #>fpref
  jsr open0
  lda API_ERROR
  jsr hex
  jsr cr
  stz API_PARAMETERS
  lda #5
  ldx #3
  jsr api
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts                       ; retour à l'appelant (sys du BASIC, ou NeoDOS pour un .NEO) ; reset sinon

; --- 3,4 open canal 0, nom (X,Y), lecture seule
open0:
  stz API_PARAMETERS
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3      ; mode 0 = lecture
  lda #4
  ldx #3
  jmp api

; --- "CUR nn" : 3,26
curvol:
  ldx #<scur
  ldy #>scur
  jsr print
  lda #26
  ldx #3
  jsr api
  lda API_PARAMETERS
  jsr hex
  jmp cr

; --- "INFOn err [attr nom]" : 3,24 volume X
info:
  phx
  ldx #<sinfo
  ldy #>sinfo
  jsr print
  plx
  phx
  txa
  clc
  adc #'0'
  jsr wchar
  lda #' '
  jsr wchar
  plx
  stx API_PARAMETERS
  lda #63                   ; capacité du tampon préfixé (DSPSetStdString tronque à cette longueur)
  sta buf
  lda #<buf
  sta API_PARAMETERS+1
  lda #>buf
  sta API_PARAMETERS+2
  lda #24
  ldx #3
  jsr api
  lda API_PARAMETERS+3
  sta rbuf
  lda API_ERROR
  pha
  jsr hex
  pla
  bne infodone
  lda #' '
  jsr wchar
  lda rbuf
  jsr hex
  lda #' '
  jsr wchar
  ldx #0
inf1:
  cpx buf
  beq infodone
  inx
  lda buf,x
  phx
  jsr wchar
  plx
  bra inf1
infodone:
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

scur:    .text "CUR ", 0
sinfo:   .text "INFO", 0
ssel1:   .text "SEL1 ", 0
ssel0:   .text "SEL0 ", 0
sopen:   .text "OPEN ", 0
sread:   .text "READ ", 0
sopen1:  .text "OPEN1 ", 0
sfin:    .text "END", 0
fvol1:   .ptext "vol1.txt"
fpref:   .ptext "1:vol1.txt"
