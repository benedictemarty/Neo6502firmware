; infopic.asm — T-164 : groupe 41, images Infocom v6 (.EG1 / .MG1), décodage LZW depuis le fichier.
; Fichiers d'essai libres de droits faits par mkpic.py du projet Neo6502Ozmoo (infopic-eg.bin = EG1, infopic-mg.bin = MG1 avec
; palettes) ; attendu calculé par tests/api/infopic_attendu.py à partir du décodeur de référence infopic.py (même projet).
; Mode 2 (320x240, 16 couleurs). Aucun affichage console entre un dessin et la lecture de l'écran (la console écrit sur l'écran) :
; tous les résultats sont rangés en RAM puis écrits à la fin.
;   Sn  erreur du dessin n, puis somme de contrôle (a += pixel, b += a) d'un rectangle de l'écran relu par 5,33
;   Pn  palette lue par 5,38 (entrée, r, g, b)
;   In  41,1 : erreur, largeur, hauteur, drapeaux, indicateurs
;   En  erreurs : numéro absent, entrée sans données, canal invalide
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
ste   = $E0                 ; enregistrement d'étape
ka    = $E2
kb    = $E3
cx    = $E4                 ; x courant (16 bits)
cy    = $E6
nx    = $E7                 ; colonnes restantes
ny    = $E8
rp    = $E9                 ; pointeur de résultats
num   = $EB
res   = $3000               ; résultats : 3 octets par étape (erreur, a, b)
pres  = $3100               ; palette : 4 octets par sonde

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #2                    ; mode 2
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda #0                    ; canal 0 : fichier EG1
  sta API_PARAMETERS
  lda #<fn0
  sta API_PARAMETERS+1
  lda #>fn0
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #3
  jsr api
  lda #1                    ; canal 1 : fichier MG1
  sta API_PARAMETERS
  lda #<fn1
  sta API_PARAMETERS+1
  lda #>fn1
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #3
  jsr api

  lda #<res
  sta rp
  lda #>res
  sta rp+1
  lda #<etapes
  sta ste
  lda #>etapes
  sta ste+1
etape:
  ldy #0
  lda (ste),y               ; canal, $FF = fin
  cmp #$FF
  beq fini
  sta API_PARAMETERS
  iny
  lda (ste),y
  sta API_PARAMETERS+1
  iny
  lda (ste),y
  sta API_PARAMETERS+2
  iny
  lda (ste),y
  sta API_PARAMETERS+3
  iny
  lda (ste),y
  sta API_PARAMETERS+4
  iny
  lda (ste),y
  sta API_PARAMETERS+5
  iny
  lda (ste),y
  sta API_PARAMETERS+6
  iny
  lda (ste),y
  sta API_PARAMETERS+7
  lda #2
  ldx #41
  jsr api
  ldy #0
  lda API_ERROR
  sta (rp),y
  jsr somme
  ldy #1
  lda ka
  sta (rp),y
  iny
  lda kb
  sta (rp),y
  clc
  lda rp
  adc #3
  sta rp
  bcc e1
  inc rp+1
e1:
  clc
  lda ste
  adc #13
  sta ste
  bcc etape
  inc ste+1
  bra etape

fini:
  lda #<pres                ; sondes de palette (entrée, r, g, b) après les dessins
  sta rp
  lda #>pres
  sta rp+1
  ldx #0
sd:
  lda sondes,x
  sta API_PARAMETERS
  phx
  lda #38
  ldx #5
  jsr api
  ldy #0
  lda API_PARAMETERS+1
  sta (rp),y
  iny
  lda API_PARAMETERS+2
  sta (rp),y
  iny
  lda API_PARAMETERS+3
  sta (rp),y
  clc
  lda rp
  adc #3
  sta rp
  bcc sd1
  inc rp+1
sd1:
  plx
  inx
  cpx #nsondes
  bne sd
  jmp sortie

; --- somme de contrôle du rectangle de l'enregistrement ste (octets 8 à 12 : x16, y, largeur, hauteur)
somme:
  stz ka
  stz kb
  ldy #8
  lda (ste),y
  sta cx
  iny
  lda (ste),y
  sta cx+1
  iny
  lda (ste),y
  sta cy
  iny
  iny
  lda (ste),y
  sta ny
sl:
  ldy #8
  lda (ste),y
  sta API_PARAMETERS
  iny
  lda (ste),y
  sta API_PARAMETERS+1
  ldy #11
  lda (ste),y
  sta nx
sp:
  lda cx
  sta API_PARAMETERS
  lda cx+1
  sta API_PARAMETERS+1
  lda cy
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #33
  ldx #5
  jsr api
  lda API_PARAMETERS
  clc
  adc ka
  sta ka
  clc
  adc kb
  sta kb
  inc cx
  bne sx
  inc cx+1
sx:
  dec nx
  bne sp
  ldy #8                    ; retour au bord gauche, ligne suivante
  lda (ste),y
  sta cx
  iny
  lda (ste),y
  sta cx+1
  inc cy
  dec ny
  bne sl
  rts

; --- écriture des résultats
sortie:
  stz ptr
  lda #<res
  sta rp
  lda #>res
  sta rp+1
  stz num
so1:
  lda #'S'
  jsr wchar
  lda num
  jsr hex
  ldy #0
so2:
  phy
  lda #' '
  jsr wchar
  ply
  lda (rp),y
  phy
  jsr hex
  ply
  iny
  cpy #3
  bne so2
  jsr cr
  clc
  lda rp
  adc #3
  sta rp
  bcc so3
  inc rp+1
so3:
  inc num
  lda num
  cmp #netapes
  bne so1

  lda #<pres
  sta rp
  lda #>pres
  sta rp+1
  stz num
pa1:
  lda #'P'
  jsr wchar
  lda num
  jsr hex
  lda #' '
  jsr wchar
  ldx num
  lda sondes,x
  jsr hex
  ldy #0
pa2:
  phy
  lda #' '
  jsr wchar
  ply
  lda (rp),y
  phy
  jsr hex
  ply
  iny
  cpy #3
  bne pa2
  jsr cr
  clc
  lda rp
  adc #3
  sta rp
  bcc pa3
  inc rp+1
pa3:
  inc num
  lda num
  cmp #nsondes
  bne pa1

  ldx #0                    ; I : 41,1 des 5 entrées du MG1
  stx num
in1:
  inc num
  lda #1
  sta API_PARAMETERS
  lda num
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #1
  ldx #41
  jsr api
  lda API_ERROR
  sta ka
  ldx #6
in2:
  lda API_PARAMETERS,x
  sta res,x
  dex
  bpl in2
  lda #'I'
  jsr wchar
  lda num
  jsr hex
  lda #' '
  jsr wchar
  lda ka
  jsr hex
  lda #' '
  jsr wchar
  lda res+1
  jsr hex
  lda res
  jsr hex
  lda #' '
  jsr wchar
  lda res+3
  jsr hex
  lda res+2
  jsr hex
  lda #' '
  jsr wchar
  lda res+5
  jsr hex
  lda res+4
  jsr hex
  lda #' '
  jsr wchar
  lda res+6
  jsr hex
  jsr cr
  lda num
  cmp #5
  beq in3
  jmp in1
in3:

  lda #'E'                  ; E1 : 41,1 numéro absent
  jsr wchar
  lda #'1'
  jsr wchar
  lda #0
  sta API_PARAMETERS
  lda #9
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #1
  jsr info
  jsr lerr
  lda #'E'                  ; E2 : 41,2 entrée sans données (3 du MG1)
  jsr wchar
  lda #'2'
  jsr wchar
  lda #1
  sta API_PARAMETERS
  lda #3
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  stz API_PARAMETERS+5
  stz API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  jsr info
  jsr lerr
  lda #'E'                  ; E3 : 41,1 canal invalide
  jsr wchar
  lda #'3'
  jsr wchar
  lda #9
  sta API_PARAMETERS
  lda #1
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #1
  jsr info
  jsr lerr
  lda #'E'                  ; E4 : 41,2 sur un fichier qui n'est pas une image (canal 7 jamais ouvert)
  jsr wchar
  lda #'4'
  jsr wchar
  lda #7                    ; canal 7 non ouvert
  sta API_PARAMETERS
  lda #1
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #1
  jsr info
  jsr lerr

  ldx #<sfin
  ldy #>sfin
  jsr print
.if NEO
  jmp $FFFF
.endif
  rts

info:
  ldx #41
  jmp api

lerr:                       ; " erreur" puis fin de ligne
  lda API_ERROR
  sta ka
  lda #' '
  jsr wchar
  lda ka
  jsr hex
  jmp cr

cr:
  lda #13
  jmp wchar

fn0: .byte 14
  .text "infopic-eg.bin"
fn1: .byte 14
  .text "infopic-mg.bin"

; étape : canal, numéro (16), x (16), y (16), options, rectangle relu : x (16), y, largeur, hauteur
etapes:
  .byte 0, 4,0, 0,0, 0,0, 0,    0,0, 0, 255, 60      ; 0 : EG1 4, 320x200 en (0,0)
  .byte 0, 1,0, 10,0, 5,0, 0,   0,0, 0, 100, 50      ; 1 : EG1 1, dégradé 64x32 en (10,5)
  .byte 0, 2,0, 0,0, 40,0, 3,   0,0, 36, 255, 30     ; 2 : EG1 2, 640x20 transparent, moitié de largeur, transparence
  .byte 0, 2,0, $9C,$FF, 50,0, 1, 0,0, 45, 255, 25   ; 3 : EG1 2 en (-100,50), moitié de largeur sans transparence (découpe à gauche)
  .byte 1, 5,0, $EC,$FF, $F6,$FF, 8, 0,0, 0, 100, 60 ; 4 : MG1 5 en (-20,-10), couleurs EGA, découpée en haut et à gauche
  .byte 1, 5,0, $FA,0, 150,0, 0, $F0,0, 140, 80, 100 ; 5 : MG1 5 en (250,150), découpée à droite et en bas
  .byte 1, 5,0, 20,0, 100,0, 4, 0,0, 100, 255, 100   ; 6 : MG1 5, 200x100 en (20,100), palette de l'image
  .byte $FF
netapes = 7

; sondes de palette : relues à la fin, donc après l'étape 6 (palette de l'image MG1 5 ; l'entrée 1 reste EGA, 2 à 15 sont celles du fichier)
sondes: .byte 0,1,2,7,15
nsondes = 5

sfin:   .text "END", 0

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
