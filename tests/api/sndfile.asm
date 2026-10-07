; sndfile.asm — flux PCM lu dans un fichier sans tampon en RAM 6502 (Trinity T-119) : 8,17 avec le paramètre 7 = 2
; (deux moitiés dans la RAM graphique), chaque moitié rendue remplie par 3,28 en page $90 puis déclarée pleine (8,14).
; Fichier readpaged.bin : 1 024 octets, lus en 2 moitiés de 128 au départ puis 6 recharges, dans l'ordre 0, 1, 0, 1...
; Seuls les comptes sont affichés : les retards dépendent de la sortie son de neo (test-snd couvre les échantillons).
; Les attentes du son sont bornées à 2 s (horloge 1,1) : si la sortie son n'avance plus, « SON BLOQUE » puis fin (T-120).
; Sortie attendue (console) :
;   OPEN 00 / PRE 80 0100 / START 00 / FILL 06 0400 / DRAIN 83 / STOP 00 / CLOSE 00 / END
; Auteur : bmarty <bmarty@mailo.com>

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
ST    = $3000                ; octets d'état de 3,28
GFX   = $2000                ; tampon dans la RAM graphique : moitié 0 en $2000, moitié 1 en $2080
HALF  = 128
next  = $F6                  ; prochaine moitié à recharger
fills = $F7                  ; recharges faites
total = $F8                  ; octets lus (16 bits)
target = $FA                 ; adresse de la moitié en RAM graphique (16 bits)
t0    = $FC                 ; départ de l'attente en cours (centièmes, 16 bits)

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar

  ldx #<sopen               ; OPEN : 3,4 canal 0
  ldy #>sopen
  jsr print
  stz API_PARAMETERS
  lda #<fname
  sta API_PARAMETERS+1
  lda #>fname
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<spre                ; PRE : les deux moitiés remplies avant le départ (256 octets)
  ldy #>spre
  jsr print
  lda #<GFX
  sta target
  lda #>GFX
  sta target+1
  lda #<(2*HALF)
  ldx #>(2*HALF)
  jsr bgread
  lda ST
  jsr hex
  lda #' '
  jsr wchar
  lda ST+2
  sta total+1
  jsr hex
  lda ST+1
  sta total
  jsr hex
  jsr cr

  ldx #<sstart              ; START : 8,17, deux moitiés de 128 en $2000, 8000 Hz, volume 50
  ldy #>sstart
  jsr print
  lda #<GFX
  sta API_PARAMETERS
  lda #>GFX
  sta API_PARAMETERS+1
  lda #HALF
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<8000
  sta API_PARAMETERS+4
  lda #>8000
  sta API_PARAMETERS+5
  lda #50
  sta API_PARAMETERS+6
  lda #2
  sta API_PARAMETERS+7
  lda #17
  ldx #8
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  stz next                  ; la boucle de lecture : attendre la moitié 'next', la recharger, la déclarer pleine
  stz fills
  jsr now
loop:
  jsr trop
  lda #13
  ldx #8
  jsr api
  lda API_PARAMETERS
  ldx next
  and mask,x
  beq loop
  lda #<GFX                 ; target = GFX + next * HALF
  sta target
  lda #>GFX
  sta target+1
  lda next
  beq l0
  lda #<(GFX+HALF)
  sta target
  lda #>(GFX+HALF)
  sta target+1
l0:
  lda #<HALF
  ldx #>HALF
  jsr bgread
  lda ST+1                  ; rien lu : fin du fichier
  ora ST+2
  beq done
  clc
  lda total
  adc ST+1
  sta total
  lda total+1
  adc ST+2
  sta total+1
  lda next
  sta API_PARAMETERS
  lda #14
  ldx #8
  jsr api
  inc fills
  lda next
  eor #1
  sta next
  jsr now
  bra loop
done:
  ldx #<sfill               ; FILL : recharges, octets lus en tout
  ldy #>sfill
  jsr print
  lda fills
  jsr hex
  lda #' '
  jsr wchar
  lda total+1
  jsr hex
  lda total
  jsr hex
  jsr cr

  ldx #<sdrain              ; DRAIN : le flux joue la fin puis rend les deux moitiés -> 83
  ldy #>sdrain
  jsr print
  jsr now
dw:
  jsr trop
  lda #13
  ldx #8
  jsr api
  lda API_PARAMETERS
  cmp #$83
  bne dw
  jsr hex
  jsr cr

  ldx #<sstop               ; STOP : 8,12 puis 8,13 -> 00
  ldy #>sstop
  jsr print
  lda #12
  ldx #8
  jsr api
  lda #13
  ldx #8
  jsr api
  lda API_PARAMETERS
  jsr hex
  jsr cr

  ldx #<sclose              ; CLOSE : 3,5
  ldy #>sclose
  jsr print
  stz API_PARAMETERS
  lda #5
  ldx #3
  jsr api
  lda API_ERROR
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

; --- départ d'une attente du son : t0 = horloge (1,1, centièmes)
now:
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta t0
  lda API_PARAMETERS+1
  sta t0+1
  rts

; --- plus de 2 s depuis t0 : « SON BLOQUE », arrêt du son, fin du test
trop:
  lda #1
  ldx #1
  jsr api
  sec
  lda API_PARAMETERS
  sbc t0
  tax
  lda API_PARAMETERS+1
  sbc t0+1
  bne bloque
  cpx #200
  bcs bloque
  rts
bloque:
  pla                       ; on ne revient pas
  pla
  ldx #<sbloque
  ldy #>sbloque
  jsr print
  lda #12
  ldx #8
  jsr api
  jmp halt

; --- 3,28 de A/X octets, canal 0, page $90, adresse 'target', état ST ; attend le bit 7
bgread:
  sta API_PARAMETERS+4
  stx API_PARAMETERS+5
  stz API_PARAMETERS
  lda #$90
  sta API_PARAMETERS+1
  lda target
  sta API_PARAMETERS+2
  lda target+1
  sta API_PARAMETERS+3
  lda #<ST
  sta API_PARAMETERS+6
  lda #>ST
  sta API_PARAMETERS+7
  lda #28
  ldx #3
  jsr api
w7:
  bit ST                    ; attente de la fin, sans appel d'API
  bpl w7
  rts

mask:    .byte 1, 2

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

sopen:   .text "OPEN ", 0
spre:    .text "PRE ", 0
sstart:  .text "START ", 0
sfill:   .text "FILL ", 0
sdrain:  .text "DRAIN ", 0
sstop:   .text "STOP ", 0
sclose:  .text "CLOSE ", 0
sbloque: .text "SON BLOQUE", 0
sfin:    .text "END", 0
fname:   .ptext "readpaged.bin"
