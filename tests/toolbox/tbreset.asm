; tbreset.asm — 32,19 Toolbox Reset (Trinity T-88) : fenêtres, menus (et bornes de la barre) oubliés.
; Une fenêtre et un menu File dans une barre bornée (100,250), puis 32,19 : plus de fenêtre au premier plan,
; plus de menu sous (110,5) ; un nouveau menu reprend le numéro 01 et la barre pleine largeur (titre en x = 4).
; Sortie attendue (console) :
;   BND 00 / NEW 00 01 / FRONT 01 / RESET 00 / FRONT 00 / SEL 00 / NEW 00 01 / SEL 01 / END 00 00 / END
; Auteur : bmarty <bmarty@mailo.com>

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

  jsr setrect               ; fenêtre (20,60)-(150,150) "Win"
  .byte 20,0, 60,0, 150,0, 150,0
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
  ldx #100                  ; BND (100,250) puis NEW File -> titre en x = 104
  ldy #250
  jsr bounds
  ldx #<snew
  ldy #>snew
  jsr print
  ldx #<mfile
  ldy #>mfile
  jsr newmenu
  ldx #<sfront              ; FRONT : 34,10 -> fenêtre 01
  ldy #>sfront
  jsr print
  jsr front
  jsr cr

  lda #19                   ; 32,19 Toolbox Reset
  ldx #32
  jsr api
  ldx #<sreset
  ldy #>sreset
  jsr print
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<sfront              ; FRONT -> 00 : plus de fenêtre
  ldy #>sfront
  jsr print
  jsr front
  jsr cr
  ldx #<ssel                ; SEL (110,5) -> 00 : plus de menu
  ldy #>ssel
  jsr print
  ldx #110
  ldy #5
  jsr select
  jsr cr
  ldx #<snew                ; NEW File -> id 01 de nouveau, barre pleine largeur : SEL (10,5) -> 01
  ldy #>snew
  jsr print
  ldx #<mfile
  ldy #>mfile
  jsr newmenu
  ldx #<ssel
  ldy #>ssel
  jsr print
  ldx #10
  ldy #5
  jsr select
  jsr cr
  ldx #<send
  ldy #>send
  jsr print
  jsr trackend
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts                       ; retour à l'appelant (sys du BASIC, ou NeoDOS pour un .NEO) ; reset sinon

; --- 34,10 Front Window -> id
front:
  lda #10
  ldx #34
  jsr api
  lda API_PARAMETERS
  jmp hex

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

sbnd:   .text "BND ", 0
sfront: .text "FRONT ", 0
sreset: .text "RESET ", 0
sbad:   .text "BAD ", 0
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
twin:   .ptext "Win"
mfile:  .ptext "File"
        .byte 0
        .ptext "Open"
        .byte 1
        .ptext "Save"
        .byte $80
        .ptext ""
        .byte 0
        .ptext "Quit"
        .byte $FF
medit:  .ptext "Edit"
        .byte 0
        .ptext "Undo"
        .byte $FF
