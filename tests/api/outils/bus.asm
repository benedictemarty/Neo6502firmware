; bus.asm — 5,41 Get Bus Stalls (Trinity T-49) : compteurs de décrochage du bus 6502.
; La machine PIO génère l'horloge du 6502 : quand le firmware est en retard, elle cale et
; l'horloge s'arrête (le processeur est étiré, jamais nourri d'un octet faux). PIO_FDEBUG
; garde la trace de ces calages ; DSPSync les échantillonne ~95 fois par seconde.
;   TX = la donnée d'une lecture n'était pas prête   RX = la file d'adresses non vidée à temps
; Affiche aussi 5,40 (lignes DVI en retard) pour comparer les deux famines.
; Compteurs remis à zéro au lancement. Échap : sortie (les autres touches sont ignorées,
; pour qu'on puisse taper pendant la mesure).
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $B00
count = $B10

start:
  lda #$FF                  ; 5,41 avec P0 = $FF : remise à zéro
  sta API_PARAMETERS
  lda #41
  ldx #5
  jsr api

loop:
  stz API_PARAMETERS        ; 5,41 -> P0-3 TX, P4-7 RX
  lda #41
  ldx #5
  jsr api
  ldx #7
c1:
  lda API_PARAMETERS,x
  sta buf,x
  dex
  bpl c1

  ldx #<stx                 ; "TX " puis le compteur 32 bits
  ldy #>stx
  jsr print
  lda buf+3
  jsr hex
  lda buf+2
  jsr hex
  lda buf+1
  jsr hex
  lda buf
  jsr hex

  ldx #<srx                 ; "  RX "
  ldy #>srx
  jsr print
  lda buf+7
  jsr hex
  lda buf+6
  jsr hex
  lda buf+5
  jsr hex
  lda buf+4
  jsr hex

  lda #40                   ; 5,40 -> lignes DVI en retard
  ldx #5
  jsr api
  ldx #3
c2:
  lda API_PARAMETERS,x
  sta buf,x
  dex
  bpl c2
  ldx #<slate
  ldy #>slate
  jsr print
  lda buf+3
  jsr hex
  lda buf+2
  jsr hex
  lda buf+1
  jsr hex
  lda buf
  jsr hex

  ldx #<ssy                 ; SY : plus longue DSPSync en microsecondes (5,42 avec P0 = 0)
  ldy #>ssy
  jsr print
  lda #0
  jsr read42
  ldx #<scm                 ; CM : plus longue commande API (P0 = 1)
  ldy #>scm
  jsr print
  lda #1
  jsr read42
  jsr cr

  lda #30                   ; ~0,3 s entre deux relevés
  sta count
w1:
  lda #1                    ; 1,1 Timer -> P0-3 (centièmes)
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta buf+8
w2:
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  cmp buf+8
  beq w2
  dec count
  bne w1

  lda #1                    ; 2,1 Read Character : 0 = rien
  ldx #2
  jsr api
  lda API_PARAMETERS
  cmp #27                   ; seul Échap sort : on doit pouvoir marteler le clavier
  beq bye                   ; pendant la mesure, c'est tout l'intérêt
  jmp loop
bye:
  rts

; read42 — A = index de mesure (0 sync, 1 commande) : 5,42 puis affiche les 4 octets
read42:
  sta API_PARAMETERS
  lda #42
  ldx #5
  jsr api
  ldx #3
r2:
  lda API_PARAMETERS,x
  sta buf+16,x
  dex
  bpl r2
  lda buf+19
  jsr hex
  lda buf+18
  jsr hex
  lda buf+17
  jsr hex
  lda buf+16
  jmp hex

api:
  sta API_FUNCTION
  stx API_COMMAND
wait:
  lda API_COMMAND
  bne wait
  rts

cr:
  lda #13
  ; tombe dans wchar

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

stx:     .text "TX ", 0
srx:     .text "  RX ", 0
slate:   .text "  DVI ", 0
ssy:     .text "  SY ", 0
scm:     .text " CM ", 0
