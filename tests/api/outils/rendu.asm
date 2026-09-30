; rendu.asm — T-90 : marge du cœur 1 (affichage) dans les modes 0, 1 et 2, sur la carte, sans sonde.
; Pour chaque mode : 5,41 $FF (remise à zéro), 2 s au repos, puis lecture de sdbench.dat (256 Ko par 3,27,
; tranches de 4 Ko) ; après chaque phase : 5,42 P0=2 (plus long rappel de ligne, µs), 3 (pire écart entre deux
; rappels, µs : 32 nominal en mode 1, 63 en modes 0 et 2 où le rappel ne vient qu'une ligne sur deux), 4 (rappels
; en retard de plus de deux écarts nominaux, seuil par mode depuis la 0.16.64), et 5,40 (lignes non encodées à
; temps, cumul depuis le changement de mode). Retour au mode 0 et affichage (hexadécimal, 16 bits bas).
; Résultats aussi en $0A00 : par mode et par phase, 4 mots (rappel, écart, longs, retards).
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
blk   = $F2
t0    = $F4
mode  = $F6
out   = $F7                 ; index dans RES
RES   = $0A00
DEST  = $4000

start:
  stz out
  stz mode
mloop:
  lda mode                  ; 5,9 mode
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda #$FF                  ; 5,41 $FF : compteurs à zéro
  sta API_PARAMETERS
  lda #41
  ldx #5
  jsr api
  jsr now                   ; repos : 2 s
w1:
  jsr since
  cpx #0
  bne w2
  cmp #200
  bcc w1
w2:
  jsr grab
  jsr open                  ; lecture de 256 Ko
  lda #64
  sta blk
r1:
  stz API_PARAMETERS
  stz API_PARAMETERS+1
  lda #<DEST
  sta API_PARAMETERS+2
  lda #>DEST
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #$10
  sta API_PARAMETERS+5
  lda #27
  ldx #3
  jsr api
  dec blk
  bne r1
  jsr close
  jsr grab
  inc mode
  lda mode
  cmp #3
  bne mloop

  stz API_PARAMETERS        ; retour au mode 0, affichage
  lda #9
  ldx #5
  jsr api
  lda #12
  jsr wchar
  ldx #<stitle
  ldy #>stitle
  jsr print
  stz out
  stz mode
p1:
  ldx #<sm
  ldy #>sm
  jsr print
  lda mode
  jsr hex
  ldx #<srep
  ldy #>srep
  jsr print
  jsr line
  ldx #<sm
  ldy #>sm
  jsr print
  lda mode
  jsr hex
  ldx #<sdsk
  ldy #>sdsk
  jsr print
  jsr line
  inc mode
  lda mode
  cmp #3
  bne p1
  rts

; --- relève 5,42 P0=2,3,4 et 5,40 dans RES+out (4 mots)
grab:
  lda #2
  jsr t42
  lda #3
  jsr t42
  lda #4
  jsr t42
  lda #40
  ldx #5
  jsr api
  jmp store
t42:
  sta API_PARAMETERS
  lda #42
  ldx #5
  jsr api
store:
  ldx out
  lda API_PARAMETERS
  sta RES,x
  lda API_PARAMETERS+1
  sta RES+1,x
  inx
  inx
  stx out
  rts

; --- affiche 4 mots de RES+out : cb= ecart= longs= retards=
line:
  ldx #<scb
  ldy #>scb
  jsr print
  jsr word
  ldx #<sgap
  ldy #>sgap
  jsr print
  jsr word
  ldx #<slong
  ldy #>slong
  jsr print
  jsr word
  ldx #<slate
  ldy #>slate
  jsr print
  jsr word
  lda #13
  jmp wchar
word:
  ldx out
  lda RES+1,x
  jsr hex
  ldx out
  lda RES,x
  jsr hex
  inc out
  inc out
  rts

open:
  stz API_PARAMETERS
  lda #<fname
  sta API_PARAMETERS+1
  lda #>fname
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #3
  jmp api
close:
  stz API_PARAMETERS
  lda #5
  ldx #3
  jmp api

now:
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta t0
  lda API_PARAMETERS+1
  sta t0+1
  rts
since:                      ; A/X = horloge - t0 (centièmes)
  lda #1
  ldx #1
  jsr api
  sec
  lda API_PARAMETERS
  sbc t0
  pha
  lda API_PARAMETERS+1
  sbc t0+1
  tax
  pla
  rts

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
pl:
  lda (ptr),y
  beq pd
  phy
  jsr wchar
  ply
  iny
  bne pl
pd:
  rts

hex:
  pha
  lsr a
  lsr a
  lsr a
  lsr a
  jsr nib
  pla
  and #15
nib:
  cmp #10
  bcc dg
  adc #6
dg:
  adc #'0'
  jmp wchar

stitle: .text "RENDU T-90 (hex) : rappel us, ecart us, longs, retards", 13, 0
sm:     .text "M", 0
srep:   .text " repos  ", 0
sdsk:   .text " disque ", 0
scb:    .text "cb=", 0
sgap:   .text " ec=", 0
slong:  .text " lg=", 0
slate:  .text " rt=", 0
fname:  .ptext "sdbench.dat"
