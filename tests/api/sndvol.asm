; sndvol.asm — 8,9 Set Channel Volume / 8,10 Get Channel Volume (F-12 du fork firmware) :
; volume instantane ou en rampe de la note en cours, sans redemarrer l'onde.
; Sequence : 8,7 note canal 0 (440 Hz, 3 s, type 0, volume 100) ; 8,10 -> 100 ($64) ;
; 8,9 volume 50 immediat -> 8,10 = $32 ; 8,9 volume 0 en 1 s ; apres 0,3 s : 8,10 entre 1 et 49
; (rampe en cours, valeur affichee) ; apres 1,5 s : 8,10 = 0 ; 8,9 canal 9 -> erreur ;
; 8,9 canal 1 (silencieux) -> erreur.
; Sortie attendue (console) : "PLAY 00 VOL 00 64 SET 00 32 RAMP 00 MID 00 ?? END 00 00 BAD 01 SIL 01 END"
; (?? = valeur intermediaire, verifiee 01..31 par le script de test).
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=sndvol.neo6502 sndvol.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
t0    = $F2

start:
  stz logLen                ; journal vide ($2000)
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  ldx #<splay
  ldy #>splay
  jsr print
  stz API_PARAMETERS        ; 8,7 : canal 0
  lda #<440
  sta API_PARAMETERS+1
  lda #>440
  sta API_PARAMETERS+2
  lda #<300                 ; 3 s
  sta API_PARAMETERS+3
  lda #>300
  sta API_PARAMETERS+4
  stz API_PARAMETERS+5      ; glissando 0
  stz API_PARAMETERS+6
  stz API_PARAMETERS+7      ; type 0 (carre) ; volume dans le parametre 8 = $FF0C
  lda #100
  sta API_PARAMETERS+8
  lda #7
  ldx #8
  jsr api
  jsr perr
  lda #' '
  jsr wchar
  ldx #<svol
  ldy #>svol
  jsr print
  jsr getvol
  ldx #<sset
  ldy #>sset
  jsr print
  stz API_PARAMETERS        ; 8,9 canal 0, volume 50, immediat
  lda #50
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  jsr setvol
  jsr getvol
  ldx #<sramp
  ldy #>sramp
  jsr print
  stz API_PARAMETERS        ; 8,9 canal 0, volume 0 en 100 cs (apres print : P0 = dernier caractere)
  stz API_PARAMETERS+1
  lda #100
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  jsr setvol
  jsr perr
  lda #' '
  jsr wchar
  lda #30
  jsr waitcs
  ldx #<smid
  ldy #>smid
  jsr print
  jsr getvol
  lda #150
  jsr waitcs
  ldx #<send0
  ldy #>send0
  jsr print
  jsr getvol
  ldx #<sbad
  ldy #>sbad
  jsr print
  lda #9                    ; canal inexistant
  sta API_PARAMETERS
  lda #50
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  stz API_PARAMETERS+3
  jsr setvol
  jsr perr
  lda #' '
  jsr wchar
  ldx #<ssil
  ldy #>ssil
  jsr print
  lda #1                    ; canal 1 : rien ne joue
  sta API_PARAMETERS
  jsr setvol
  jsr perr
  lda #' '
  jsr wchar
  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts                       ; retour à l'appelant (sys du BASIC, ou NeoDOS pour un .NEO) ; reset sinon

setvol:
  lda #9
  ldx #8
  jmp api

; 8,10 canal 0 -> "ee vv " (erreur, volume)
getvol:
  stz API_PARAMETERS
  lda #10
  ldx #8
  jsr api
  jsr perr
  lda #' '
  jsr wchar
  lda API_PARAMETERS+1
  jsr hex
  lda #' '
  jmp wchar

; attend A centiemes de seconde (timer 100 Hz, 1,1 ; A < 128)
waitcs:
  sta t0+1
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta t0
wl:
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  sec
  sbc t0
  cmp t0+1
  bcc wl
  rts

perr:
  lda API_ERROR
  jmp hex

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

splay: .text "PLAY ", 0
svol:  .text "VOL ", 0
sset:  .text "SET ", 0
sramp: .text "RAMP ", 0
smid:  .text "MID ", 0
send0: .text "END ", 0
sbad:  .text "BAD ", 0
ssil:  .text "SIL ", 0
sfin:  .text "END", 13, 0
