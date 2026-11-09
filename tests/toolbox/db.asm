; db.asm — Toolbox groupe 40 Database (T-138) : create, open, info, append, read, search, update, delete, close, persistance.
; Fichier t.db créé dans le stockage (canal 3), enregistrements de 8 octets : statut, 5 caractères de nom, 2 octets libres.
; Sortie attendue : voir db.expected
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=db.neo6502 db.asm

NEO = 0                     ; 1 : construit pour neo (fin par jmp $FFFF) — run_neo.sh passe -D NEO=1
ptr2   = $F4
logLen = $1FFE               ; journal $2000..

* = $800
  jmp start

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $E00                ; enregistrement lu
cap   = $E40                ; copie du résultat : erreur, puis 8 octets de paramètres

; --- appelle la fonction A du groupe 40 et copie le résultat dans cap
call40:
  ldx #40
  jsr api
  lda API_ERROR
  sta cap
  ldx #7
c40:
  lda API_PARAMETERS,x
  sta cap+1,x
  dex
  bpl c40
  rts

; --- affiche le libellé X/Y puis l'erreur
label:
  jsr print
  lda cap
  jmp hex

; --- espace puis nombre 16 bits cap+1 (octet bas), cap+2 (octet haut) : affiché haut puis bas
sp16:
  lda #' '
  jsr wchar
  lda cap+2
  jsr hex
  lda cap+1
  jmp hex

; --- espace puis 8 octets de buf en hexadécimal
sbuf:
  lda #' '
  jsr wchar
  ldy #0
sb1:
  lda buf,y
  phy
  jsr hex
  ply
  iny
  cpy #8
  bne sb1
  rts

; --- 40,x avec nom en X/Y, canal 3 : p0 canal, p1,2 nom, p3,4 taille
setname:
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  lda #3
  sta API_PARAMETERS
  rts

; --- p0,1 = numéro A
setrec:
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  rts

start:
  stz logLen
  lda #$20
  sta logLen+1
  lda #12
  jsr wchar
  lda #1                    ; 32,1
  ldx #32
  jsr api

  ldx #<fname               ; CREATE : t.db, enregistrements de 8 octets
  ldy #>fname
  jsr setname
  lda #8
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #1
  jsr call40
  ldx #<s_create
  ldy #>s_create
  jsr label
  jsr cr

  ldx #<fname               ; OPEN
  ldy #>fname
  jsr setname
  lda #2
  jsr call40
  ldx #<s_open
  ldy #>s_open
  jsr label
  jsr cr

  lda #4                    ; INFO : 0 enregistrement, taille 8
  jsr call40
  ldx #<s_info
  ldy #>s_info
  jsr label
  jsr sp16
  lda cap+3
  sta cap+1
  lda cap+4
  sta cap+2
  jsr sp16
  jsr cr

  ldx #<r1                  ; APPEND x3 : ALPHA, BRAVO, CHARL
  ldy #>r1
  jsr append
  ldx #<r2
  ldy #>r2
  jsr append
  ldx #<r3
  ldy #>r3
  jsr append
  jsr cr

  lda #4                    ; INFO : 3 enregistrements
  jsr call40
  ldx #<s_info
  ldy #>s_info
  jsr label
  jsr sp16
  jsr cr

  lda #2                    ; READ 2 -> buf
  jsr read
  ldx #<s_read
  ldy #>s_read
  jsr label
  jsr sbuf
  jsr cr

  ldx #<k_char              ; SEARCH "CHAR" à l'offset 1 depuis 1 -> 3
  ldy #>k_char
  lda #1
  jsr search
  ldx #<s_search
  ldy #>s_search
  jsr label
  jsr sp16
  jsr cr

  ldx #<k_zz                ; SEARCH "ZZ" -> introuvable
  ldy #>k_zz
  lda #1
  jsr search
  ldx #<s_search
  ldy #>s_search
  jsr label
  jsr sp16
  jsr cr

  lda #2                    ; UPDATE 2 <- BRAVOXY puis READ
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #<r2b
  sta API_PARAMETERS+2
  lda #>r2b
  sta API_PARAMETERS+3
  lda #6
  jsr call40
  ldx #<s_update
  ldy #>s_update
  jsr label
  lda #2
  jsr read
  jsr sbuf
  jsr cr

  lda #1                    ; DELETE 1 puis SEARCH "ALPH" -> introuvable ; READ 1 : statut 00
  jsr setrec
  lda #8
  jsr call40
  ldx #<s_delete
  ldy #>s_delete
  jsr label
  jsr cr
  ldx #<k_alph
  ldy #>k_alph
  lda #1
  jsr search
  ldx #<s_search
  ldy #>s_search
  jsr label
  jsr cr
  ldx #<s_read
  ldy #>s_read
  jsr print
  lda #1
  jsr read
  jsr sbuf
  jsr cr

  lda #9                    ; READ 9 : hors limites
  jsr read
  ldx #<s_read
  ldy #>s_read
  jsr label
  jsr cr

  lda #3                    ; CLOSE puis CLOSE sans base
  jsr call40
  ldx #<s_close
  ldy #>s_close
  jsr label
  lda #3
  jsr call40
  jsr sp8
  jsr cr

  ldx #<fname               ; REOPEN : les 3 enregistrements sont restés sur le stockage
  ldy #>fname
  jsr setname
  lda #2
  jsr call40
  ldx #<s_reopen
  ldy #>s_reopen
  jsr label
  lda #4
  jsr call40
  jsr sp16
  ldx #<r4                  ; APPEND après réouverture -> 4
  ldy #>r4
  jsr append
  lda #4
  jsr read
  jsr sbuf
  jsr cr

  lda #3
  jsr call40

  ldx #<fnone               ; OPEN d'un fichier absent -> 02
  ldy #>fnone
  jsr setname
  lda #2
  jsr call40
  ldx #<s_missing
  ldy #>s_missing
  jsr label
  ldx #<fbad                ; OPEN d'un fichier qui n'est pas une base -> 01
  ldy #>fbad
  jsr setname
  lda #2
  jsr call40
  jsr sp8
  jsr cr

  ldx #<s_fin
  ldy #>s_fin
  jsr print
halt:
.if NEO
  jmp $FFFF                 ; neo : sortie de l'émulateur (memory.dump)
.endif
  rts

; --- espace puis l'erreur
sp8:
  lda #' '
  jsr wchar
  lda cap
  jmp hex

; --- 40,7 : enregistrement en X/Y -> " err numéro"
append:
  stx API_PARAMETERS
  sty API_PARAMETERS+1
  lda #7
  jsr call40
  ldx #<s_append
  ldy #>s_append
  jsr label
  jmp sp16

; --- 40,5 : enregistrement A -> buf
read:
  jsr setrec
  lda #<buf
  sta API_PARAMETERS+3
  lda #>buf
  sta API_PARAMETERS+4
  stz API_PARAMETERS+2
  lda #5
  jmp call40

; --- 40,9 : motif en X/Y, 4 octets (ou 2 pour ZZ) depuis A, offset 1
search:
  stx API_PARAMETERS+4
  sty API_PARAMETERS+5
  jsr setrec
  lda #1
  sta API_PARAMETERS+2
  lda #4
  sta API_PARAMETERS+3
  lda #9
  jmp call40

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

s_create: .text "CREATE ", 0
s_open:   .text "OPEN ", 0
s_info:   .text "INFO ", 0
s_append: .text " APPEND ", 0
s_read:   .text "READ ", 0
s_search: .text "SEARCH ", 0
s_update: .text "UPDATE ", 0
s_delete: .text "DELETE ", 0
s_close:  .text "CLOSE ", 0
s_reopen: .text "REOPEN ", 0
s_missing:.text "MISSING ", 0
s_fin:    .text "END", 13, 0
fname:    .ptext "t.db"
fnone:    .ptext "none.db"
fbad:     .ptext "test.res"
k_char:   .text "CHAR"
k_zz:     .text "ZZZZ"
k_alph:   .text "ALPH"
r1:       .byte 1
          .text "ALPHA", 0, 0
r2:       .byte 1
          .text "BRAVO", 0, 0
r2b:      .byte 1
          .text "BRAVO", "X", "Y"
r3:       .byte 1
          .text "CHARL", 0, 0
r4:       .byte 1
          .text "DELTA", 0, 0
