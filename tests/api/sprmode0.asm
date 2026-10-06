; sprmode0.asm — T-117 : vérifie ce que dit docs/SPRITES-MODE0.md sur un pixel du mode 0.
; Image : sprite 16 x 16 n° 0, couleur 1 partout (en-tête 01 00 01 00 en $90:0000, image en $90:0100).
; Pixel relu : (100,200), octet $FA64 de la VRAM (page $80), copié par 12,2 en $3200.
; Chaque ligne : « lettre erreur octet ».
;   A 6,2 sprite 0 en (100,200), ancre 7 (haut gauche)      -> 10 (sprite dans les 4 bits hauts)
;   B 12,2 écrit $05 dans l'octet (copie simple)             -> 05 (couche des sprites écrasée)
;   C 6,3 masque le sprite (effacement par OU exclusif)      -> 15 (le « négatif » apparaît)
;     (l'octet est remis à 00 par 12,2)
;   D 6,2 mêmes paramètres qu'en A                           -> 00 (rien n'a changé : pas de redessin)
;   E 6,2 ancre 0 (centre) en (108,208) : même place         -> 10 (ancre et position changent : redessiné)
;   F 12,3 copie $07 vers l'octet, cible au format 4         -> 17 (quartet haut conservé)
;   G 6,3 masque le sprite                                   -> 07 (effacement propre)
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
err   = $F7                 ; API_ERROR, sauvé avant tout affichage (2,6 le remet à 0)
LU    = $3200               ; octet relu dans la VRAM
SRC   = $3210               ; zones du blitter pour F (12 octets chacune)
TGT   = $321C
OCT   = $3230               ; octet à écrire par 12,2 ou 12,3
IMG   = $3300               ; image 16 x 16, 128 octets
PIX   = $FA64               ; 200 * 320 + 100

start:
  stz logLen
  lda #$20
  sta logLen+1
  ldx #0                    ; image : couleur 1 sur tous les pixels
  lda #$11
i1:
  sta IMG,x
  inx
  cpx #128
  bne i1
  lda #<entete              ; en-tête -> $90:0000
  ldx #>entete
  ldy #4
  jsr versgfx0
  lda #<IMG                 ; image -> $90:0100
  ldx #>IMG
  ldy #128
  jsr versgfx1

  ldx #<spriteA
  ldy #>spriteA           ; A
  jsr sprite
  lda #'A'
  jsr ligne

  lda #$05                  ; B
  jsr ecrire
  lda #'B'
  jsr ligne

  jsr masquer               ; C
  lda #'C'
  jsr ligne

  lda #$00
  jsr ecrire
  ldx #<spriteA
  ldy #>spriteA           ; D
  jsr sprite
  lda #'D'
  jsr ligne

  ldx #<spriteE
  ldy #>spriteE           ; E
  jsr sprite
  lda #'E'
  jsr ligne

  lda #$07                  ; F : 12,3, copie, cible au format 4
  sta OCT
  ldy #0
f1:
  lda zonesF,y
  sta SRC,y
  iny
  cpy #24
  bne f1
  stz API_PARAMETERS        ; action 0 : copie
  lda #<SRC
  sta API_PARAMETERS+1
  lda #>SRC
  sta API_PARAMETERS+2
  lda #<TGT
  sta API_PARAMETERS+3
  lda #>TGT
  sta API_PARAMETERS+4
  lda #3
  ldx #12
  jsr api
  lda API_ERROR
  sta err
  lda #'F'
  jsr ligne

  jsr masquer               ; G
  lda #'G'
  jsr ligne

  ldx #<sfin
  ldy #>sfin
  jsr print
.if NEO
  jmp $FFFF
.endif
  rts

; 6,2 avec les 8 octets de paramètres en X (bas) / Y (haut)
sprite:
  stx ptr
  sty ptr+1
  ldy #0
s1:
  lda (ptr),y
  sta API_PARAMETERS,y
  iny
  cpy #8
  bne s1
  lda #2
  ldx #6
  jsr api
  lda API_ERROR
  sta err
  rts

masquer:
  stz API_PARAMETERS        ; sprite 0
  lda #3
  ldx #6
  jsr api
  lda API_ERROR
  sta err
  rts

; écrit A dans l'octet du pixel (12,2, octet entier)
ecrire:
  sta OCT
  stz API_PARAMETERS        ; de la page 0, OCT
  lda #<OCT
  sta API_PARAMETERS+1
  lda #>OCT
  sta API_PARAMETERS+2
  lda #$80                  ; vers la VRAM, PIX
  sta API_PARAMETERS+3
  lda #<PIX
  sta API_PARAMETERS+4
  lda #>PIX
  sta API_PARAMETERS+5
  lda #1
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  lda API_ERROR
  sta err
  rts

; « lettre erreur octet » : relit l'octet du pixel par 12,2 vers LU
ligne:
  jsr wchar
  lda #$80
  sta API_PARAMETERS
  lda #<PIX
  sta API_PARAMETERS+1
  lda #>PIX
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<LU
  sta API_PARAMETERS+4
  lda #>LU
  sta API_PARAMETERS+5
  lda #1
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  lda #' '
  jsr wchar
  lda err
  jsr hex
  lda #' '
  jsr wchar
  lda LU
  jsr hex
  jmp cr

; copie Y octets de A:X (page 0) vers la RAM graphique, en $0000 ou $0100
versgfx0:
  stz API_PARAMETERS+5
  bra vg
versgfx1:
  pha
  lda #1
  sta API_PARAMETERS+5
  pla
vg:
  sta API_PARAMETERS+1
  stx API_PARAMETERS+2
  stz API_PARAMETERS
  lda #$90
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  sty API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jmp api

entete: .byte 1, 0, 1, 0    ; graphismes présents, 0 tuile, 1 sprite 16 x 16, 0 sprite 32 x 32

; 6,2 : numéro, x, y, image, retournement, ancre
spriteA: .byte 0, <100, >100, <200, >200, 0, 0, 7
spriteE: .byte 0, <108, >108, <208, >208, 0, 0, 0

; F : source OCT (page 0, pas 1, format 0, 1 ligne de 1 octet) ; cible PIX (page $80, pas 320, format 4)
zonesF: .byte <OCT,>OCT,0,0, 1,0, 0,0,0, 1, 1,0,   <PIX,>PIX,$80,0, <320,>320, 4,0,0,0,0,0

sfin:   .text "END", 0

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
