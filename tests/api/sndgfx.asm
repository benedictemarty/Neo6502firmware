; sndgfx.asm — flux PCM depuis la RAM graphique, 8,17 (Trinity T-119) : paramètres refusés, lecture unique qui
; s'arrête seule, boucle qui ne s'arrête pas, 8,14 refusé, 8,12 qui arrête, 8,17 qui remplace un flux 8,11.
; Le contenu de la RAM graphique est indifférent (seul l'état est affiché) ; test-snd couvre les échantillons.
; Sortie attendue (console) :
;   BAD 01 01 01 01 / ONCE 00 80 / OVER 00 / LOOP 00 80 / FIL 01 / LONG 80 / STOP 00 / OVER11 00 80 01 / END
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=sndgfx.neo6502 sndgfx.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
cnt   = $F2

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar

  ldx #<sbad                ; BAD : longueur 0, cadence 0, volume 101, au-delà de la RAM graphique
  ldy #>sbad
  jsr print
  jsr setok
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  jsr startg
  jsr setok
  stz API_PARAMETERS+4
  stz API_PARAMETERS+5
  jsr startg
  jsr setok
  lda #101
  sta API_PARAMETERS+6
  jsr startg
  jsr setok
  lda #$7F                  ; $7FF0 + 32 > $8000
  sta API_PARAMETERS+1
  lda #$F0
  sta API_PARAMETERS
  jsr startg
  jsr cr

  ldx #<sonce               ; ONCE : 8,17 une fois -> 00, 8,13 -> 80
  ldy #>sonce
  jsr print
  jsr setok
  jsr startg
  jsr status
  jsr cr

  ldx #<sover               ; OVER : 32 octets à 8000 Hz (4 ms) : 8,13 revient à 00 tout seul
  ldy #>sover
  jsr print
  stz cnt
  stz cnt+1
ow:
  lda #13
  ldx #8
  jsr api
  lda API_PARAMETERS
  beq od
  inc cnt
  bne ow
  inc cnt+1
  bne ow
od:
  jsr hex
  jsr cr

  ldx #<sloop               ; LOOP : 8,17 en boucle -> 00, 8,13 -> 80
  ldy #>sloop
  jsr print
  jsr setok
  lda #1
  sta API_PARAMETERS+7
  jsr startg
  jsr status
  jsr cr

  ldx #<sfil                ; FIL : 8,14 refusé -> 01
  ldy #>sfil
  jsr print
  stz API_PARAMETERS
  lda #14
  ldx #8
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr

  ldx #<slong               ; LONG : après la même attente que OVER, la boucle joue toujours -> 80
  ldy #>slong
  jsr print
  stz cnt
  stz cnt+1
lw:
  lda #13
  ldx #8
  jsr api
  inc cnt
  bne lw
  inc cnt+1
  bne lw
  jsr status
  jsr cr

  ldx #<sstop               ; STOP : 8,12 puis 8,13 -> 00
  ldy #>sstop
  jsr print
  lda #12
  ldx #8
  jsr api
  jsr status
  jsr cr

  ldx #<sover11             ; OVER11 : 8,11 joue, 8,17 le remplace -> 00, 80, et 8,14 refusé -> 01
  ldy #>sover11
  jsr print
  lda #<$4000
  sta API_PARAMETERS
  lda #>$4000
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #1
  sta API_PARAMETERS+3
  lda #<22050
  sta API_PARAMETERS+4
  lda #>22050
  sta API_PARAMETERS+5
  lda #50
  sta API_PARAMETERS+6
  lda #11
  ldx #8
  jsr api
  jsr setok
  lda #1
  sta API_PARAMETERS+7
  jsr startg
  jsr status
  lda #' '
  jsr wchar
  stz API_PARAMETERS
  lda #14
  ldx #8
  jsr api
  lda API_ERROR
  jsr hex
  jsr cr
  lda #1                    ; 8,1 : tout arrêter
  ldx #8
  jsr api

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF
.endif
  rts

; --- paramètres valides : offset $1000, 32 octets, 8000 Hz, volume 50, une fois
setok:
  stz API_PARAMETERS
  lda #$10
  sta API_PARAMETERS+1
  lda #32
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<8000
  sta API_PARAMETERS+4
  lda #>8000
  sta API_PARAMETERS+5
  lda #50
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  rts

; --- 8,17 -> affiche "err "
startg:
  lda #17
  ldx #8
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jmp wchar

; --- 8,13 -> affiche P0
status:
  lda #13
  ldx #8
  jsr api
  lda API_PARAMETERS
  jmp hex

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

sbad:    .text "BAD ", 0
sonce:   .text "ONCE ", 0
sover:   .text "OVER ", 0
sloop:   .text "LOOP ", 0
sfil:    .text "FIL ", 0
slong:   .text "LONG ", 0
sstop:   .text "STOP ", 0
sover11: .text "OVER11 ", 0
sfin:    .text "END", 0
