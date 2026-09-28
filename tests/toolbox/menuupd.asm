; menuupd.asm — T-89 : un menu ouvert au clavier reste dessiné quand le programme redessine ses fenêtres.
; Fenêtre sous les menus ; F10 puis RIGHT (File se ferme : mise à jour de la fenêtre ; Edit s'ouvre) ; le
; programme repeint la fenêtre en blanc (32,8) puis 34,11 End Update ; le firmware redessine le menu ouvert.
; Sortie attendue (console) : PIX 00 0F / END   (sans le correctif : PIX 0F 0F)
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
  jsr setrect               ; fenêtre (10,15)-(150,150) "Win", sous la barre et les menus
  .byte 10,0, 15,0, 150,0, 150,0
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
  ldx #<mfile               ; menus File et Edit
  ldy #>mfile
  jsr newmenu0
  ldx #<medit
  ldy #>medit
  jsr newmenu0
  jsr flush

  lda #$43                  ; F10 : File ; puis RIGHT : File se ferme (mise à jour de la fenêtre), Edit s'ouvre
  jsr mkey
  lda #$4F
  jsr mkey

u1:                         ; le programme traite ses événements : mise à jour de la fenêtre
  jsr getevent
  lda API_PARAMETERS+4
  beq u2
  lda rec
  cmp #9
  bne u1
  lda #15                   ; 32,4 encre 15, 32,8 Paint Rect (0,0)-(160,160) : la fenêtre repeinte en blanc
  sta API_PARAMETERS
  lda #4
  ldx #32
  jsr api
  jsr setrect
  .byte 0,0, 0,0, 160,0, 160,0
  lda #8
  jsr apirect
  lda rec+1                 ; 34,11 End Update
  sta API_PARAMETERS
  lda #11
  ldx #34
  jsr api
  bra u1
u2:
  ldx #<spix                ; PIX : (74,20) dans Undo surligné = 00 si le menu est redessiné ; (10,40) fenêtre = 0F
  ldy #>spix
  jsr print
  ldx #74
  ldy #20
  jsr pixel
  ldx #10
  ldy #40
  jsr pixel
  jsr cr
  lda #$29                  ; ESC
  jsr mkey
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts                       ; retour à l'appelant (sys du BASIC, ou NeoDOS pour un .NEO) ; reset sinon

; --- 35,1 sans affichage
newmenu0:
  stx API_PARAMETERS
  sty API_PARAMETERS+1
  lda #1
  ldx #35
  jmp api

; --- 35,10 avec le code de touche A
mkey:
  sta API_PARAMETERS
  lda #10
  ldx #35
  jmp api

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

spix:   .text "PIX", 0
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
