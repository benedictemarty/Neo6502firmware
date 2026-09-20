; dlg.asm — Toolbox groupe 37 Dialog Manager (F-45 du fork firmware) : dialogue modal depuis un descripteur
; (fenêtre (20,20)-(220,160) avec titre ; contenu (21,31)-(219,159)) puis alerte Yes/No centrée.
; Sortie attendue (console) :
;   NEW 00 01 01 / ITEM 01 01 00 06 / RING 0F 00 0F / CLICK 00 01 02 01 / KEY 04 04 02 41 62 / RET 01 02
;   CHECK 03 0001 / OTHER 00 00 / ALERT 00 02 02 / ESC 02 / YES 00 01 0F / DISP 00 00 01 / END
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=dlg.neo6502 dlg.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $E10
rec   = $E20                ; EventRecord (8 octets)

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  lda #1                    ; 32,1
  ldx #32
  jsr api
  stz API_PARAMETERS        ; 33,1
  stz API_PARAMETERS+1
  lda #1
  ldx #33
  jsr api
  stz tbuf

  ldx #<snew                ; NEW : 37,1 -> erreur, dialogue 1, fenêtre 1
  ldy #>snew
  jsr print
  lda #<desc
  sta API_PARAMETERS
  lda #>desc
  sta API_PARAMETERS+1
  lda #1
  ldx #37
  jsr api
  lda API_ERROR
  jsr hex
  lda API_PARAMETERS+2
  sta buf
  lda API_PARAMETERS+3
  sta buf+1
  lda #' '
  jsr wchar
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda buf+1
  jsr hex
  jsr cr

  ldx #<sitem               ; ITEM : item 1 -> contrôle 1 genre 1 ; item 5 -> 0, genre 6
  ldy #>sitem
  jsr print
  lda #1
  jsr getitem
  lda #5
  jsr getitem
  jsr cr

  ldx #<sring               ; RING : anneau du bouton par défaut (29,129)-(81,149) : (29,140)=0F (30,140)=00 (31,140)=0F
  ldy #>sring
  jsr print
  ldx #29
  ldy #140
  jsr pixel
  ldx #30
  ldy #140
  jsr pixel
  ldx #31
  ldy #140
  jsr pixel
  jsr cr

  ldx #<sclick              ; CLICK : down (100,140) sur Cancel -> item 0, consommé 1 ; up -> item 2, consommé 1
  ldy #>sclick
  jsr print
  lda #4
  ldx #100
  ldy #140
  jsr mouse
  jsr event2
  lda #5
  ldx #100
  ldy #140
  jsr mouse
  jsr event2
  jsr cr

  ldx #<skey                ; KEY : 'A', 'b' -> item 4 chaque fois, tampon "Ab"
  ldy #>skey
  jsr print
  lda #'A'
  jsr key
  jsr event1
  lda #'b'
  jsr key
  jsr event1
  jsr ptbuf
  jsr cr

  ldx #<sret                ; RET : Return -> item 1 ; Escape -> item 2
  ldy #>sret
  jsr print
  lda #13
  jsr key
  jsr event1
  lda #27
  jsr key
  jsr event1
  jsr cr

  ldx #<scheck              ; CHECK : down/up (51,45) sur la case -> item 3 ; valeur du contrôle 3 = 1
  ldy #>scheck
  jsr print
  lda #4
  ldx #51
  ldy #45
  jsr mouse
  jsr event0
  lda #5
  ldx #51
  ldy #45
  jsr mouse
  jsr event1
  lda #3
  jsr getval
  jsr cr

  ldx #<sother              ; OTHER : update de la fenêtre 5 -> consommé 0 ; timer -> consommé 0
  ldy #>sother
  jsr print
  lda #9
  sta rec
  lda #5
  sta rec+1
  jsr eventc
  lda #8
  sta rec
  jsr eventc
  jsr cr

  ldx #<salert              ; ALERT : 37,6 "Save?" Yes/No -> erreur 0, dialogue 2, fenêtre 2
  ldy #>salert
  jsr print
  lda #<msg
  sta API_PARAMETERS
  lda #>msg
  sta API_PARAMETERS+1
  lda #2
  sta API_PARAMETERS+2
  lda #6
  ldx #37
  jsr api
  lda API_ERROR
  jsr hex
  lda API_PARAMETERS+3
  sta buf
  lda API_PARAMETERS+4
  sta buf+1
  lda #' '
  jsr wchar
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda buf+1
  jsr hex
  jsr cr

  ldx #<sesc                ; ESC sur l'alerte (dialogue 2) -> item 2 (No)
  ldy #>sesc
  jsr print
  lda #27
  jsr key
  lda #2
  jsr eventd
  lda API_PARAMETERS+3
  jsr hex
  jsr cr

  ldx #<syes                ; YES : fenêtre (99,95)-(221,145), Yes en (164,120)-(212,136) : down -> 0, up -> item 1 ; anneau (162,128)=0F
  ldy #>syes
  jsr print
  lda #4
  ldx #180
  ldy #128
  jsr mouse
  lda #2
  jsr eventd
  lda API_PARAMETERS+3
  jsr hex
  lda #5
  ldx #180
  ldy #128
  jsr mouse
  lda #2
  jsr eventd
  lda #' '
  jsr wchar
  lda API_PARAMETERS+3
  jsr hex
  ldx #162
  ldy #128
  jsr pixel
  jsr cr

  ldx #<sdisp               ; DISP : dispose 2 -> 00, dispose 1 -> 00, dispose 1 -> 01
  ldy #>sdisp
  jsr print
  lda #2
  jsr dispose
  lda #1
  jsr dispose
  lda #1
  jsr dispose
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  jmp halt

; --- 37,4 : A = item du dialogue 1 -> " ctl kind"
getitem:
  sta API_PARAMETERS+1
  lda #1
  sta API_PARAMETERS
  lda #4
  ldx #37
  jsr api
  lda API_PARAMETERS+2
  sta buf
  lda API_PARAMETERS+3
  sta buf+1
  lda #' '
  jsr wchar
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda buf+1
  jmp hex

; --- 37,2 : A = dialogue -> " err"
dispose:
  sta API_PARAMETERS
  lda #2
  ldx #37
  jsr api
  lda API_ERROR
  sta buf
  lda #' '
  jsr wchar
  lda buf
  jmp hex

; --- EventRecord : A = what, X/Y = position
mouse:
  sta rec
  stz rec+1
  stz rec+2
  stz rec+3
  stx rec+4
  stz rec+5
  sty rec+6
  stz rec+7
  rts

; --- EventRecord : A = ASCII (keyDown)
key:
  sta rec+1
  lda #1
  sta rec
  rts

; --- 37,3 sur le dialogue A ; event0 : rien affiché ; event1 : " item" ; event2 : " item consommé" ; eventc : " consommé"
eventd:
  sta API_PARAMETERS
  lda #<rec
  sta API_PARAMETERS+1
  lda #>rec
  sta API_PARAMETERS+2
  lda #3
  ldx #37
  jmp api
event0:
  lda #1
  jmp eventd
event1:
  jsr event0
  lda #' '
  jsr wchar
  lda API_PARAMETERS+3
  jmp hex
event2:
  jsr event1
  lda #' '
  jsr wchar
  lda API_PARAMETERS+4
  jmp hex
eventc:
  jsr event0
  lda #' '
  jsr wchar
  lda API_PARAMETERS+4
  jmp hex

; --- 36,6 : A = contrôle -> " vvvv"
getval:
  sta API_PARAMETERS
  lda #6
  ldx #36
  jsr api
  lda #' '
  jsr wchar
  lda API_PARAMETERS+2
  jsr hex
  lda API_PARAMETERS+1
  jmp hex

; --- affiche le tampon texte : " len c c"
ptbuf:
  lda #' '
  jsr wchar
  lda tbuf
  jsr hex
  ldx #0
pt1:
  cpx tbuf
  beq pt2
  lda #' '
  jsr wchar
  lda tbuf+1,x
  phx
  jsr hex
  plx
  inx
  bne pt1
pt2:
  rts

; --- lit le pixel (X, Y) de la VRAM par 12,2 et l'affiche " nn" ; adresse = Y*256 + Y*64 + X
pixel:
  stx ptr
  stz ptr+1
  sty buf                   ; buf = Y*64 (16 bits)
  stz buf+1
  ldx #6
p64:
  asl buf
  rol buf+1
  dex
  bne p64
  lda buf
  clc
  adc ptr
  sta ptr
  lda buf+1
  adc ptr+1
  sta ptr+1
  tya                       ; + Y*256
  clc
  adc ptr+1
  sta ptr+1
  lda #$80
  sta API_PARAMETERS
  lda ptr
  sta API_PARAMETERS+1
  lda ptr+1
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<buf
  sta API_PARAMETERS+4
  lda #>buf
  sta API_PARAMETERS+5
  lda #1
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  lda #' '
  jsr wchar
  lda buf
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

snew:      .text "NEW ", 0
sitem:     .text "ITEM", 0
sring:     .text "RING", 0
sclick:    .text "CLICK", 0
skey:      .text "KEY", 0
sret:      .text "RET", 0
scheck:    .text "CHECK", 0
sother:    .text "OTHER", 0
salert:    .text "ALERT ", 0
sesc:      .text "ESC ", 0
syes:      .text "YES ", 0
sdisp:     .text "DISP", 0
sfin:      .text "END", 13, 0
msg:       .ptext "Save?"

; --- descripteur du dialogue (lu en place)
desc:
  .byte 20,0, 20,0, 220,0, 160,0     ; rect écran
  .byte 1                            ; barre de titre
  .ptext "Dlg"
  .byte 1, 2, 10,0, 100,0, 58,0, 116,0, 0,0       ; 1 : bouton OK, par défaut
  .ptext "OK"
  .byte 1, 4, 70,0, 100,0, 118,0, 116,0, 0,0      ; 2 : bouton Cancel, annulation
  .ptext "Cancel"
  .byte 2, 0, 10,0, 10,0, 70,0, 18,0, 0,0         ; 3 : case à cocher
  .ptext "Chk"
  .byte 5, 0, 10,0, 30,0, 150,0, 42,0, 16,0       ; 4 : champ de texte, 16 max
tbuf:
  .byte 0
  .fill 16, 0
  .byte 6, 0, 10,0, 50,0, 150,0, 66,0, 0,0        ; 5 : texte statique, deux lignes
  .ptext "Hello", 13, "World"
  .byte 0
