; sprmode1.asm — T-111 : sprites opaques (6,6 = 1), relus sur un pixel du mode 0.
; Images 16 x 16 : n° 0 couleur 1, n° 1 couleur 2, n° 2 vide (tout transparent).
; Pixel relu : (100,200), octet $FA64 de la VRAM, copié par 12,2 en $3200. Ligne : « lettre erreur octet ».
;   A 6,6 mode 2                                        -> 01 00 (refusé)
;   B 6,6 mode 1 ; sprite 1 (couleur 2) en (100,200)     -> 00 20
;   C sprite 0 (couleur 1) au même endroit               -> 00 10 (le plus petit numéro devant ; XOR : 30)
;   D 6,3 sprite 0                                       -> 00 20 (sprite 1 redessiné dessous)
;   E sprite 0 réaffiché (ancre $47 : forcer)            -> 00 10
;   F 6,3 sprite 1 (derrière)                            -> 00 10 (sprite 0 reste)
;   G 12,2 écrit $05, puis 6,3 sprite 0                  -> 00 05 (effacement propre ; XOR : 15)
;   H sprite 0 réaffiché (forcer), puis 6,6 mode 0       -> 00 15 (redessiné en XOR)
;   I 6,3 sprite 0 (XOR)                                 -> 00 05
;   J 6,6 mode 1 ; sprite 1 réaffiché (forcer) ;
;     sprite 0 avec l'image vide (forcer)                -> 00 25 (couleur 0 transparente)
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
err   = $F7
LU    = $3200
OCT   = $3230
IMG   = $3300               ; 3 images de 128 octets
PIX   = $FA64               ; 200 * 320 + 100

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #1                    ; T-121 : sprites remis dans l'état du démarrage. Sur la carte, NeoDOS enchaîne les
  ldx #6                    ; programmes sans remise à zéro : 6,1 efface les sprites, mais laisse le mode de
  jsr api                   ; dessin (6,6) et la page des images (6,8) du programme précédent
  stz API_PARAMETERS        ; 6,6 mode 0 (OU exclusif)
  lda #6
  ldx #6
  jsr api
  lda #$90                  ; 6,8 $90 (images en RAM graphique)
  sta API_PARAMETERS
  lda #8
  ldx #6
  jsr api
  lda #12                   ; écran effacé : la console reste en haut, loin du pixel relu
  jsr wchar
  ldx #0
i1:
  lda #$11
  sta IMG,x
  lda #$22
  sta IMG+128,x
  stz IMG+256,x
  inx
  cpx #128
  bne i1
  lda #<entete              ; en-tête -> $90:0000
  ldx #>entete
  ldy #4
  stz API_PARAMETERS+5
  jsr versgfx
  lda #1                    ; 3 images de 128 octets -> $90:0100, $0180, $0200
  sta API_PARAMETERS+5
  lda #<IMG
  ldx #>IMG
  ldy #128
  jsr versgfx
  lda #$80
  sta API_PARAMETERS+4
  lda #1
  sta API_PARAMETERS+5
  lda #<(IMG+128)
  ldx #>(IMG+128)
  ldy #128
  jsr versgfx2
  stz API_PARAMETERS+4
  lda #2
  sta API_PARAMETERS+5
  lda #<(IMG+256)
  ldx #>(IMG+256)
  ldy #128
  jsr versgfx2

  ldx #<mode2               ; A
  ldy #>mode2
  jsr appel
  lda #'A'
  jsr ligne

  ldx #<mode1               ; B
  ldy #>mode1
  jsr appel
  ldx #<s1
  ldy #>s1
  jsr appel
  lda #'B'
  jsr ligne

  ldx #<s0                  ; C
  ldy #>s0
  jsr appel
  lda #'C'
  jsr ligne

  ldx #<cache0              ; D
  ldy #>cache0
  jsr appel
  lda #'D'
  jsr ligne

  ldx #<s0f                 ; E
  ldy #>s0f
  jsr appel
  lda #'E'
  jsr ligne

  ldx #<cache1              ; F
  ldy #>cache1
  jsr appel
  lda #'F'
  jsr ligne

  lda #$05                  ; G
  jsr ecrire
  ldx #<cache0
  ldy #>cache0
  jsr appel
  lda #'G'
  jsr ligne

  ldx #<s0f                 ; H
  ldy #>s0f
  jsr appel
  ldx #<mode0
  ldy #>mode0
  jsr appel
  lda #'H'
  jsr ligne

  ldx #<cache0              ; I
  ldy #>cache0
  jsr appel
  lda #'I'
  jsr ligne

  ldx #<mode1               ; J
  ldy #>mode1
  jsr appel
  ldx #<s1f
  ldy #>s1f
  jsr appel
  ldx #<s0vide
  ldy #>s0vide
  jsr appel
  lda #'J'
  jsr ligne

  ldx #<sfin
  ldy #>sfin
  jsr print
.if NEO
  jmp $FFFF
.endif
  rts

; appel du groupe 6 : enregistrement en X (bas) / Y (haut) = fonction puis 8 paramètres
appel:
  stx ptr
  sty ptr+1
  ldy #1
a1:
  lda (ptr),y
  sta API_PARAMETERS-1,y
  iny
  cpy #9
  bne a1
  lda (ptr)
  ldx #6
  jsr api
  lda API_ERROR
  sta err
  rts

ecrire:
  sta OCT
  stz API_PARAMETERS
  lda #<OCT
  sta API_PARAMETERS+1
  lda #>OCT
  sta API_PARAMETERS+2
  lda #$80
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
  rts

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

; copie Y octets de A:X (page 0) vers la RAM graphique à l'adresse déjà posée en API_PARAMETERS+4,+5
versgfx:
  stz API_PARAMETERS+4
versgfx2:
  sta API_PARAMETERS+1
  stx API_PARAMETERS+2
  stz API_PARAMETERS
  lda #$90
  sta API_PARAMETERS+3
  sty API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jmp api

entete: .byte 1, 0, 3, 0    ; graphismes présents, 0 tuile, 3 sprites 16 x 16, 0 sprite 32 x 32

; fonction, puis paramètres (6,2 : numéro, x, y, image, retournement, ancre)
mode0:  .byte 6, 0, 0,0,0,0,0,0,0
mode1:  .byte 6, 1, 0,0,0,0,0,0,0
mode2:  .byte 6, 2, 0,0,0,0,0,0,0
s0:     .byte 2, 0, <100, >100, <200, >200, 0, 0, 7
s0f:    .byte 2, 0, <100, >100, <200, >200, 0, 0, $47
s1:     .byte 2, 1, <100, >100, <200, >200, 1, 0, 7
s1f:    .byte 2, 1, <100, >100, <200, >200, 1, 0, $47
s0vide: .byte 2, 0, <100, >100, <200, >200, 2, 0, $47
cache0: .byte 3, 0, 0,0,0,0,0,0,0
cache1: .byte 3, 1, 0,0,0,0,0,0,0

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
