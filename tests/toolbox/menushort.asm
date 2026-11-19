; menushort.asm — 35,11 Menu Shortcut et raccourci aligné à droite par TAB (Trinity T-148, demande D-07 de NeoWin).
; File = Open Ctrl+O, Save Ctrl+S (grisé), séparateur, Quit Ctrl+Q ; Edit = Undo ctrl+z (minuscule), Plain (sans raccourci).
; KEY code mods -> « résultat menu item » (2 = choisi, 0 = non pris) ; puis ROW : la ligne de pixels de l'article Open du menu
; ouvert (# = encre 0, . = fond), le raccourci doit être aligné à droite. Auteur : bmarty <bmarty@mailo.com>

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
rect  = $C00
buf   = $C10
rec   = $C20

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  jsr cr
  jsr cr
  jsr cr
  lda #1                    ; 32,1 InitGraf
  ldx #32
  jsr api
  stz API_PARAMETERS        ; 33,1 Init Events
  stz API_PARAMETERS+1
  lda #1
  ldx #33
  jsr api

  ldx #<snew
  ldy #>snew
  jsr print
  ldx #<mfile
  ldy #>mfile
  jsr newmenu
  ldx #<snew
  ldy #>snew
  jsr print
  ldx #<medit
  ldy #>medit
  jsr newmenu
  lda #$12                  ; Ctrl+O -> 02 01 01
  ldx #1
  jsr sc
  lda #$16                  ; Ctrl+S : article grisé -> 00 00 00
  ldx #1
  jsr sc
  lda #$14                  ; Ctrl+Q -> 02 01 04
  ldx #1
  jsr sc
  lda #$1D                  ; Ctrl droit + z (minuscule dans le texte) -> 02 02 01
  ldx #$10
  jsr sc
  lda #$12                  ; O sans Ctrl -> 00
  ldx #0
  jsr sc
  lda #$13                  ; Ctrl+P : aucun article -> 00
  ldx #1
  jsr sc
  lda #$1E                  ; Ctrl+1 : aucun article -> 00
  ldx #1
  jsr sc
  lda #$43                  ; F10 : File ouvert
  sta API_PARAMETERS
  lda #10
  ldx #35
  jsr api
  ldx #<srow
  ldy #>srow
  jsr print
  jsr row
  jsr cr
  lda #$12                  ; menu ouvert : Ctrl+O -> 00 (35,10 est la fonction pour un menu ouvert)
  ldx #1
  jsr sc
  lda #$29                  ; ESC
  sta API_PARAMETERS
  lda #10
  ldx #35
  jsr api
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts                       ; retour à l'appelant (sys du BASIC, ou NeoDOS pour un .NEO) ; reset sinon

; --- 35,9 (X,Y) -> "BND err" puis CR
bounds:
  phx
  phy
  ldx #<sbnd
  ldy #>sbnd
  jsr print
  ply
  plx
  jsr bounds2
  jmp cr
; --- 35,9 left = X, right = Y (octets bas, hauts à 0) -> "err"
bounds2:
  stx API_PARAMETERS
  stz API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #9
  ldx #35
  jsr api
  lda API_ERROR
  jmp hex

; --- A = code de touche, X = modificateurs : 35,11, affiche "KEY nn mm : r menu item"
sc:
  pha
  phx
  ldx #<sk
  ldy #>sk
  jsr print
  plx
  pla
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  lda #11
  ldx #35
  jsr api
  lda API_PARAMETERS+2
  pha
  lda API_PARAMETERS+1
  pha
  lda API_PARAMETERS
  jsr hex
  lda #' '
  jsr wchar
  pla
  jsr hex
  lda #' '
  jsr wchar
  pla
  jsr hex
  jmp cr

; --- la ligne y = 17 (centre du texte de l'article 1) de x = 0 à 95 : # = 00, . = autre
row:
  stz rowx
rw1:
  ldx rowx
  ldy #17
  jsr pixel
  lda buf
  beq rw2
  lda #'.'
  bra rw3
rw2:
  lda #'#'
rw3:
  jsr wchar
  inc rowx
  lda rowx
  cmp #96
  bne rw1
  rts

pixel:
  stx ptr
  stz ptr+1
  sty buf
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
  tya
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
  rts

; --- 35,1 descripteur X/Y -> "err id"
newmenu:
  stx API_PARAMETERS
  sty API_PARAMETERS+1
  lda #1
  ldx #35
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda API_PARAMETERS+2
  jsr hex
  jmp cr

; --- 35,3 (X,Y) ; affiche le menu ouvert
select:
  jsr select2
  lda API_PARAMETERS+4
  jmp hex
select2:
  stx API_PARAMETERS
  stz API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #3
  ldx #35
  jmp api

; --- 35,4 (X,Y)
track:
  stx API_PARAMETERS
  stz API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #35
  jmp api

; --- 35,5 -> "menu item"
trackend:
  jsr trackend2
  lda API_PARAMETERS
  sta buf
  lda API_PARAMETERS+1
  sta buf+1
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda buf+1
  jmp hex
trackend2:
  lda #5
  ldx #35
  jmp api

; --- vide la file en affichant les événements fenêtre "EV what msg msg2"
drain:
  jsr getevent
  lda API_PARAMETERS+4
  beq drdone
  lda rec
  cmp #9
  bcc drain                 ; autres types : ignorés
  ldx #<sev
  ldy #>sev
  jsr print
  lda rec
  jsr hex
  lda #' '
  jsr wchar
  lda rec+1
  jsr hex
  lda #' '
  jsr wchar
  lda rec+2
  jsr hex
  jsr cr
  bra drain
drdone:
  rts

flush:                      ; 33,4 tout
  stz API_PARAMETERS
  stz API_PARAMETERS+1
  lda #4
  ldx #33
  jmp api

getevent:
  lda #<rec
  sta API_PARAMETERS
  lda #>rec
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #2
  ldx #33
  jmp api

; --- affiche rect : " l t r b" puis CR
prect:
  ldx #0
pr1:
  lda #' '
  jsr wchar
  lda rect+1,x
  phx
  jsr hex
  plx
  lda rect,x
  phx
  jsr hex
  plx
  inx
  inx
  cpx #8
  bne pr1
  jmp cr

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

sbnd:   .text "BND ", 0
sbad:   .text "BAD ", 0
sk:     .text "KEY ", 0
sf:     .text "F10 ", 0
sd:     .text "DOWN ", 0
su:     .text "UP ", 0
sl:     .text "LEFT ", 0
sr:     .text "RIGHT ", 0
se:     .text "ENTER ", 0
sx:     .text "ESC ", 0
snew:   .text "NEW ", 0
sev:    .text "EV ", 0
sbar:   .text "BAR", 0
ssel:   .text "SEL ", 0
sopen:  .text "OPEN", 0
strack: .text "TRACK", 0
send:   .text "END ", 0
safter: .text "AFTER", 0
sdis:   .text "DIS ", 0
sflags: .text "FLAGS ", 0
sdisp:  .text "DISP ", 0
sfin:   .text "END", 13, 0
srow:   .text "ROW ", 0
rowx    = $C30
mfile:  .ptext "File"
        .byte 0
        .ptext "Open", 9, "Ctrl+O"
        .byte 1
        .ptext "Save", 9, "Ctrl+S"
        .byte $80
        .ptext ""
        .byte 0
        .ptext "Quit", 9, "Ctrl+Q"
        .byte $FF
medit:  .ptext "Edit"
        .byte 0
        .ptext "Undo", 9, "Ctrl+z"
        .byte 0
        .ptext "Plain"
        .byte $FF
