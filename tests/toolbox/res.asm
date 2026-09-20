; res.asm — Toolbox groupe 38 Resource Manager (F-46 du fork firmware) : test.res (mkres.py : FONT 1 = dejavu9.nf1,
; TEXT 2 = "Hello") ouvert sur le canal 3.
; Sortie attendue (console) :
;   OPEN 00 / COUNT 02 / FIND 01 02 00 / INFO TEXT 0002 / SIZE 00000005 / LOAD 00 0005 Hello / LIMIT 00 0003
;   FONT 00 0968 0C 0016 / BAD 01 / CLOSE 00 01 / MISSING 02 / END
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=res.neo6502 res.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $E00                ; données chargées

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  lda #1                    ; 32,1
  ldx #32
  jsr api

  ldx #<sopen               ; OPEN : 38,1 canal 3 "test.res"
  ldy #>sopen
  jsr print
  ldx #<fname
  ldy #>fname
  jsr open
  jsr cr

  ldx #<scount              ; COUNT : 38,3 -> 2
  ldy #>scount
  jsr print
  lda #3
  ldx #38
  jsr api
  lda API_PARAMETERS
  jsr hex
  jsr cr

  ldx #<sfind               ; FIND : FONT 1 -> 1, TEXT 2 -> 2, TEXT 9 -> 0
  ldy #>sfind
  jsr print
  ldx #<tfont
  ldy #>tfont
  lda #1
  jsr find
  ldx #<ttext
  ldy #>ttext
  lda #2
  jsr find
  ldx #<ttext
  ldy #>ttext
  lda #9
  jsr find
  jsr cr

  ldx #<sinfo               ; INFO : ressource 2 -> type TEXT, id 2
  ldy #>sinfo
  jsr print
  lda #2
  sta API_PARAMETERS
  lda #5
  ldx #38
  jsr api
  ldx #0
il:
  lda API_PARAMETERS+1,x
  phx
  jsr wchar
  plx
  inx
  cpx #4
  bne il
  lda #' '
  jsr wchar
  lda API_PARAMETERS+6
  jsr hex
  lda API_PARAMETERS+5
  jsr hex
  jsr cr

  ldx #<ssize               ; SIZE : ressource 2 -> 5 (32 bits)
  ldy #>ssize
  jsr print
  lda #2
  sta API_PARAMETERS
  lda #6
  ldx #38
  jsr api
  ldx #3
sl:
  lda API_PARAMETERS+1,x
  phx
  jsr hex
  plx
  dex
  bpl sl
  jsr cr

  ldx #<sload               ; LOAD : ressource 2 en RAM $E00, 16 max -> 5 octets "Hello"
  ldy #>sload
  jsr print
  lda #2
  ldx #16
  jsr load
  jsr showload
  lda #' '
  jsr wchar
  ldx #0
hl:
  lda buf,x
  phx
  jsr wchar
  plx
  inx
  cpx #5
  bne hl
  jsr cr

  ldx #<slimit              ; LIMIT : 3 octets max -> 3
  ldy #>slimit
  jsr print
  lda #2
  ldx #3
  jsr load
  jsr showload
  jsr cr

  ldx #<sfont               ; FONT : 38,8 ressource 1 en RAM graphique $90:0000 -> 0968 octets, hauteur 0C, "Neo" = 0016
  ldy #>sfont
  jsr print
  lda #1
  sta API_PARAMETERS
  lda #$90
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #$FF
  sta API_PARAMETERS+4
  lda #$0F
  sta API_PARAMETERS+5
  lda #8
  ldx #38
  jsr api
  jsr showload
  lda #18                   ; 32,18 Get Font Info
  ldx #32
  jsr api
  lda API_PARAMETERS        ; avant wchar, qui écrase P0
  sta buf+32
  lda #' '
  jsr wchar
  lda buf+32
  jsr hex
  lda #<tneo                ; 32,17 Text Width
  sta API_PARAMETERS
  lda #>tneo
  sta API_PARAMETERS+1
  lda #17
  ldx #32
  jsr api
  lda #' '
  jsr wchar
  lda API_PARAMETERS+3
  jsr hex
  lda API_PARAMETERS+2
  jsr hex
  jsr cr

  ldx #<sbad                ; BAD : ressource 2 (texte) en Use Font -> 1
  ldy #>sbad
  jsr print
  lda #2
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #<buf
  sta API_PARAMETERS+2
  lda #>buf
  sta API_PARAMETERS+3
  lda #16
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #8
  ldx #38
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<sclose              ; CLOSE : 38,2 -> 0, puis 1 (rien d'ouvert)
  ldy #>sclose
  jsr print
  lda #2
  ldx #38
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda #2
  ldx #38
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<smissing            ; MISSING : fichier absent -> 2
  ldy #>smissing
  jsr print
  ldx #<fnone
  ldy #>fnone
  jsr open
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  jmp halt

; --- 38,1 canal 3, nom en X/Y -> "err"
open:
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  lda #3
  sta API_PARAMETERS
  lda #1
  ldx #38
  jsr api
  lda API_ERROR
  jmp hex

; --- 38,4 type en X/Y (4 octets), id A -> " idx"
find:
  stx ptr
  sty ptr+1
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5
  ldy #0
fl:
  lda (ptr),y
  sta API_PARAMETERS,y
  iny
  cpy #4
  bne fl
  lda #4
  ldx #38
  jsr api
  lda API_PARAMETERS+6
  sta buf+32
  lda #' '
  jsr wchar
  lda buf+32
  jmp hex

; --- 38,7 ressource A en RAM $E00, X octets max
load:
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #<buf
  sta API_PARAMETERS+2
  lda #>buf
  sta API_PARAMETERS+3
  stx API_PARAMETERS+4
  stz API_PARAMETERS+5
  lda #7
  ldx #38
  jmp api

; --- "err nnnn" après un chargement
showload:
  lda API_ERROR
  sta buf+32
  lda API_PARAMETERS+7
  sta buf+33
  lda API_PARAMETERS+6
  sta buf+34
  lda buf+32
  jsr hex
  lda #' '
  jsr wchar
  lda buf+33
  jsr hex
  lda buf+34
  jmp hex

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

sopen:    .text "OPEN ", 0
scount:   .text "COUNT ", 0
sfind:    .text "FIND", 0
sinfo:    .text "INFO ", 0
ssize:    .text "SIZE ", 0
sload:    .text "LOAD ", 0
slimit:   .text "LIMIT ", 0
sfont:    .text "FONT ", 0
sbad:     .text "BAD ", 0
sclose:   .text "CLOSE ", 0
smissing: .text "MISSING ", 0
sfin:     .text "END", 13, 0
fname:    .ptext "test.res"
fnone:    .ptext "none.res"
tfont:    .text "FONT"
ttext:    .text "TEXT"
tneo:     .ptext "Neo"
