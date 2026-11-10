; usbread.asm — mesure de la latence de lecture d'un fichier sur la clé USB par le groupe 3 (3,6 Seek + 3,8 Read) : étude « banque sur USB » (T-139).
; Crée usbtest.bin (512 Ko, 3,11 Set Size), puis mesure, en centièmes de seconde (1,1 Timer, 100 Hz), le total de N lectures :
;   R2048  : 200 lectures de 2048 octets à des positions pseudo-aléatoires (seek + read)
;   S2048  : 200 lectures de 2048 octets consécutives (sans seek)
;   R256   : 200 lectures de 256 octets aléatoires
;   R8192  : 100 lectures de 8192 octets aléatoires (taille d'une banque)
;   S8192  : 50 lectures de 8192 octets consécutives
; Sortie (journal en RAM $2000.., écran) : "LIBELLE tttt" (hexadécimal, 16 bits) ; en fin : "ERR xx" (OU des erreurs API) et "GOT xxxx" (taille de la dernière lecture).
; Le fichier est supprimé à la fin. Il est créé dans le répertoire courant de la clé.
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=usbread.neo6502 usbread.asm

NEO = 0
ptr2   = $F4
logLen = $1FFE

* = $800
  jmp start

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
CH    = 3                   ; canal de fichier utilisé
buf   = $4000               ; tampon de lecture (jusqu'à 8 Ko : $4000-$5FFF)

; variables en page zéro
pos   = $E0                 ; position de seek (32 bits)
r     = $E4                 ; registre LFSR (16 bits)
sz    = $E6                 ; taille d'une lecture
n     = $E8                 ; nombre de lectures
cn    = $EA                 ; compteur
rflag = $EC                 ; 1 = positions aléatoires, 0 = séquentiel
lbl   = $ED                 ; libellé (adresse)
errs  = $EF
got   = $D0                 ; taille lue en dernier
t0    = $D2
t1    = $D3
t2    = $D4
flag  = $FC
hi    = $FE
lo    = $FF
tl    = $FA

call3:
  ldx #3
  jmp api

; --- 1,1 : A = 0 début (mémorise), A = 1 fin (affiche la durée après le libellé X/Y)
timer:
  sta flag
  phx
  phy
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta lo
  lda API_PARAMETERS+1
  sta hi
  ply
  plx
  lda flag
  bne tend
  lda lo
  sta tl
  lda hi
  sta tl+1
  rts
tend:
  sec
  lda lo
  sbc tl
  sta lo
  lda hi
  sbc tl+1
  sta hi
  jsr print
  lda #' '
  jsr wchar
  lda hi
  jsr hex
  lda lo
  jsr hex
  jmp cr

; --- 3,6 : seek à pos
doseek:
  lda #CH
  sta API_PARAMETERS
  lda pos
  sta API_PARAMETERS+1
  lda pos+1
  sta API_PARAMETERS+2
  lda pos+2
  sta API_PARAMETERS+3
  lda pos+3
  sta API_PARAMETERS+4
  lda #6
  jsr call3
  lda API_ERROR
  ora errs
  sta errs
  rts

; --- 3,8 : lecture de sz octets vers buf
doread:
  lda #CH
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #>buf
  sta API_PARAMETERS+2
  lda sz
  sta API_PARAMETERS+3
  lda sz+1
  sta API_PARAMETERS+4
  lda #8
  jsr call3
  lda API_ERROR
  ora errs
  sta errs
  lda API_PARAMETERS+3
  sta got
  lda API_PARAMETERS+4
  sta got+1
  rts

; --- pos = 3 * (LFSR suivant) : positions de 0 à 196 605
nextrnd:
  lsr r+1
  ror r
  bcc nx
  lda r+1
  eor #$B4
  sta r+1
nx:
  lda r
  asl a
  sta t0
  lda r+1
  rol a
  sta t1
  lda #0
  rol a
  sta t2
  clc
  lda t0
  adc r
  sta pos
  lda t1
  adc r+1
  sta pos+1
  lda t2
  adc #0
  sta pos+2
  stz pos+3
  rts

; --- bench : sz, n, rflag renseignés ; X/Y = libellé
bench:
  stx lbl
  sty lbl+1
  stz pos
  stz pos+1
  stz pos+2
  stz pos+3
  jsr doseek
  lda #0
  jsr timer
  lda n
  sta cn
  lda n+1
  sta cn+1
bl:
  lda rflag
  beq bread
  jsr nextrnd
  jsr doseek
bread:
  jsr doread
  lda cn
  bne bd
  dec cn+1
bd:
  dec cn
  lda cn
  ora cn+1
  bne bl
  lda #1
  ldx lbl
  ldy lbl+1
  jmp timer

; --- paramètres d'une mesure : X = taille/256 (octet haut), Y = taille (octet bas), A = n, C = aléatoire
setup:
  stx sz+1
  sty sz
  sta n
  stz n+1
  lda #0
  rol a
  sta rflag
  rts

start:
  stz logLen
  lda #$20
  sta logLen+1
  stz errs
  lda #12
  jsr wchar
  lda #$A1
  sta r
  lda #$5C
  sta r+1

  ; ---- création du fichier de 512 Ko
  lda #CH
  sta API_PARAMETERS
  lda #<nom
  sta API_PARAMETERS+1
  lda #>nom
  sta API_PARAMETERS+2
  lda #3
  sta API_PARAMETERS+3
  lda #4
  jsr call3
  lda API_ERROR
  ora errs
  sta errs
  lda #CH
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #8                    ; $00080000 = 524 288
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #11
  jsr call3
  lda API_ERROR
  ora errs
  sta errs

  ldx #8                    ; 2048 aléatoire
  ldy #0
  lda #200
  sec
  jsr setup
  ldx #<sr2048
  ldy #>sr2048
  jsr bench

  ldx #8                    ; 2048 séquentiel
  ldy #0
  lda #200
  clc
  jsr setup
  ldx #<ss2048
  ldy #>ss2048
  jsr bench

  ldx #1                    ; 256 aléatoire
  ldy #0
  lda #200
  sec
  jsr setup
  ldx #<sr256
  ldy #>sr256
  jsr bench

  ldx #$20                  ; 8192 aléatoire
  ldy #0
  lda #100
  sec
  jsr setup
  ldx #<sr8192
  ldy #>sr8192
  jsr bench

  ldx #$20                  ; 8192 séquentiel
  ldy #0
  lda #50
  clc
  jsr setup
  ldx #<ss8192
  ldy #>ss8192
  jsr bench

  ; ---- fermeture, suppression
  lda #CH
  sta API_PARAMETERS
  lda #5
  jsr call3
  lda #<nom
  sta API_PARAMETERS
  lda #>nom
  sta API_PARAMETERS+1
  lda #13
  jsr call3

  ldx #<serr
  ldy #>serr
  jsr print
  lda errs
  jsr hex
  jsr cr
  ldx #<sgot
  ldy #>sgot
  jsr print
  lda got+1
  jsr hex
  lda got
  jsr hex
  jsr cr
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

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

sr2048: .text "R2048x200 ", 0
ss2048: .text "S2048x200 ", 0
sr256:  .text "R256x200 ", 0
sr8192: .text "R8192x100 ", 0
ss8192: .text "S8192x50 ", 0
serr:   .text "ERR ", 0
sgot:   .text "GOT ", 0
sfin:   .text "END", 13, 0
nom:    .ptext "usbtest.bin"
