; mixed.asm — T-165 : mode 4 (graphisme du mode 0 + grille de texte 80 x 25, cellule par cellule), 5,44 / 5,45, 5,13 en mode 4,
; grille lue par la page $91 (12,2) et par la banque RAM $80 (1,18), 41,2 avec couleur de base.
; Attendu : voir mixed.expected (la somme du dessin est recalculée par mixed_attendu.py avec le décodeur de référence d'Ozmoo).
;   M mode 4 -> 00 ; E1 hors grille -> 01 ; C1-C3 découpes (4 segments permis) ; E2 5 segments -> 02 (rien ne change)
;   W 5,13 : 12,2 page $91 glyphes puis attributs ; B même lecture par 1,18 banque $80 ; S 5,45 ; D dessin avec base 37 (somme) ;
;   P palette base 100 (5,38) ; Z mode 0 : 5,44 -> 01, 5,13 -> 01
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
ka    = $E2
kb    = $E3
cx    = $E4
cy    = $E6
nx    = $E7
ny    = $E8
tmp   = $EB
errsave = $EC
dest  = $3000
WIN   = $4000

start:
  stz logLen
  lda #$20
  sta logLen+1

; M : mode 4
  lda #4
  sta API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda #'M'
  ldy #' '
  jsr tag
  jsr lerr
; E1 : rectangle hors de la grille
  lda #0
  ldx #0
  ldy #80
  jsr rect44a
  lda #'E'
  ldy #'1'
  jsr tag
  jsr lerr
; C1 : texte pour les colonnes 27 à 79, toutes les rangées
  lda #27
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #79
  sta API_PARAMETERS+2
  lda #24
  sta API_PARAMETERS+3
  lda #1
  sta API_PARAMETERS+4
  jsr f44
  lda #'C'
  ldy #'1'
  jsr tag
  jsr lerr
; C2 : la cellule (10, 0) en texte -> 4 segments
  lda #10
  jsr one44t
  lda #'C'
  ldy #'2'
  jsr tag
  jsr lerr
; E2 : la cellule (20, 0) en texte -> 6 segments, refusé
  lda #20
  jsr one44t
  lda #'E'
  ldy #'2'
  jsr tag
  jsr lerr
; C3 : la cellule (10, 0) revient au graphisme -> 2 segments, puis (20, 0) en texte devient possible : 4 segments
  lda #10
  jsr one44g
  lda #'C'
  ldy #'3'
  jsr tag
  jsr lerr
  lda #20
  jsr one44t
  lda #'C'
  ldy #'4'
  jsr tag
  jsr lerr
; W : 5,13 HELLO en (27, 0), attribut $1E ; AB en (79, 24), attribut $2F (B coupé : 1 seul octet écrit)
  lda #27
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #$1E
  sta API_PARAMETERS+2
  lda #<shello
  sta API_PARAMETERS+3
  lda #>shello
  sta API_PARAMETERS+4
  lda #13
  ldx #5
  jsr api
  lda #'W'
  ldy #' '
  jsr tag
  jsr lerr
  lda #79
  sta API_PARAMETERS
  lda #24
  sta API_PARAMETERS+1
  lda #$2F
  sta API_PARAMETERS+2
  lda #<sab
  sta API_PARAMETERS+3
  lda #>sab
  sta API_PARAMETERS+4
  lda #13
  ldx #5
  jsr api
  lda #'W'
  ldy #'2'
  jsr tag
  jsr lerr
; R : 12,2 de la page $91 : glyphes en 27 (5 octets), attributs en 2000+27 (5 octets), glyphe de la cellule (79, 24) puis les attributs
  lda #<27
  ldx #>27
  ldy #5
  jsr blit91
  lda #'R'
  ldy #'1'
  jsr tag
  jsr bytes5
  lda #<(2000+27)
  ldx #>(2000+27)
  ldy #5
  jsr blit91
  lda #'R'
  ldy #'2'
  jsr tag
  jsr bytes5
  lda #<(1999)
  ldx #>(1999)
  ldy #5
  jsr blit91
  lda #'R'
  ldy #'3'
  jsr tag
  jsr bytes5
; B : la même grille par 1,18 banque $80 dans la fenêtre $4000
  lda #$80
  sta API_PARAMETERS
  lda #<WIN
  sta API_PARAMETERS+1
  lda #>WIN
  sta API_PARAMETERS+2
  lda #18
  ldx #1
  jsr api
  lda #'B'
  ldy #' '
  jsr tag
  jsr lerr
  ldx #4
b1:
  lda WIN+27,x
  sta dest,x
  dex
  bpl b1
  lda #'B'
  ldy #'1'
  jsr tag
  jsr bytes5
; S : 5,45 : "1111" en (30, 2), "2222" en (30, 3), défilement vers le haut de (30,2)-(33,3), attribut $07
  lda #<s1111
  ldx #>s1111
  ldy #2
  jsr write30
  lda #<s2222
  ldx #>s2222
  ldy #3
  jsr write30
  lda #30
  sta API_PARAMETERS
  lda #2
  sta API_PARAMETERS+1
  lda #33
  sta API_PARAMETERS+2
  lda #3
  sta API_PARAMETERS+3
  lda #1
  sta API_PARAMETERS+4
  lda #$07
  sta API_PARAMETERS+5
  lda #45
  ldx #5
  jsr api
  lda #'S'
  ldy #' '
  jsr tag
  jsr lerr
  lda #<(2*80+30)
  ldx #>(2*80+30)
  ldy #5
  jsr blit91
  lda #'S'
  ldy #'1'
  jsr tag
  jsr bytes5
  lda #<(3*80+30)
  ldx #>(3*80+30)
  ldy #5
  jsr blit91
  lda #'S'
  ldy #'2'
  jsr tag
  jsr bytes5
; D : fichier EG1 canal 0 ; image 1 (64x32) en (0,100) avec la couleur de base 37 ; somme du rectangle relu par 5,33
  lda #0
  sta API_PARAMETERS
  lda #<fn0
  sta API_PARAMETERS+1
  lda #>fn0
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #3
  jsr api
  lda #1
  sta API_PARAMETERS
  lda #<fn1
  sta API_PARAMETERS+1
  lda #>fn1
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #3
  jsr api
  stz API_PARAMETERS            ; canal 0
  lda #1
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #100
  sta API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #$10
  sta API_PARAMETERS+7
  lda #37
  sta API_PARAMETERS+8
  lda #2
  ldx #41
  jsr api
  lda API_ERROR
  sta tmp
  jsr somme
  lda #'D'
  ldy #' '
  jsr tag
  lda tmp
  jsr hex
  lda #' '
  jsr wchar
  lda ka
  jsr hex
  lda kb
  jsr hex
  jsr cr
; P : image 5 du MG1 (200x100) avec palette et base 100 : entrées 102, 115 relues par 5,38
  lda #1
  sta API_PARAMETERS            ; canal 1
  lda #5
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #130
  sta API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #$14
  sta API_PARAMETERS+7
  lda #100
  sta API_PARAMETERS+8
  lda #2
  ldx #41
  jsr api
  lda API_ERROR
  sta tmp
  lda #'P'
  ldy #' '
  jsr tag
  lda tmp
  jsr hex
  jsr cr
  lda #102
  jsr pal38
  lda #115
  jsr pal38
; Z : mode 0, 5,44 et 5,13 refusés
  stz API_PARAMETERS
  lda #9
  ldx #5
  jsr api
  lda #0
  ldx #0
  ldy #1
  jsr rect44a
  lda #'Z'
  ldy #'1'
  jsr tag
  jsr lerr
  lda #<sab
  sta API_PARAMETERS+3
  lda #>sab
  sta API_PARAMETERS+4
  stz API_PARAMETERS
  stz API_PARAMETERS+1
  lda #13
  ldx #5
  jsr api
  lda #'Z'
  ldy #'2'
  jsr tag
  jsr lerr
  ldx #<sfin
  ldy #>sfin
  jsr print
.if NEO
  jmp $FFFF
.endif
  rts

; --- 5,44 : (A,0)-(X?,...) rectangle de l'erreur E1 : colonne gauche A, rangée X, colonne droite Y, rangée 0 ; texte 1
rect44a:
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #1
  sta API_PARAMETERS+4
f44:
  lda #44
  ldx #5
  jmp api
one44t:
  sta API_PARAMETERS
  sta API_PARAMETERS+2
  stz API_PARAMETERS+1
  stz API_PARAMETERS+3
  lda #1
  sta API_PARAMETERS+4
  jmp f44
one44g:
  sta API_PARAMETERS
  sta API_PARAMETERS+2
  stz API_PARAMETERS+1
  stz API_PARAMETERS+3
  stz API_PARAMETERS+4
  jmp f44

; --- 12,2 : page $91 offset X:A (X haut, A bas), Y octets -> dest ($3000)
blit91:
  sta tmp
  lda #$91
  sta API_PARAMETERS
  lda tmp
  sta API_PARAMETERS+1
  stx API_PARAMETERS+2
  stz API_PARAMETERS+3          ; page 0
  lda #<dest
  sta API_PARAMETERS+4
  lda #>dest
  sta API_PARAMETERS+5
  sty API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jmp api

; --- 5,13 : chaîne (A bas, X haut) en colonne 30, rangée Y, attribut $07
write30:
  sta API_PARAMETERS+3
  stx API_PARAMETERS+4
  lda #30
  sta API_PARAMETERS
  sty API_PARAMETERS+1
  lda #$07
  sta API_PARAMETERS+2
  lda #13
  ldx #5
  jmp api

; --- 5,38 : palette de l'entrée A
pal38:
  sta API_PARAMETERS
  sta tmp
  lda #38
  ldx #5
  jsr api
  lda API_PARAMETERS+1
  sta dest
  lda API_PARAMETERS+2
  sta dest+1
  lda API_PARAMETERS+3
  sta dest+2
  lda #'P'
  jsr wchar
  lda tmp
  jsr hex
  ldx #0
pl1:
  lda #' '
  phx
  jsr wchar
  plx
  lda dest,x
  phx
  jsr hex
  plx
  inx
  cpx #3
  bne pl1
  jmp cr

; --- somme de contrôle du rectangle (0,100), 63 x 32 (64 donnerait une somme indépendante de la base), relu par 5,33
somme:
  stz ka
  stz kb
  lda #100
  sta cy
  lda #32
  sta ny
sl:
  stz cx
  stz cx+1
  lda #63
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
  dec nx
  bne sp
  inc cy
  dec ny
  bne sl
  rts

; --- " " + 5 octets de dest en hexadécimal, puis fin de ligne
bytes5:
  ldx #0
by1:
  lda #' '
  phx
  jsr wchar
  plx
  lda dest,x
  phx
  jsr hex
  plx
  inx
  cpx #5
  bne by1
  jmp cr

; --- A = lettre, Y = second caractère : début de ligne "Xy "
tag:
  pha
  lda API_ERROR
  sta errsave
  pla
  jsr wchar
  tya
  jsr wchar
  lda #' '
  jmp wchar

; --- erreur de l'API en hexadécimal + fin de ligne
lerr:
  lda errsave
  jsr hex
  jmp cr

cr:
  lda #13
  jmp wchar

shello: .byte 5
  .text "HELLO"
sab:    .byte 2
  .text "AB"
s1111:  .byte 4
  .text "1111"
s2222:  .byte 4
  .text "2222"
fn0: .byte 14
  .text "infopic-eg.bin"
fn1: .byte 14
  .text "infopic-mg.bin"

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
