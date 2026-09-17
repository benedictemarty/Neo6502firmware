; bankblit.asm — stockage des banques mémoire (F-23) vu comme pages $A0/$A1 du blitter :
; 3,27 charge une banque depuis le disque sans la monter, 12,2 la relit ; le write-back du
; démontage est visible par 12,2. Fichier readpaged.bin (octet i = i & $FF).
; Sortie attendue (console) :
;   OPEN 00 / LOAD 00 0100 / MAP 00 R 00 01 02 03 / UNMAP 00 / BLIT 00 51 01 02 03
;   END 01 / PAGE 01 / MAP 00 R 51 / END
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=bankblit.neo6502 bankblit.asm

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0
buf   = $B00

start:
  lda #12
  jsr wchar
  ldx #<sopen
  ldy #>sopen
  jsr print
  stz API_PARAMETERS        ; 3,4 canal 0
  lda #<fname
  sta API_PARAMETERS+1
  lda #>fname
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #3
  jsr api
  jsr perr
  jsr cr

  ldx #<sload               ; 3,27 : 256 octets -> $A1:$0100 (banque 1, non montée)
  ldy #>sload
  jsr print
  stz API_PARAMETERS
  lda #$A1
  sta API_PARAMETERS+1
  stz API_PARAMETERS+2
  lda #$01
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #$01
  sta API_PARAMETERS+5
  lda #27
  ldx #3
  jsr api
  lda API_ERROR
  jsr hex
  lda #' '
  jsr wchar
  lda API_PARAMETERS+5
  jsr hex
  lda API_PARAMETERS+4
  jsr hex
  jsr cr
  stz API_PARAMETERS        ; 3,5 close
  lda #5
  ldx #3
  jsr api

  ldx #<smap                ; 1,18 banque 1 en $4000 : $4100.. = 00 01 02 03
  ldy #>smap
  jsr print
  lda #1
  jsr select
  ldx #<sr
  ldy #>sr
  jsr print
  ldx #0
mloop:
  lda $4100,x
  phx
  jsr hex
  lda #' '
  jsr wchar
  plx
  inx
  cpx #4
  bne mloop
  jsr cr
  lda #'Q'                  ; modifie la fenêtre
  sta $4100

  ldx #<sunmap              ; démonte : write-back vers le stockage
  ldy #>sunmap
  jsr print
  lda #$FF
  jsr select
  jsr cr

  ldx #<sblit               ; 12,2 : $A1:$0100 (4 octets) -> $B00 : 51 01 02 03
  ldy #>sblit
  jsr print
  lda #$A1
  ldx #<$0100
  ldy #>$0100
  lda #$A1
  jsr blit4
  jsr cr

  ldx #<send                ; 12,2 : $A0:$1FFE, 4 octets : dépasse la banque -> erreur
  ldy #>send
  jsr print
  lda #$A0
  ldx #<$1FFE
  ldy #>$1FFE
  jsr blit4err
  jsr cr

  ldx #<spage               ; 12,2 : page $A2 inconnue -> erreur
  ldy #>spage
  jsr print
  lda #$A2
  ldx #0
  ldy #0
  jsr blit4err
  jsr cr

  ldx #<smap                ; remonte la banque 1 : $4100 = 51
  ldy #>smap
  jsr print
  lda #1
  jsr select
  ldx #<sr
  ldy #>sr
  jsr print
  lda $4100
  jsr hex
  jsr cr

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
  jmp halt

; --- 1,18 : A = banque, fenêtre $4000 ; affiche l'erreur
select:
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  lda #$40
  sta API_PARAMETERS+2
  lda #18
  ldx #1
  jsr api
  lda API_ERROR
  jmp hex

; --- 12,2 : 4 octets de A:X/Y -> $B00 ; affiche erreur puis les 4 octets
blit4:
  jsr blit4err
  lda #' '
  jsr wchar
  ldx #0
b4:
  lda buf,x
  phx
  jsr hex
  lda #' '
  jsr wchar
  plx
  inx
  cpx #4
  bne b4
  rts
blit4err:
  sta API_PARAMETERS
  stx API_PARAMETERS+1
  sty API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #<buf
  sta API_PARAMETERS+4
  lda #>buf
  sta API_PARAMETERS+5
  lda #4
  sta API_PARAMETERS+6
  stz API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
perr:
  lda API_ERROR
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

sopen:  .text "OPEN ", 0
sload:  .text "LOAD ", 0
smap:   .text "MAP ", 0
sr:     .text " R ", 0
sunmap: .text "UNMAP ", 0
sblit:  .text "BLIT ", 0
send:   .text "END ", 0
spage:  .text "PAGE ", 0
sfin:   .text "END", 13, 0
fname:  .ptext "readpaged.bin"
