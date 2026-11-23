; sprzoom.asm — T-160 (demande Spellbinder) : zoom entier des sprites, bits 2-3 de l'octet de retournement de 6,2
; (0 = x1, 4 = x2, 8 = x3, 12 = x4). Images de sprbyte : n° 0 16 x 16 couleur 1 ; n° 1 vide ; 1 bit 16 x 16 + table {0,$C8}.
; Fond effacé à 0 ; chaque ligne : « lettre erreur octet » relu dans la VRAM au pixel (x,y) donné. Mode 2, zone $90:1000 / $2000 (un sprite x4 de 16 x 16 = 4 096 o).
;   A sprite 0 en (100,120), x1                          (115,120) 01 ; (116,120) 00 ; (100,135) 01 ; (100,136) 00
;   B retournement 4 (x2), même place                    (131,120) 01 ; (132,120) 00 ; (100,151) 01 ; (100,152) 00
;   C ancre 0 (centre) en (100,121), x2 inchangé         (84,105) 01 ; (83,105) 00 ; (115,136) 01 ; (116,105) 00
;   D ancre 7 en (100,120), retournement 12 (x4)         (163,183) 01 ; (164,120) 00 ; (100,184) 00
;   E retournement 0 (retour à x1), même place           (115,120) 01 ; (116,120) 00 ; (163,120) 00
;   F 6,7 sprite 1 (1 bit, table $C8) en (200,120) x2    (231,151) C8 ; (232,120) 00
;   G 6,1 puis mode 0 (OU exclusif), sprite 0 x2         (131,151) 10 ; (132,120) 00
;   H 6,6 = 1 (opaque), sprite 0 x2                      (131,151) 10 ; (132,120) 00
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
  ldx #<s0z0
  ldy #>s0z0
  jsr appel
  lda #<38515
  sta pa
  lda #>38515
  sta pa+1
  lda #'A'
  jsr ligne
  lda #<38516
  sta pa
  lda #>38516
  sta pa+1
  lda #'a'
  jsr ligne
  lda #<43300
  sta pa
  lda #>43300
  sta pa+1
  lda #'b'
  jsr ligne
  lda #<43620
  sta pa
  lda #>43620
  sta pa+1
  lda #'c'
  jsr ligne
  ldx #<s0z1
  ldy #>s0z1
  jsr appel
  lda #<38531
  sta pa
  lda #>38531
  sta pa+1
  lda #'B'
  jsr ligne
  lda #<38532
  sta pa
  lda #>38532
  sta pa+1
  lda #'d'
  jsr ligne
  lda #<48420
  sta pa
  lda #>48420
  sta pa+1
  lda #'e'
  jsr ligne
  lda #<48740
  sta pa
  lda #>48740
  sta pa+1
  lda #'f'
  jsr ligne
  ldx #<s0anc
  ldy #>s0anc
  jsr appel
  lda #<33684
  sta pa
  lda #>33684
  sta pa+1
  lda #'C'
  jsr ligne
  lda #<33683
  sta pa
  lda #>33683
  sta pa+1
  lda #'g'
  jsr ligne
  lda #<43635
  sta pa
  lda #>43635
  sta pa+1
  lda #'h'
  jsr ligne
  lda #<33716
  sta pa
  lda #>33716
  sta pa+1
  lda #'i'
  jsr ligne
  ldx #<s0z3
  ldy #>s0z3
  jsr appel
  lda #<58723
  sta pa
  lda #>58723
  sta pa+1
  lda #'D'
  jsr ligne
  lda #<38564
  sta pa
  lda #>38564
  sta pa+1
  lda #'j'
  jsr ligne
  lda #<58980
  sta pa
  lda #>58980
  sta pa+1
  lda #'k'
  jsr ligne
  ldx #<s0z0b
  ldy #>s0z0b
  jsr appel
  lda #<38515
  sta pa
  lda #>38515
  sta pa+1
  lda #'E'
  jsr ligne
  lda #<38516
  sta pa
  lda #>38516
  sta pa+1
  lda #'l'
  jsr ligne
  lda #<38563
  sta pa
  lda #>38563
  sta pa+1
  lda #'m'
  jsr ligne
  ldx #<img1
  ldy #>img1
  jsr appel
  ldx #<s1z1
  ldy #>s1z1
  jsr appel
  lda #<48551
  sta pa
  lda #>48551
  sta pa+1
  lda #'F'
  jsr ligne
  lda #<38632
  sta pa
  lda #>38632
  sta pa+1
  lda #'n'
  jsr ligne
  ldx #<reset
  ldy #>reset
  jsr appel
  ldx #<mode0
  ldy #>mode0
  jsr appel
  ldx #<s0z1
  ldy #>s0z1
  jsr appel
  lda #<48451
  sta pa
  lda #>48451
  sta pa+1
  lda #'G'
  jsr ligne
  lda #<38532
  sta pa
  lda #>38532
  sta pa+1
  lda #'o'
  jsr ligne
  ldx #<mode1
  ldy #>mode1
  jsr appel
  lda #<48451
  sta pa
  lda #>48451
  sta pa+1
  lda #'H'
  jsr ligne
  lda #<38532
  sta pa
  lda #>38532
  sta pa+1
  lda #'p'
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
s0z1:   .byte 2, 0, <100, >100, <120, >120, 0, 4, $47
s0anc:  .byte 2, 0, <100, >100, <121, >121, 0, $80, 0
s0z3:   .byte 2, 0, <100, >100, <120, >120, 0, 12, 7
s1z1:   .byte 2, 1, <200, >200, <120, >120, $80, 4, $47
mode0:  .byte 6, 0,0,0,0,0,0,0,0
mode1:  .byte 6, 1,0,0,0,0,0,0,0
reset:  .byte 1, 0,0,0,0,0,0,0,0
s0:     .byte 2, 0, <100, >100, <200, >200, 0, 0, $47
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
