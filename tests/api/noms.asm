; noms.asm — noms de fichiers en Latin-1 (Trinity T-130, demande D-03 de NeoWin).
; 3,14 md « Été », 3,15 cd « Été », 3,23 cwd, 3,3 écrit « Noël.txt », 3,15 cd « / », 3,12 renomme
; « Été/Noël.txt » en « Été/À lire.txt », puis 3,17/3,18 liste « Été ». Les restes d'un essai précédent
; (carte) sont d'abord effacés par 3,13 ; ceux de cet essai restent, pour la comparaison ci-dessous.
; Chaque ligne : l'erreur (00 = bien) et, pour 3,23 et 3,18, le nom rendu, en Latin-1.
; Dans neo, run_neo.sh compare aussi les noms créés sur le PC (noms.fichiers) : ils doivent être en UTF-8.
; « cd été » (casse ignorée) n'est pas essayé ici : le PC hôte distingue la casse ; voir make test-noms.
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
NBUF  = $B00

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar

  lda #<nneuf               ; restes d'un essai précédent (carte) : effacés sans rien afficher
  sta API_PARAMETERS
  lda #>nneuf
  sta API_PARAMETERS+1
  lda #13
  ldx #3
  jsr api
  lda #<nete
  sta API_PARAMETERS
  lda #>nete
  sta API_PARAMETERS+1
  lda #13
  ldx #3
  jsr api

  ldx #<smd                 ; MD Été
  ldy #>smd
  jsr print
  ldx #<nete
  ldy #>nete
  lda #14
  jsr apiname
  jsr cr

  ldx #<scd                 ; CD Été
  ldy #>scd
  jsr print
  ldx #<nete
  ldy #>nete
  lda #15
  jsr apiname
  jsr cr

  ldx #<scwd                ; CWD -> /Été
  ldy #>scwd
  jsr print
  lda #<NBUF
  sta API_PARAMETERS
  lda #>NBUF
  sta API_PARAMETERS+1
  lda #64
  sta API_PARAMETERS+2
  lda #23
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  jsr shownbuf
  jsr cr

  ldx #<ssave               ; SAVE Noël.txt (4 octets de sfin)
  ldy #>ssave
  jsr print
  lda #<nnoel
  sta API_PARAMETERS
  lda #>nnoel
  sta API_PARAMETERS+1
  lda #<sfin
  sta API_PARAMETERS+2
  lda #>sfin
  sta API_PARAMETERS+3
  lda #4
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #3
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<scd                 ; CD /
  ldy #>scd
  jsr print
  ldx #<nracine
  ldy #>nracine
  lda #15
  jsr apiname
  jsr cr

  ldx #<sren                ; REN Été/Noël.txt Été/À lire.txt
  ldy #>sren
  jsr print
  lda #<nvieux
  sta API_PARAMETERS
  lda #>nvieux
  sta API_PARAMETERS+1
  lda #<nneuf
  sta API_PARAMETERS+2
  lda #>nneuf
  sta API_PARAMETERS+3
  lda #12
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<sdir                ; DIR Été -> À lire.txt
  ldy #>sdir
  jsr print
  ldx #<nete
  ldy #>nete
  lda #17
  jsr apiname
  lda #<NBUF
  sta API_PARAMETERS
  lda #>NBUF
  sta API_PARAMETERS+1
  lda #64
  sta NBUF
  lda #18
  ldx #3
  jsr api
  lda #' '
  jsr wchar
  lda API_ERROR
  jsr hex
  jsr shownbuf
  jsr cr
  lda #19                   ; 3,19 Close Directory
  ldx #3
  jsr api

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- appel 3,A avec Parameters:0,1 = X,Y (nom) ; affiche l'erreur
apiname:
  stx API_PARAMETERS
  sty API_PARAMETERS+1
  ldx #3
  jsr api
  lda API_ERROR
  jmp hex

; --- " " puis la chaîne à longueur en NBUF
shownbuf:
  lda #' '
  jsr wchar
  ldx #0
sn1:
  cpx NBUF
  beq sn2
  lda NBUF+1,x
  phx
  jsr wchar
  plx
  inx
  bra sn1
sn2:
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

smd:     .text "MD ", 0
scd:     .text "CD ", 0
scwd:    .text "CWD ", 0
ssave:   .text "SAVE ", 0
sren:    .text "REN ", 0
sdir:    .text "DIR ", 0
sfin:    .text "END", 0
; noms en Latin-1, octets explicites (É = $C9, é = $E9, ë = $EB, À = $C0)
nete:    .byte 3, $C9, 't', $E9
nnoel:   .byte 8, 'N', 'o', $EB, 'l', '.', 't', 'x', 't'
nracine: .byte 1, '/'
nvieux:  .byte 12, $C9, 't', $E9, '/', 'N', 'o', $EB, 'l', '.', 't', 'x', 't'
nneuf:   .byte 14, $C9, 't', $E9, '/', $C0, ' ', 'l', 'i', 'r', 'e', '.', 't', 'x', 't'
