; sprfg.asm — T-159 (demande Spellbinder) : 6,11 Sprite Foreground, seuil de premier plan en mode octet entier (6,6 = 2).
; Mêmes images, même fond 8 bits et même pixel relu que sprbyte : (100,200), octet $FA64. Images 16 x 16 : n° 0
; couleur 1, n° 1 couleur 2. Zone de sauvegarde $90:1000, 4 sprites. Tous les 6,2 forcent le redessin (ancre $47).
;   A 6,11 $A0, fond $A7, sprite 1 en (100,200)          -> 00 A7 (fond >= seuil : le sprite ne l'écrase pas)
;   B fond $50 écrit, 6,10                               -> 00 02 (fond < seuil : sprite dessiné)
;   C 6,11 $40 (sprite déjà affiché)                     -> 00 50 (redessiné : fond >= seuil)
;   D 6,11 $60                                           -> 00 02
;   E sprite 0 (couleur 1) en (92,200), devant le 1      -> 00 01 (le seuil ne joue que sur le fond)
;   F 6,3 des deux, fond $A7, 6,11 0, sprite 1           -> 00 02 (seuil coupé : écrase tout)
;   G 6,11 $A0                                           -> 00 A7
;   H 6,1 (remet le seuil à 0), 6,9, 6,6 = 2, sprite 1   -> 00 02 (seuil remis à 0 par 6,1)
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
  ldx #<fgA0
  ldy #>fgA0
  jsr appel
  ldx #<s1
  ldy #>s1
  jsr appel
  lda #'A'
  jsr ligne

  lda #$50
  jsr ecrire
  ldx #<fond
  ldy #>fond
  jsr appel
  lda #'B'
  jsr ligne

  ldx #<fg40
  ldy #>fg40
  jsr appel
  lda #'C'
  jsr ligne

  ldx #<fg60
  ldy #>fg60
  jsr appel
  lda #'D'
  jsr ligne

  ldx #<s0c
  ldy #>s0c
  jsr appel
  lda #'E'
  jsr ligne

  ldx #<cache0
  ldy #>cache0
  jsr appel
  ldx #<cache1
  ldy #>cache1
  jsr appel
  lda #$A7
  jsr ecrire
  ldx #<fg00
  ldy #>fg00
  jsr appel
  ldx #<s1
  ldy #>s1
  jsr appel
  lda #'F'
  jsr ligne

  ldx #<fgA0
  ldy #>fgA0
  jsr appel
  lda #'G'
  jsr ligne

  ldx #<reset
  ldy #>reset
  jsr appel
  ldx #<zone4
  ldy #>zone4
  jsr appel
  ldx #<mode2
  ldy #>mode2
  jsr appel
  ldx #<s1
  ldy #>s1
  jsr appel
  lda #'H'
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
fgA0:   .byte 11, $A0,0,0,0,0,0,0,0
fg40:   .byte 11, $40,0,0,0,0,0,0,0
fg60:   .byte 11, $60,0,0,0,0,0,0,0
fg00:   .byte 11, 0,0,0,0,0,0,0,0
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
