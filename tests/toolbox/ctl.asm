; ctl.asm — Toolbox groupe 36 Control Manager (F-44 du fork firmware) : bouton, case à cocher, ascenseur,
; champ de texte dans une fenêtre (10,20)-(200,150) ; contenu en (11,31)-(199,149).
; Sortie attendue (console) :
;   NEW 01 02 03 04 / FIND 01 01 / TRACK 00 0F 00 01 / CHECK 01 0001 / SCROLL 02 03 / SVAL 0001 0002 0007
;   FIND 03 06 / TEXT 01 02 41 62 / BS 01 01 41 / DISABLED 00 / DISP 00 00 / END
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=ctl.neo6502 ctl.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
rect  = $E00
buf   = $E10
rec   = $E20
tbuf  = $E30                ; tampon du champ de texte (longueur + 16)

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
  jsr setrect               ; fenêtre (10,20)-(200,150) "Ctl"
  .byte 10,0, 20,0, 200,0, 150,0
  lda #<rect
  sta API_PARAMETERS
  lda #>rect
  sta API_PARAMETERS+1
  lda #<twin
  sta API_PARAMETERS+2
  lda #>twin
  sta API_PARAMETERS+3
  lda #1
  sta API_PARAMETERS+4
  lda #1
  ldx #34
  jsr api
  jsr flush

  ldx #<snew                ; NEW : bouton (10,10)-(60,26), case (10,30)-(70,38), ascenseur (170,10)-(178,90) max 10, texte (10,50)-(150,62) max 16
  ldy #>snew
  jsr print
  jsr setrect
  .byte 10,0, 10,0, 60,0, 26,0
  lda #1
  ldx #<tok
  ldy #>tok
  jsr newctl
  .byte 0,0
  jsr setrect
  .byte 10,0, 30,0, 70,0, 38,0
  lda #2
  ldx #<tchk
  ldy #>tchk
  jsr newctl
  .byte 0,0
  jsr setrect
  .byte 170,0, 10,0, 178,0, 90,0
  lda #4
  ldx #<tok
  ldy #>tok
  jsr newctl
  .byte 10,0
  jsr setrect
  .byte 10,0, 50,0, 150,0, 62,0
  lda #5
  ldx #<tbuf
  ldy #>tbuf
  jsr newctl
  .byte 16,0
  jsr cr

  ldx #<sfind               ; FIND (30,45) -> bouton 1, corps
  ldy #>sfind
  jsr print
  ldx #30
  ldy #45
  jsr find
  jsr cr

  ldx #<strack              ; TRACK bouton : down dedans -> 0, pixel (25,50)=0F ; move dehors -> pixel 00 ; up dedans -> 1
  ldy #>strack
  jsr print
  lda #1
  ldx #30
  ldy #45
  jsr track0
  lda API_PARAMETERS+6
  jsr hex
  ldx #25
  ldy #50
  jsr pixel
  lda #1
  ldx #5
  ldy #5
  jsr track1
  ldx #25
  ldy #50
  jsr pixel
  lda #1
  ldx #30
  ldy #45
  jsr track1
  lda #1
  ldx #30
  ldy #45
  jsr track2
  lda #' '
  jsr wchar
  lda API_PARAMETERS+6
  jsr hex
  jsr cr

  ldx #<scheck              ; CHECK : down/up sur la case -> acted 1, valeur 1
  ldy #>scheck
  jsr print
  lda #2
  ldx #25
  ldy #65
  jsr track0
  lda #2
  ldx #25
  ldy #65
  jsr track2
  lda API_PARAMETERS+6
  jsr hex
  lda #2
  jsr getval
  jsr cr

  ldx #<sscroll             ; SCROLL : parties : (185,44) flèche haut = 02, (185,118) flèche bas = 03
  ldy #>sscroll
  jsr print
  ldx #185
  ldy #44
  jsr findpart
  ldx #185
  ldy #118
  jsr findpart
  jsr cr

  ldx #<ssval               ; SVAL : clic flèche bas -> 1 ; clic page bas (185,100) -> 2 ; Set Value 7 -> 7
  ldy #>ssval
  jsr print
  lda #3
  ldx #185
  ldy #118
  jsr track0
  lda #3
  ldx #185
  ldy #118
  jsr track2
  lda #3
  jsr getval
  lda #3
  ldx #185
  ldy #100
  jsr track0
  lda #3
  ldx #185
  ldy #100
  jsr track2
  lda #3
  jsr getval
  lda #3
  sta API_PARAMETERS
  lda #7
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #5
  ldx #36
  jsr api
  lda #3
  jsr getval
  jsr cr

  ldx #<sfind               ; FIND (185,93) : curseur en 41+8+60*7/10 = 91..94 -> partie 6
  ldy #>sfind
  jsr print
  ldx #185
  ldy #93
  jsr find
  jsr cr

  ldx #<stext               ; TEXT : 'A' puis 'b' -> changed 1, tampon "Ab"
  ldy #>stext
  jsr print
  lda #'A'
  jsr key
  lda #'b'
  jsr key
  lda API_PARAMETERS+2
  jsr hex
  jsr ptbuf
  jsr cr
  ldx #<sbs                 ; BS : effacement -> "A"
  ldy #>sbs
  jsr print
  lda #8
  jsr key
  lda API_PARAMETERS+2
  jsr hex
  jsr ptbuf
  jsr cr

  ldx #<sdisabled           ; DISABLED : bouton inactif, up dedans -> 0
  ldy #>sdisabled
  jsr print
  lda #1
  sta API_PARAMETERS
  lda #1
  sta API_PARAMETERS+1
  lda #7
  ldx #36
  jsr api
  lda #1
  ldx #30
  ldy #45
  jsr track2
  lda API_PARAMETERS+6
  jsr hex
  jsr cr

  ldx #<sdisp               ; DISP : fenêtre détruite -> contrôle 1 inconnu (erreur 01) ; "00 00" = Dispose fenêtre ok, puis Find (30,45) = 0
  ldy #>sdisp
  jsr print
  lda #1
  sta API_PARAMETERS
  lda #2
  ldx #34
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  ldx #30
  ldy #45
  jsr findid
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts                       ; retour à l'appelant (sys du BASIC, ou NeoDOS pour un .NEO) ; reset sinon

; --- 36,1 : A = genre, X/Y = texte, rect, max = 2 octets inline ; affiche " id"
newctl:
  sta API_PARAMETERS
  stx API_PARAMETERS+4
  sty API_PARAMETERS+5
  pla
  sta ptr
  pla
  sta ptr+1
  ldy #1
  lda (ptr),y
  sta API_PARAMETERS+6
  iny
  lda (ptr),y
  sta API_PARAMETERS+7
  lda ptr
  clc
  adc #2
  sta ptr
  lda ptr+1
  adc #0
  pha
  lda ptr
  pha
  lda #1
  sta API_PARAMETERS+1
  lda #<rect
  sta API_PARAMETERS+2
  lda #>rect
  sta API_PARAMETERS+3
  lda #1
  ldx #36
  jsr api
  lda API_PARAMETERS        ; avant wchar, qui écrase P0
  sta buf
  lda #' '
  jsr wchar
  lda buf
  jmp hex

; --- 36,8 (X,Y) -> "id part" / findpart : " part" / findid : "id"
find:
  jsr findraw
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda buf+1
  jmp hex
findpart:
  jsr findraw
  lda #' '
  jsr wchar
  lda buf+1
  jmp hex
findid:
  jsr findraw
  lda buf
  jmp hex
findraw:
  stx API_PARAMETERS
  stz API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #8
  ldx #36
  jsr api
  lda API_PARAMETERS+4
  sta buf
  lda API_PARAMETERS+5
  sta buf+1
  rts

; --- 36,9 : A = id, X/Y = point, phase 0/1/2
track0:
  pha
  lda #0
  bra trk
track1:
  pha
  lda #1
  bra trk
track2:
  pha
  lda #2
trk:
  sta API_PARAMETERS+5
  pla
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  stz API_PARAMETERS+2
  sty API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #9
  ldx #36
  jmp api

; --- 36,6 : A = id -> " vvvv"
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

; --- 36,10 : A = touche, champ 4
key:
  sta API_PARAMETERS+1
  lda #4
  sta API_PARAMETERS
  lda #10
  ldx #36
  jmp api

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

flush:
  stz API_PARAMETERS
  stz API_PARAMETERS+1
  lda #4
  ldx #33
  jmp api

; --- copie les 8 octets qui suivent l'appel dans rect
setrect:
  pla
  sta ptr
  pla
  sta ptr+1
  ldy #1
sr1:
  lda (ptr),y
  sta rect-1,y
  iny
  cpy #9
  bne sr1
  lda ptr
  clc
  adc #8                    ; retour sur le dernier octet des données (RTS ajoute 1)
  sta ptr
  lda ptr+1
  adc #0
  pha
  lda ptr
  pha
  rts

; --- 32,A avec P0-1 = rect
apirect:
  ldx #<rect
  stx API_PARAMETERS
  ldx #>rect
  stx API_PARAMETERS+1
  ldx #32
  jmp api

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

snew:      .text "NEW", 0
sfind:     .text "FIND ", 0
strack:    .text "TRACK ", 0
scheck:    .text "CHECK ", 0
sscroll:   .text "SCROLL", 0
ssval:     .text "SVAL", 0
stext:     .text "TEXT ", 0
sbs:       .text "BS ", 0
sdisabled: .text "DISABLED ", 0
sdisp:     .text "DISP ", 0
sfin:      .text "END", 13, 0
twin:      .ptext "Ctl"
tok:       .ptext "OK"
tchk:      .ptext "Chk"
