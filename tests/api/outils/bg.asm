; bg.asm — T-82 (ADR-0002) : combien de temps de lecture la 3,28 rend au 6502, sur la carte.
; Lit sdbench.dat (256 Ko, à la racine de la clé) deux fois par tranches de 4 Ko en $4000 :
;   A. par 3,27 (le 6502 attend)                                   -> durée TA (centièmes)
;   B. par 3,28, le 6502 incrémente un compteur 24 bits en attendant -> durée TB, compteur NB
;   C. la même boucle à vide pendant 2^19 tours                    -> durée TC
; Part rendue au 6502 = NB / (2^19 / TC × TB). Résultats affichés et rangés en $0A00 :
;   TA (2 o), TB (2 o), NB (3 o), TC (2 o), erreurs A/B (1 o chacune).
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
blk   = $F2                 ; tranches restantes
t0    = $F4                 ; horloge au départ (16 bits)
cnt   = $F6                 ; compteur 24 bits ($F6-$F8)
ST    = $3000               ; octets d'état de la 3,28
RES   = $0A00
DEST  = $4000

start:
  lda #12
  jsr wchar
  ldx #<stitle
  ldy #>stitle
  jsr print
  ldx #10
z0:
  stz RES,x
  dex
  bpl z0

  jsr open                  ; A : 3,27
  jsr now
  lda #64
  sta blk
a1:
  jsr setp
  lda #27
  ldx #3
  jsr api
  lda API_ERROR
  ora RES+9
  sta RES+9
  dec blk
  bne a1
  jsr since
  sta RES
  stx RES+1
  jsr close

  jsr open                  ; B : 3,28 + compteur
  stz cnt
  stz cnt+1
  stz cnt+2
  jsr now
  lda #64
  sta blk
b1:
  jsr setp
  lda #<ST
  sta API_PARAMETERS+6
  lda #>ST
  sta API_PARAMETERS+7
  lda #28
  ldx #3
  jsr api
b2:
  bit ST                    ; boucle mesurée : même code en C
  bmi b3
  inc cnt
  bne b2
  inc cnt+1
  bne b2
  inc cnt+2
  bra b2
b3:
  lda ST
  and #$7F
  ora RES+10
  sta RES+10
  dec blk
  bne b1
  jsr since
  sta RES+2
  stx RES+3
  lda cnt
  sta RES+4
  lda cnt+1
  sta RES+5
  lda cnt+2
  sta RES+6
  jsr close

  stz ST                    ; C : la boucle à vide, 2^19 tours (cnt+2 atteint 8)
  stz cnt
  stz cnt+1
  stz cnt+2
  jsr now
c1:
  bit ST
  bmi c3
  inc cnt
  bne c1
  inc cnt+1
  bne c1
  inc cnt+2
  lda cnt+2
  cmp #8
  bne c1
c3:
  jsr since
  sta RES+7
  stx RES+8

  ldx #<sa                  ; affichage : A, B, compteur, C, erreurs
  ldy #>sa
  jsr print
  lda RES+1
  jsr hex
  lda RES
  jsr hex
  ldx #<sb
  ldy #>sb
  jsr print
  lda RES+3
  jsr hex
  lda RES+2
  jsr hex
  ldx #<sn
  ldy #>sn
  jsr print
  lda RES+6
  jsr hex
  lda RES+5
  jsr hex
  lda RES+4
  jsr hex
  ldx #<sc
  ldy #>sc
  jsr print
  lda RES+8
  jsr hex
  lda RES+7
  jsr hex
  ldx #<se
  ldy #>se
  jsr print
  lda RES+9
  jsr hex
  lda #' '
  jsr wchar
  lda RES+10
  jsr hex
  lda #13
  jsr wchar
  rts

; --- 3,4 : sdbench.dat, canal 0
open:
  stz API_PARAMETERS
  lda #<fname
  sta API_PARAMETERS+1
  lda #>fname
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #3
  jmp api

close:
  stz API_PARAMETERS
  lda #5
  ldx #3
  jmp api

; --- canal 0, page 0, $4000, 4096 octets
setp:
  stz API_PARAMETERS
  stz API_PARAMETERS+1
  lda #<DEST
  sta API_PARAMETERS+2
  lda #>DEST
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #$10
  sta API_PARAMETERS+5
  rts

; --- 1,1 : horloge (centièmes) -> t0
now:
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta t0
  lda API_PARAMETERS+1
  sta t0+1
  rts

; --- A/X = horloge - t0
since:
  lda #1
  ldx #1
  jsr api
  sec
  lda API_PARAMETERS
  sbc t0
  pha
  lda API_PARAMETERS+1
  sbc t0+1
  tax
  pla
  rts

api:
  sta API_FUNCTION
  stx API_COMMAND
wait:
  lda API_COMMAND
  bne wait
  rts

wchar:
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
pl:
  lda (ptr),y
  beq pd
  phy
  jsr wchar
  ply
  iny
  bne pl
pd:
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

stitle: .text "BG T-82 : 3,27 puis 3,28 sur sdbench.dat", 13, 0
sa:     .text "A (3,27) cs=", 0
sb:     .text 13, "B (3,28) cs=", 0
sn:     .text " compteur=", 0
sc:     .text 13, "C a vide 2^19 tours cs=", 0
se:     .text 13, "erreurs A B : ", 0
fname:  .ptext "sdbench.dat"
