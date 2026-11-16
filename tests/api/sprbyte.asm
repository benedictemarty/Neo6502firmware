; sprbyte.asm — T-146 (NeoWoW D-01) : sprites « octet entier avec sauvegarde » (6,6 = 2, 6,9, 6,10), relus sur un
; pixel du mode 0. Images 16 x 16 : n° 0 couleur 1, n° 1 couleur 2, n° 2 vide ; image libre (6,7) 16 x 16 en 1 bit
; avec la table {0, $C8}. Fond 8 bits posé par 12,2. Zone de sauvegarde $90:1000, 4 sprites 16 x 16.
; Pixel relu : (100,200), octet $FA64 de la VRAM. Ligne : « lettre erreur octet ». Tous les 6,2 forcent le redessin
; (ancre $47) : un sprite masqué réaffiché au même endroit ne serait pas redessiné (docs/SPRITES-MODE0.md § 3).
;   A 6,9 $1000/$0400 ; 6,6 mode 2 ; fond $A7          -> 00 A7
;   B sprite 1 (couleur 2) en (100,200)                 -> 00 02 (octet entier)
;   C sprite 0 (couleur 1) au même endroit              -> 00 01 (le plus petit numéro devant)
;   D 6,3 sprite 0                                      -> 00 02 (sprite 1 redessiné dessous)
;   E 6,3 sprite 1                                      -> 00 A7 (fond rendu)
;   F sprite 0 en (100,200) puis en (120,200)           -> 00 A7 (déplacé : fond rendu)
;   G sprite 1 en (100,200), sprite 0 en (92,200)       -> 00 01 (chevauchement, 0 devant)
;   H sprite 1 déplacé en (150,200)                     -> 00 01 (groupe restauré puis redessiné)
;   I 6,3 sprite 0                                      -> 00 A7
;   J sprite 1 en (100,200), 12,2 écrit $5C, 6,10       -> 00 02 (fond resauvé, sprite redessiné)
;   K 6,3 sprite 1                                      -> 00 5C (nouveau fond)
;   L 6,7 sprite 2 (1 bit, table {0,$C8}) en (100,200)  -> 00 C8 (couleur 0-255 par la table)
;   M 6,3 sprite 2                                      -> 00 5C
;   N 6,9 $1000/$0100 (1 sprite) ; sprite 1 en (100,200) ;
;     sprite 0 en (92,200)                              -> 03 02 (pas de place : erreur 3, non dessiné)
;   O 6,3 sprite 1                                      -> 00 5C
;   P sprite 1 en (100,200), puis 6,1                   -> 00 5C (fond rendu, quartet haut non effacé)
;   Q 6,9 $7F00/$0200                                   -> 01 5C (hors des 32 Ko)
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
IMG   = $3300               ; 3 images de 128 octets, puis 32 octets 1 bit et la table de 2 octets
PIX   = $FA64               ; 200 * 320 + 100

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #1                    ; 6,1 : sprites dans l'état du démarrage (mode 0, page $90, pas de zone)
  ldx #6
  jsr api
  stz API_PARAMETERS
  lda #6
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
  lda #$FF
  sta IMG+384,x
  inx
  cpx #128
  bne i1
  stz IMG+416               ; table {0, $C8}
  lda #$C8
  sta IMG+417
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
  lda #3                    ; image 1 bit -> $90:0300, table -> $90:0320
  sta API_PARAMETERS+5
  lda #<(IMG+384)
  ldx #>(IMG+384)
  ldy #32
  jsr versgfx
  lda #$20
  sta API_PARAMETERS+4
  lda #3
  sta API_PARAMETERS+5
  lda #<(IMG+416)
  ldx #>(IMG+416)
  ldy #2
  jsr versgfx2

  ldx #<zone4
  ldy #>zone4
  jsr appel
  ldx #<mode2
  ldy #>mode2
  jsr appel
  lda #$A7
  jsr ecrire
  lda #'A'
  jsr ligne

  ldx #<s1
  ldy #>s1
  jsr appel
  lda #'B'
  jsr ligne

  ldx #<s0
  ldy #>s0
  jsr appel
  lda #'C'
  jsr ligne

  ldx #<cache0
  ldy #>cache0
  jsr appel
  lda #'D'
  jsr ligne

  ldx #<cache1
  ldy #>cache1
  jsr appel
  lda #'E'
  jsr ligne

  ldx #<s0
  ldy #>s0
  jsr appel
  ldx #<s0b
  ldy #>s0b
  jsr appel
  lda #'F'
  jsr ligne

  ldx #<cache0
  ldy #>cache0
  jsr appel
  ldx #<s1
  ldy #>s1
  jsr appel
  ldx #<s0c
  ldy #>s0c
  jsr appel
  lda #'G'
  jsr ligne

  ldx #<s1b
  ldy #>s1b
  jsr appel
  lda #'H'
  jsr ligne

  ldx #<cache0
  ldy #>cache0
  jsr appel
  lda #'I'
  jsr ligne

  ldx #<cache1
  ldy #>cache1
  jsr appel
  ldx #<s1
  ldy #>s1
  jsr appel
  lda #$5C
  jsr ecrire
  ldx #<fond
  ldy #>fond
  jsr appel
  lda #'J'
  jsr ligne

  ldx #<cache1
  ldy #>cache1
  jsr appel
  lda #'K'
  jsr ligne

  ldx #<img2
  ldy #>img2
  jsr appel
  ldx #<s2
  ldy #>s2
  jsr appel
  lda #'L'
  jsr ligne

  ldx #<cache2
  ldy #>cache2
  jsr appel
  lda #'M'
  jsr ligne

  ldx #<zone1
  ldy #>zone1
  jsr appel
  ldx #<s1
  ldy #>s1
  jsr appel
  ldx #<s0c
  ldy #>s0c
  jsr appel
  lda #'N'
  jsr ligne

  ldx #<cache1
  ldy #>cache1
  jsr appel
  lda #'O'
  jsr ligne

  ldx #<s1
  ldy #>s1
  jsr appel
  ldx #<reset
  ldy #>reset
  jsr appel
  lda #'P'
  jsr ligne

  ldx #<zonemax
  ldy #>zonemax
  jsr appel
  lda #'Q'
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
mode2:  .byte 6, 2, 0,0,0,0,0,0,0
zone4:  .byte 9, <$1000, >$1000, <$0400, >$0400, 0,0,0,0
zone1:  .byte 9, <$1000, >$1000, <$0100, >$0100, 0,0,0,0
zonemax: .byte 9, <$7F00, >$7F00, <$0200, >$0200, 0,0,0,0
fond:   .byte 10, 0,0,0,0,0,0,0,0
reset:  .byte 1, 0,0,0,0,0,0,0,0
s0:     .byte 2, 0, <100, >100, <200, >200, 0, 0, $47
s0b:    .byte 2, 0, <120, >120, <200, >200, 0, 0, $47
s0c:    .byte 2, 0, <92, >92, <200, >200, 0, 0, $47
s1:     .byte 2, 1, <100, >100, <200, >200, 1, 0, $47
s1b:    .byte 2, 1, <150, >150, <200, >200, 1, 0, $47
img2:   .byte 7, 2, <$0300, >$0300, 16, 16, 1, <$0320, >$0320
s2:     .byte 2, 2, <100, >100, <200, >200, $80, 0, $47
cache0: .byte 3, 0, 0,0,0,0,0,0,0
cache1: .byte 3, 1, 0,0,0,0,0,0,0
cache2: .byte 3, 2, 0,0,0,0,0,0,0

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
