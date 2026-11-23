; sprmany.asm — T-161 (demande Spellbinder) : 6,12 Sprite Rectangle Collision et 6,13 Sprite Move Many.
; Images de sprbyte (n° 0 : 16 x 16 couleur 1), mode 0 (OU exclusif, quartet haut). Lignes « lettre erreur P0 P2 »
; pour les appels, « lettre erreur octet » pour un pixel relu dans la VRAM.
;   A sprites 0 en (100,120) et 1 en (200,120) ; 6,12 (0,1)               00 00 : rectangles éloignés
;   B sprite 1 en (110,120) ; 6,12                                         00 01 : chevauchement
;   C sprite 1 en (116,120) ; 6,12                                         00 00 : contigus (0 couvre x 100-115)
;   D sprite 1 en (115,120) ; 6,12                                         00 01 : un pixel commun
;   E 6,3 sprite 1 ; 6,12                                                  00 00 : masqué
;   F 6,12 (0,200)                                                         01 00 00 : numéro invalide
;   G 6,13 liste $90:0400 de 2 entrées : sprite 0 en (50,60), 1 en (150,60) 00 00 02 : 2 faites
;   H pixels (50,60) et (150,60)                                           00 10 / 00 10
;   I 6,13 liste en $7FFE, 2 entrées                                       01 FE 00 : hors de la RAM graphique
;   J 6,13 0 entrée                                                        01 00 00
;   K sprite 0 x2 (retournement 4) en (50,60) ; 6,13 sprite 1 en (81,60) ; 6,12   00 01 : le zoom compte
;   L 6,13 sprite 1 en (82,60) ; 6,12                                      00 00
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
pa    = $F8
r0    = $FA
r2    = $FB
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

  lda #<lista
  ldx #>lista
  ldy #10
  stz API_PARAMETERS+4
  lda #4
  sta API_PARAMETERS+5
  lda #<lista
  ldx #>lista
  ldy #10
  jsr versgfx2
  lda #<listb
  ldx #>listb
  ldy #5
  lda #$10
  sta API_PARAMETERS+4
  lda #4
  sta API_PARAMETERS+5
  lda #<listb
  ldx #>listb
  ldy #5
  jsr versgfx2
  lda #<listc
  ldx #>listc
  ldy #5
  lda #$20
  sta API_PARAMETERS+4
  lda #4
  sta API_PARAMETERS+5
  lda #<listc
  ldx #>listc
  ldy #5
  jsr versgfx2
  ldx #<s0
  ldy #>s0
  jsr appel
  ldx #<s1far
  ldy #>s1far
  jsr appel
  ldx #<r01
  ldy #>r01
  jsr appel
  lda #'A'
  jsr lignep
  ldx #<s1a
  ldy #>s1a
  jsr appel
  ldx #<r01
  ldy #>r01
  jsr appel
  lda #'B'
  jsr lignep
  ldx #<s1b
  ldy #>s1b
  jsr appel
  ldx #<r01
  ldy #>r01
  jsr appel
  lda #'C'
  jsr lignep
  ldx #<s1c
  ldy #>s1c
  jsr appel
  ldx #<r01
  ldy #>r01
  jsr appel
  lda #'D'
  jsr lignep
  ldx #<cache1
  ldy #>cache1
  jsr appel
  ldx #<r01
  ldy #>r01
  jsr appel
  lda #'E'
  jsr lignep
  ldx #<r0x
  ldy #>r0x
  jsr appel
  lda #'F'
  jsr lignep
  ldx #<many2
  ldy #>many2
  jsr appel
  lda #'G'
  jsr lignep
  lda #<19250
  sta pa
  lda #>19250
  sta pa+1
  lda #'H'
  jsr ligne
  lda #<19350
  sta pa
  lda #>19350
  sta pa+1
  lda #'h'
  jsr ligne
  ldx #<manyout
  ldy #>manyout
  jsr appel
  lda #'I'
  jsr lignep
  ldx #<many0
  ldy #>many0
  jsr appel
  lda #'J'
  jsr lignep
  ldx #<s0z1
  ldy #>s0z1
  jsr appel
  ldx #<manyb
  ldy #>manyb
  jsr appel
  ldx #<r01
  ldy #>r01
  jsr appel
  lda #'K'
  jsr lignep
  ldx #<manyc
  ldy #>manyc
  jsr appel
  ldx #<r01
  ldy #>r01
  jsr appel
  lda #'L'
  jsr lignep
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
  lda pa
  sta API_PARAMETERS+1
  lda pa+1
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

lignep:
  pha
  lda API_PARAMETERS
  sta r0
  lda API_PARAMETERS+2
  sta r2
  pla
  jsr wchar
  lda #' '
  jsr wchar
  lda err
  jsr hex
  lda #' '
  jsr wchar
  lda r0
  jsr hex
  lda #' '
  jsr wchar
  lda r2
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
zone4:  .byte 9, <$1000, >$1000, <$2000, >$2000, 0,0,0,0
zone1:  .byte 9, <$1000, >$1000, <$0100, >$0100, 0,0,0,0
zonemax: .byte 9, <$7F00, >$7F00, <$0200, >$0200, 0,0,0,0
fond:   .byte 10, 0,0,0,0,0,0,0,0
s0z0:   .byte 2, 0, <100, >100, <120, >120, 0, 0, $47
s0z0b:  .byte 2, 0, <100, >100, <120, >120, 0, 0, $47
s0anc:  .byte 2, 0, <100, >100, <121, >121, 0, $80, 0
s0z3:   .byte 2, 0, <100, >100, <120, >120, 0, 12, 7
s1z1:   .byte 2, 1, <200, >200, <120, >120, $80, 4, $47
mode0:  .byte 6, 0,0,0,0,0,0,0,0
mode1:  .byte 6, 1,0,0,0,0,0,0,0
s0:     .byte 2, 0, <100, >100, <120, >120, 0, 0, $47
s1far:  .byte 2, 1, <200, >200, <120, >120, 0, 0, $47
s1a:    .byte 2, 1, <110, >110, <120, >120, 0, 0, $47
s1c:    .byte 2, 1, <115, >115, <120, >120, 0, 0, $47
s0z1:   .byte 2, 0, <50, >50, <60, >60, 0, 4, $47
r01:    .byte 12, 0, 1, 0,0,0,0,0,0
r0x:    .byte 12, 0, 200, 0,0,0,0,0,0
many2:  .byte 13, <$0400, >$0400, 2, 0,0,0,0,0
manyb:  .byte 13, <$0410, >$0410, 1, 0,0,0,0,0
manyc:  .byte 13, <$0420, >$0420, 1, 0,0,0,0,0
manyout: .byte 13, <$7FFE, >$7FFE, 2, 0,0,0,0,0
many0:  .byte 13, <$0400, >$0400, 0, 0,0,0,0,0
lista:  .byte 0, 50,0, 60,0,  1, 150,0, 60,0
listb:  .byte 1, 81,0, 60,0
listc:  .byte 1, 82,0, 60,0
reset:  .byte 1, 0,0,0,0,0,0,0,0
s0b:    .byte 2, 0, <120, >120, <200, >200, 0, 0, $47
s0c:    .byte 2, 0, <92, >92, <200, >200, 0, 0, $47
s1:     .byte 2, 1, <100, >100, <200, >200, 1, 0, $47
s1b:    .byte 2, 1, <150, >150, <200, >200, 1, 0, $47
img1:   .byte 7, 1, <$0300, >$0300, 16, 16, 1, <$0320, >$0320
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
