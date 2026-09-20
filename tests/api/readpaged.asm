; readpaged.asm — 3,27 File Read Paged (F-16 du fork firmware) : lecture d'un fichier
; directement en VRAM (page $80/$81), RAM graphique ($90) ou RAM 6502 ($00).
; Fichier de test readpaged.bin : 1024 octets, octet i = i & $FF.
; Sortie attendue (console) :
;   OPEN 00 / P81 00 0140 / V 00 01 02 03 / P90 00 00C8 / G 40 41 42 43 / P00 00 0010
;   R 08 09 0A 0B / BAD 15 / WRAP 15 / END81 15 / LAST81 00 0001 / REST 00 01E7
;   EOF 02 0000 / CLOSE 00 / V2 3C 3D 3E 3F / END   (15 = FIOERROR_INVALID_PARAMETER, 02 = EOF)
; La ligne 230 de l'écran reçoit un dégradé 0..255,0..63 (320 octets lus en $81:$1F80).
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=readpaged.neo6502 readpaged.asm

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

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12                   ; CLS : la console ne doit pas défiler (un défilement redessine tout l'écran)
  jsr wchar
  ldx #<sopen
  ldy #>sopen
  jsr print
  stz API_PARAMETERS        ; 3,4 canal 0, lecture
  lda #<fname
  sta API_PARAMETERS+1
  lda #>fname
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #3
  jsr api
  jsr perr

  ldx #<sp81                ; 320 octets -> $81:$1F80 (ligne 230 = 73600 = $11F80)
  ldy #>sp81
  jsr print
  lda #$81
  ldx #<$1F80
  ldy #>$1F80
  jsr setdest
  lda #<320
  ldx #>320
  jsr readpaged
  ldx #<sv
  ldy #>sv
  jsr print
  lda #$81                  ; relecture par 12,2 : $81:$1F80 -> $B00, 4 octets
  ldx #<$1F80
  ldy #>$1F80
  jsr copyback

  ldx #<sp90                ; 200 octets -> $90:$0100
  ldy #>sp90
  jsr print
  lda #$90
  ldx #<$0100
  ldy #>$0100
  jsr setdest
  lda #<200
  ldx #>200
  jsr readpaged
  ldx #<sg
  ldy #>sg
  jsr print
  lda #$90
  ldx #<$0100
  ldy #>$0100
  jsr copyback

  ldx #<sp00                ; 16 octets -> $00:$B00
  ldy #>sp00
  jsr print
  lda #$00
  ldx #<buf
  ldy #>buf
  jsr setdest
  lda #16
  ldx #0
  jsr readpaged
  ldx #<sr
  ldy #>sr
  jsr print
  jsr dump4

  ldx #<sbad                ; page inconnue
  ldy #>sbad
  jsr print
  lda #$50
  ldx #0
  ldy #0
  jsr setdest
  lda #1
  ldx #0
  jsr readpaged

  ldx #<swrap               ; débordement 16 bits
  ldy #>swrap
  jsr print
  lda #$80
  ldx #<$FFF0
  ldy #>$FFF0
  jsr setdest
  lda #32
  ldx #0
  jsr readpaged

  ldx #<send81              ; $81:$2C00 = 76800 = hors VRAM
  ldy #>send81
  jsr print
  lda #$81
  ldx #<$2C00
  ldy #>$2C00
  jsr setdest
  lda #1
  ldx #0
  jsr readpaged

  ldx #<slast81             ; $81:$2BFF = dernier octet de la VRAM
  ldy #>slast81
  jsr print
  lda #$81
  ldx #<$2BFF
  ldy #>$2BFF
  jsr setdest
  lda #1
  ldx #0
  jsr readpaged

  ldx #<srest               ; le reste (487 octets) en RAM 6502
  ldy #>srest
  jsr print
  lda #$00
  ldx #<buf
  ldy #>buf
  jsr setdest
  lda #<1000
  ldx #>1000
  jsr readpaged

  ldx #<seof                ; fin de fichier
  ldy #>seof
  jsr print
  lda #$00
  ldx #<buf
  ldy #>buf
  jsr setdest
  lda #16
  ldx #0
  jsr readpaged

  ldx #<sclose
  ldy #>sclose
  jsr print
  stz API_PARAMETERS
  lda #5
  ldx #3
  jsr api
  jsr perr

  ldx #<sv2                 ; la ligne 230 est-elle intacte après la sortie console ?
  ldy #>sv2
  jsr print
  lda #$81
  ldx #<($1F80+60)
  ldy #>($1F80+60)
  jsr copyback

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  jmp halt

; --- A = page, X/Y = adresse -> P1..P3 (canal 0)
setdest:
  stz API_PARAMETERS
  sta API_PARAMETERS+1
  stx API_PARAMETERS+2
  sty API_PARAMETERS+3
  rts

; --- A/X = taille ; appelle 3,27 puis affiche "err nnnn" + CR
readpaged:
  sta API_PARAMETERS+4
  stx API_PARAMETERS+5
  lda #27
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda API_PARAMETERS+5
  jsr hex
  lda API_PARAMETERS+4
  jsr hex
  jmp cr

; --- 12,2 : 4 octets de A:X/Y -> $B00, puis dump4
copyback:
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<buf
  sta API_PARAMETERS+4
  lda #>buf
  sta API_PARAMETERS+5
  lda #4
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
dump4:
  ldx #0
d4:
  lda buf,x
  phx
  jsr hex
  lda #' '
  jsr wchar
  plx
  inx
  cpx #4
  bne d4
  jmp cr

perr:
  lda API_ERROR
  jsr hex
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

sopen:   .text "OPEN ", 0
sp81:    .text "P81 ", 0
sv:      .text "V ", 0
sp90:    .text "P90 ", 0
sg:      .text "G ", 0
sp00:    .text "P00 ", 0
sr:      .text "R ", 0
sbad:    .text "BAD ", 0
swrap:   .text "WRAP ", 0
send81:  .text "END81 ", 0
slast81: .text "LAST81 ", 0
srest:   .text "REST ", 0
seof:    .text "EOF ", 0
sclose:  .text "CLOSE ", 0
sv2:     .text "V2 ", 0
sfin:    .text "END", 13, 0
fname:   .ptext "readpaged.bin"
