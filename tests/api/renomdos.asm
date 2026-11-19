; renomdos.asm — renommage d'un dossier à l'autre (Trinity T-147, question D-07 de NeoWin).
; 3,13 efface les restes, 3,14 md RA et RB, 3,3 écrit RA/F.TXT, 3,12 le renomme en RB/F.TXT (déplacement,
; attendu 00), 3,17/3,18 liste RA (vide) puis RB (F.TXT), 3,12 vers un nom existant (attendu 01).
; Chaque ligne : l'erreur (00 = bien) et, pour 3,18, le nom rendu.
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
  ldx #<nraf                ; restes d'un essai précédent, effacés sans rien afficher
  ldy #>nraf
  jsr apiquiet13
  ldx #<nrag
  ldy #>nrag
  jsr apiquiet13
  ldx #<nrbf
  ldy #>nrbf
  jsr apiquiet13
  ldx #<nra
  ldy #>nra
  jsr apiquiet13
  ldx #<nrb
  ldy #>nrb
  jsr apiquiet13

  ldx #<smd                 ; MD RA, MD RB
  ldy #>smd
  jsr print
  ldx #<nra
  ldy #>nra
  lda #14
  jsr apiname
  ldx #<nrb
  ldy #>nrb
  lda #14
  jsr apiname
  jsr cr

  ldx #<nraf                ; SAVE RA/F.TXT puis RA/G.TXT
  ldy #>nraf
  jsr save
  ldx #<nrag
  ldy #>nrag
  jsr save

  ldx #<sren                ; REN RA/F.TXT RB/F.TXT (autre dossier)
  ldy #>sren
  jsr print
  ldx #<nraf
  ldy #>nraf
  stx API_PARAMETERS
  sty API_PARAMETERS+1
  ldx #<nrbf
  ldy #>nrbf
  jsr ren
  jsr cr

  ldx #<sdirra              ; DIR RA : reste G.TXT
  ldy #>sdirra
  jsr print
  ldx #<nra
  ldy #>nra
  jsr dir
  ldx #<sdirrb              ; DIR RB : F.TXT
  ldy #>sdirrb
  jsr print
  ldx #<nrb
  ldy #>nrb
  jsr dir

  ldx #<sren2               ; REN RA/G.TXT RB/F.TXT : le nom existe, erreur attendue
  ldy #>sren2
  jsr print
  ldx #<nrag
  ldy #>nrag
  stx API_PARAMETERS
  sty API_PARAMETERS+1
  ldx #<nrbf
  ldy #>nrbf
  jsr ren
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

apiquiet13:
  stx API_PARAMETERS
  sty API_PARAMETERS+1
  lda #13
  ldx #3
  jmp api

; --- X,Y = nom : SAVE 4 octets de sfin
save:
  stx API_PARAMETERS
  sty API_PARAMETERS+1
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
  rts

; --- Parameters:0,1 déjà posés (ancien nom), X,Y = nouveau nom : 3,12, affiche l'erreur
ren:
  stx API_PARAMETERS+2
  sty API_PARAMETERS+3
  lda #12
  ldx #3
  jsr api
  lda API_ERROR
  jmp hex

; --- X,Y = dossier : 3,17, un 3,18, 3,19
dir:
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
  lda #19
  ldx #3
  jmp api

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
sren:    .text "REN ", 0
sren2:   .text "REN2 ", 0
sdirra:  .text "DIR RA ", 0
sdirrb:  .text "DIR RB ", 0
sfin:    .text "END", 0
nra:     .byte 2, 'R', 'A'
nrb:     .byte 2, 'R', 'B'
nraf:    .byte 8, 'R', 'A', '/', 'F', '.', 'T', 'X', 'T'
nrag:    .byte 8, 'R', 'A', '/', 'G', '.', 'T', 'X', 'T'
nrbf:    .byte 8, 'R', 'B', '/', 'F', '.', 'T', 'X', 'T'
