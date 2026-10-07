; tile8m.asm — T-118 : durée de 5,43 (Draw Tilemap 8x8) sur la carte, pour un écran du gabarit de Bagman.
; Carte 32 x 28 cases ($3000, RAM 6502), 256 tuiles de 2 bits par pixel ($90:0000, 4 Ko), palette de 16 banques
; ($90:1000), fenêtre 256 x 224 en (32,8). Pour chaque cas : 5,41 $FF (remise à zéro), 5,43, puis 5,42 P0=1 (plus
; longue commande, en µs, 3 octets) :
;   cas 0 mode 0, palette, options 0       cas 1 mode 0, quartet bas       cas 2 mode 0, 0 transparent
;   cas 3 mode 2 (chemin empaqueté), palette, options 0
; Résultats en $0A00 : 4 x (erreur de 5,43, µs sur 3 octets). Retour au mode 0, fin par RTS. Dans les émulateurs
; 5,42 rend 0 : l'outil n'a de sens que sur la carte.
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
cas   = $F2
out   = $F3
MAP   = $3000               ; 32 x 28 x 2 = 1 792 octets
TILES = $4000               ; 4 096 octets, copiés en $90:0000
PAL   = $5000               ; 64 octets, copiés en $90:1000
DESC  = $0B00
RES   = $0A00

start:
  lda #<TILES               ; tuiles : octet i = i * 37 (motif quelconque, non nul)
  sta ptr
  lda #>TILES
  sta ptr+1
  ldx #16
  ldy #0
  lda #0
t1:
  clc
  adc #37
  sta (ptr),y
  iny
  bne t1
  inc ptr+1
  dex
  bne t1
  ldx #63                   ; palette[n] = n
p1:
  txa
  sta PAL,x
  dex
  bpl p1
  lda #<MAP                 ; carte : tuile = (x + 32 y) & 255, banque = y & 15, retournements = x & 3
  sta ptr
  lda #>MAP
  sta ptr+1
  ldx #0                    ; x
  ldy #0                    ; y
m1:
  phy
  tya
  asl a
  asl a
  asl a
  asl a
  asl a
  stx cas
  clc
  adc cas                   ; (x + 32 y) & 255
  sta (ptr)
  ply
  tya
  and #15
  asl a
  asl a                     ; banque dans les bits 10-13 : octet haut bits 2-5
  sta cas
  txa
  and #3
  lsr a
  ror a
  ror a                     ; x & 3 dans les bits 6-7 (retournements)
  ora cas
  phy
  ldy #1
  sta (ptr),y
  ply
  clc
  lda ptr
  adc #2
  sta ptr
  bcc m2
  inc ptr+1
m2:
  inx
  cpx #32
  bne m1
  ldx #0
  iny
  cpy #28
  bne m1
  lda #<TILES               ; 12,2 : TILES -> $90:0000, 4 096 octets
  ldx #>TILES
  ldy #$10
  stz API_PARAMETERS+4
  stz API_PARAMETERS+5
  jsr versgfx
  lda #<PAL                 ; 12,2 : PAL -> $90:1000, 64 octets
  ldx #>PAL
  ldy #0
  stz API_PARAMETERS+4
  phx
  ldx #$10
  stx API_PARAMETERS+5
  plx
  pha
  lda #64
  sta API_PARAMETERS+6
  pla
  jsr versgfx2
  ldx #25                   ; descripteur
d1:
  lda base,x
  sta DESC,x
  dex
  bpl d1
  stz out

  stz cas                   ; cas 0 : mode 0, options 0
  jsr mesure
  lda #1                    ; cas 1 : quartet bas
  sta DESC+9
  jsr mesure
  lda #2                    ; cas 2 : 0 transparent
  sta DESC+9
  jsr mesure
  stz DESC+9                ; cas 3 : mode 2
  lda #2
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  jsr mesure
  stz API_PARAMETERS        ; retour au mode 0
  lda #9
  ldx #5
  jmp api

; --- 5,41 $FF, 5,43 DESC, 5,42 P0=1 -> RES+out : erreur, µs (3 octets)
mesure:
  lda #$FF
  sta API_PARAMETERS
  lda #41
  ldx #5
  jsr api
  lda #<DESC
  sta API_PARAMETERS
  lda #>DESC
  sta API_PARAMETERS+1
  lda #43
  ldx #5
  jsr api
  ldx out
  lda API_ERROR
  sta RES,x
  lda #1
  sta API_PARAMETERS
  lda #42
  ldx #5
  jsr api
  ldx out
  lda API_PARAMETERS
  sta RES+1,x
  lda API_PARAMETERS+1
  sta RES+2,x
  lda API_PARAMETERS+2
  sta RES+3,x
  txa
  clc
  adc #4
  sta out
  rts

; --- 12,2 de A:X (page 0) vers $90:P4-P5, Y pages de 256 octets (versgfx) ou P6 octets (versgfx2)
versgfx:
  stz API_PARAMETERS+6
  sty API_PARAMETERS+7
  bra vg
versgfx2:
  stz API_PARAMETERS+7
vg:
  sta API_PARAMETERS+1
  stx API_PARAMETERS+2
  stz API_PARAMETERS
  lda #$90
  sta API_PARAMETERS+3
  lda #2
  ldx #12
  jmp api

api:
  sta API_FUNCTION
  stx API_COMMAND
wait:
  lda API_COMMAND
  bne wait
  rts

; carte MAP (page 0) 32 x 28, tuiles $90:0000 2 bpp, options 0, palette $90:1000, défilement 0, fenêtre (32,8) 256 x 224
base:
  .word MAP
  .byte 0, 32, 28
  .word $0000
  .byte $90, 2, 0
  .word $1000
  .byte $90, 0
  .word 0, 0
  .word 32, 8, 256, 224
