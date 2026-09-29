; softreset.asm — 1,0 reset logiciel (Trinity T-36, ADR-0001) et 1,27 Apply Boot Choice.
; Mode 2, un menu, une marque en RAM ($3000 = $A5), puis 1,0 : le programme continue (RAM conservée), mode 0,
; plus de menu (35,3 sur son titre -> 0), et 1,27 rend 0 (le choix de démarrage a déjà été appliqué au reset).
; Résultats relevés en $3010 avant tout affichage (1,0 efface la console).
; Sortie attendue : RESET 00 / MODE 00 / RAM A5 / MENU 00 / BOOT 00 / END
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
R     = $3010

start:
  lda #$A5
  sta $3000
  lda #2                    ; mode 2
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda #<mfile               ; un menu (35,1)
  sta API_PARAMETERS
  lda #>mfile
  sta API_PARAMETERS+1
  lda #1
  ldx #35
  jsr api
  lda #0                    ; 1,0 reset logiciel
  ldx #1
  jsr api
  lda API_ERROR
  sta R
  lda #10                   ; 5,10 : mode
  ldx #5
  jsr api
  lda API_PARAMETERS
  sta R+1
  lda $3000                 ; RAM conservée
  sta R+2
  lda #10                   ; 35,3 sur le titre (10,5) : plus de menu -> 0
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #5
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #3
  ldx #35
  jsr api
  lda API_PARAMETERS+4
  sta R+3
  lda #27                   ; 1,27 : choix déjà appliqué -> 0
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta R+4

  stz logLen                ; journal
  lda #$20
  sta logLen+1
  ldx #0
p1:
  phx
  lda lblo,x
  sta ptr
  lda lbhi,x
  sta ptr+1
  ldy #0
p2:
  lda (ptr),y
  beq p3
  phy
  jsr wchar
  ply
  iny
  bra p2
p3:
  plx
  phx
  lda R,x
  jsr hex
  jsr cr
  plx
  inx
  cpx #5
  bne p1
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
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

sreset:  .text "RESET ", 0
smode:   .text "MODE ", 0
sram:    .text "RAM ", 0
smenu:   .text "MENU ", 0
sboot:   .text "BOOT ", 0
sfin:    .text "END", 0
lblo:    .byte <sreset, <smode, <sram, <smenu, <sboot
lbhi:    .byte >sreset, >smode, >sram, >smenu, >sboot
mfile:   .ptext "File"
         .byte 0
         .ptext "Quit"
         .byte $FF
