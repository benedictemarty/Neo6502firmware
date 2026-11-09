; dbperf.asm — mesure de la vitesse du groupe 40 Database (T-138) sur la carte : durées en centièmes de seconde (1,1 Timer, 100 Hz).
; Sortie (journal en RAM $2000.., écran) : une ligne par mesure, "LIBELLE tttt" (hexadécimal, 16 bits).
;   APPEND 500 enregistrements de 64 octets / READ 500 lectures de 64 octets / SEARCH du dernier de 500 (pire cas)
;   READ676 : 200 lectures de 676 octets (taille d'une carte de Tyrann)
; Les fichiers m64.db et m676.db sont créés dans le répertoire courant (écrasés s'ils existent).
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=dbperf.neo6502 dbperf.asm

NEO = 0
ptr2   = $F4
logLen = $1FFE
cnt    = $F8                ; compteur 16 bits
tl     = $FA                ; début de la mesure (16 bits)

* = $800
  jmp start

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
recb  = $1000               ; enregistrement à écrire : statut, numéro (2 octets)
rbuf  = $1200               ; enregistrement lu

call40:
  ldx #40
  jmp api

; --- 1,1 : A = 0 début (mémorise), A = 1 fin (affiche la durée après le libellé X/Y)
flag = $FC
hi   = $FE
lo   = $FF
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
  jsr print                 ; libellé X/Y
  lda #' '
  jsr wchar
  lda hi
  jsr hex
  lda lo
  jsr hex
  jmp cr

; --- crée (nom X/Y, taille A bas, Y haut déjà dans P3,4 par l'appelant) et ouvre
creeouvre:
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  lda #3
  sta API_PARAMETERS
  lda #1
  jsr call40
  lda #3
  sta API_PARAMETERS
  lda #2
  jmp call40

; --- 40,5 : enregistrement cnt -> rbuf
lire:
  lda cnt
  sta API_PARAMETERS
  lda cnt+1
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #<rbuf
  sta API_PARAMETERS+3
  lda #>rbuf
  sta API_PARAMETERS+4
  lda #5
  jmp call40

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  lda #1
  sta recb                  ; statut actif

  ; ---- base de 64 octets, 500 enregistrements
  lda #64
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  ldx #<n64
  ldy #>n64
  jsr creeouvre

  lda #0
  jsr timer
  lda #1
  sta cnt
  stz cnt+1
ap1:
  lda cnt
  sta recb+1
  lda cnt+1
  sta recb+2
  lda #<recb
  sta API_PARAMETERS
  lda #>recb
  sta API_PARAMETERS+1
  lda #7
  jsr call40
  inc cnt
  bne ap2
  inc cnt+1
ap2:
  lda cnt
  cmp #<501
  bne ap1
  lda cnt+1
  cmp #>501
  bne ap1
  lda #1
  ldx #<sapp
  ldy #>sapp
  jsr timer

  ; ---- 500 lectures
  lda #0
  jsr timer
  lda #1
  sta cnt
  stz cnt+1
rd1:
  jsr lire
  inc cnt
  bne rd2
  inc cnt+1
rd2:
  lda cnt
  cmp #<501
  bne rd1
  lda cnt+1
  cmp #>501
  bne rd1
  lda #1
  ldx #<srd
  ldy #>srd
  jsr timer

  ; ---- recherche du dernier (numéro 500 aux octets 1-2)
  lda #0
  jsr timer
  lda #1
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #1
  sta API_PARAMETERS+2
  lda #2
  sta API_PARAMETERS+3
  lda #<k500
  sta API_PARAMETERS+4
  lda #>k500
  sta API_PARAMETERS+5
  lda #9
  jsr call40
  lda API_ERROR
  sta $F6
  lda API_PARAMETERS
  sta cnt
  lda API_PARAMETERS+1
  sta cnt+1
  lda #1
  ldx #<ssr
  ldy #>ssr
  jsr timer
  ldx #<serr
  ldy #>serr
  jsr print
  lda $F6
  jsr hex
  lda #' '
  jsr wchar
  lda cnt+1
  jsr hex
  lda cnt
  jsr hex
  jsr cr
  lda #3
  jsr call40

  ; ---- base de 676 octets, 20 enregistrements, 200 lectures
  lda #<676
  sta API_PARAMETERS+3
  lda #>676
  sta API_PARAMETERS+4
  ldx #<n676
  ldy #>n676
  jsr creeouvre
  lda #1
  sta cnt
  stz cnt+1
b1:
  lda cnt
  sta recb+1
  lda #<recb
  sta API_PARAMETERS
  lda #>recb
  sta API_PARAMETERS+1
  lda #7
  jsr call40
  inc cnt
  lda cnt
  cmp #21
  bne b1

  lda #0
  jsr timer
  lda #10
  sta $FD                   ; 10 tours de 20 lectures
b2:
  lda #1
  sta cnt
  stz cnt+1
b3:
  jsr lire
  inc cnt
  lda cnt
  cmp #21
  bne b3
  dec $FD
  bne b2
  lda #1
  ldx #<srd676
  ldy #>srd676
  jsr timer
  lda #3
  jsr call40

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

sapp:   .text "APPEND64x500 ", 0
srd:    .text "READ64x500 ", 0
ssr:    .text "SEARCH64x500 ", 0
serr:   .text "  ERR REC ", 0
srd676: .text "READ676x200 ", 0
sfin:   .text "END", 13, 0
n64:    .ptext "m64.db"
n676:   .ptext "m676.db"
k500:   .word 500
