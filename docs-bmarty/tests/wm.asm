; wm.asm — Toolbox groupe 34 Window Manager (F-43 du fork firmware) : fenêtres, ordre Z, Find Window,
; événements update (9) / activate (10) du groupe 33, Set Port / End Update.
; Sortie attendue (console) :
;   NEW 00 01 / EV 09 01 00 / EV 0A 01 01 / NEW 00 02 / EV 09 01 00 / EV 09 02 00 / EV 0A 01 00 / EV 0A 02 01
;   FIND 02 01 / FIND 01 02 / FIND 01 03 / FIND 02 04 / SEL 00 / EV 09 02 00 / EV 09 01 00
;   EV 0A 02 00 / EV 0A 01 01 / FIND 01 01 / RECT 0015 006F 0095 00BD / PIX 0F 0F 09 / MOVE 00 / FRONT 01
;   DISP 00 / FRONT 02 / PIX 00 / CLIP 00C9 00A1 0140 00EF / END 00 / BAD 01 / END
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=wm.neo6502 wm.asm

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
  lda #12
  jsr wchar
  lda #1                    ; 32,1 InitGraf
  ldx #32
  jsr api
  stz API_PARAMETERS        ; 33,1 Init Events (tous)
  stz API_PARAMETERS+1
  lda #1
  ldx #33
  jsr api

  ldx #<snew                ; NEW A : (20,100)-(150,190) "Alpha", titre + fermeture
  ldy #>snew
  jsr print
  jsr setrect
  .byte 20,0, 100,0, 150,0, 190,0
  ldx #<talpha
  ldy #>talpha
  lda #3
  jsr newwin
  jsr drain

  ldx #<snew                ; NEW B : (60,130)-(200,220) "Beta", titre + fermeture + taille
  ldy #>snew
  jsr print
  jsr setrect
  .byte 60,0, 130,0, 200,0, 220,0
  ldx #<tbeta
  ldy #>tbeta
  lda #7
  jsr newwin
  jsr drain

  ldx #70                   ; FIND (70,150) -> B contenu (la barre de titre de B couvre y 131-140)
  ldy #150
  jsr find
  ldx #30                   ; (30,105) -> A barre de titre
  ldy #105
  jsr find
  ldx #25                   ; (25,104) -> A case de fermeture
  ldy #104
  jsr find
  ldx #195                  ; (195,215) -> B case de taille
  ldy #215
  jsr find

  ldx #<ssel                ; SEL A au premier plan
  ldy #>ssel
  jsr print
  lda #1
  sta API_PARAMETERS
  lda #4
  ldx #34
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr
  jsr drain
  ldx #70                   ; FIND (70,150) -> A contenu
  ldy #150
  jsr find

  ldx #<srect               ; RECT : contenu de A
  ldy #>srect
  jsr print
  lda #1
  sta API_PARAMETERS
  lda #<rect
  sta API_PARAMETERS+1
  lda #>rect
  sta API_PARAMETERS+2
  lda #8
  ldx #34
  jsr api
  jsr prect

  ldx #<spix                ; PIX : cadre A (20,100)=0F, titre A (140,105)=0F, titre B (180,135)=09
  ldy #>spix
  jsr print
  ldx #20
  ldy #100
  jsr pixel
  ldx #140                  ; barre de titre de A, hors du texte
  ldy #105
  jsr pixel
  ldx #180
  ldy #135
  jsr pixel
  jsr cr

  ldx #<smove               ; MOVE B en (200,150)
  ldy #>smove
  jsr print
  lda #2
  sta API_PARAMETERS
  lda #200
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #150
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #5
  ldx #34
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr
  jsr flush
  jsr front                 ; FRONT 01

  ldx #<sdisp               ; DISP A
  ldy #>sdisp
  jsr print
  lda #1
  sta API_PARAMETERS
  lda #2
  ldx #34
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr
  jsr flush
  jsr front                 ; FRONT 02
  ldx #<spix                ; PIX (20,100) = 00
  ldy #>spix
  jsr print
  ldx #20
  ldy #100
  jsr pixel
  jsr cr

  ldx #<sclip               ; CLIP : Set Port B puis 32,3 Get Clip
  ldy #>sclip
  jsr print
  lda #2
  sta API_PARAMETERS
  lda #12
  ldx #34
  jsr api
  lda #<rect
  sta API_PARAMETERS
  lda #>rect
  sta API_PARAMETERS+1
  lda #3
  ldx #32
  jsr api
  jsr prect

  ldx #<send                ; END UPDATE B
  ldy #>send
  jsr print
  lda #2
  sta API_PARAMETERS
  lda #11
  ldx #34
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<sbad                ; DISPOSE 9 -> 01
  ldy #>sbad
  jsr print
  lda #9
  sta API_PARAMETERS
  lda #2
  ldx #34
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
  jmp halt

; --- 34,1 : rect, titre X/Y, flags A ; affiche "err id"
newwin:
  sta API_PARAMETERS+4
  stx API_PARAMETERS+2
  sty API_PARAMETERS+3
  lda #<rect
  sta API_PARAMETERS
  lda #>rect
  sta API_PARAMETERS+1
  lda #1
  ldx #34
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda API_PARAMETERS+5
  jsr hex
  jmp cr

; --- 34,7 : X, Y (8 bits) -> "FIND id part"
find:
  lda #<sfind
  sta ptr
  lda #>sfind
  sta ptr+1
  phx
  phy
  ldx #<sfind
  ldy #>sfind
  jsr print
  ply
  plx
  stx API_PARAMETERS
  stz API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #7
  ldx #34
  jsr api
  lda API_PARAMETERS+4
  sta buf
  lda API_PARAMETERS+5
  sta buf+1
  lda buf
  jsr hex
  lda #' '
  jsr wchar
  lda buf+1
  jsr hex
  jmp cr

; --- 34,10 -> "FRONT id"
front:
  ldx #<sfront
  ldy #>sfront
  jsr print
  lda #10
  ldx #34
  jsr api
  lda API_PARAMETERS
  jsr hex
  jmp cr

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

snew:   .text "NEW ", 0
sev:    .text "EV ", 0
sfind:  .text "FIND ", 0
ssel:   .text "SEL ", 0
srect:  .text "RECT", 0
spix:   .text "PIX", 0
smove:  .text "MOVE ", 0
sfront: .text "FRONT ", 0
sdisp:  .text "DISP ", 0
sclip:  .text "CLIP", 0
send:   .text "END ", 0
sbad:   .text "BAD ", 0
sfin:   .text "END", 13, 0
talpha: .ptext "Alpha"
tbeta:  .ptext "Beta"
