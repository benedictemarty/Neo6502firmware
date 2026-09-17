; banks.asm — banques mémoire 6502 (F-23 du fork firmware) : 1,18 Select Bank, 1,19 Get Bank Info.
; 2 banques de 8 Ko (BANK_COUNT, limité par la SRAM du RP2040) hors RAM 6502, commutées par copie dans une fenêtre alignée sur une page.
; Sortie attendue (console) :
;   INFO FF 0000 02 2000 / SEL0 00 / SEL1 00 R 00 / SEL0@6000 00 R 41 5A / INFO 00 6000 02 2000
;   SEL1 00 R 42 / BAD2 01 / ALIGN 01 / HIGH 01 / SEL1@DF00 00 / UNMAP 00 / INFO FF 0000 02 2000 / END
; SPDX-License-Identifier: EUPL-1.2
; Auteur : bmarty <bmarty@mailo.com>
;   64tass --mw65c02 --nostart --output=banks.neo6502 banks.asm

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr   = $F0

start:
  lda #12                   ; CLS (un défilement de la console redessine tout l'écran)
  jsr wchar
  jsr info                  ; INFO FF 0000 04 2000

  ldx #<ssel0
  ldy #>ssel0
  jsr print
  lda #0                    ; banque 0 en $4000
  ldx #$40
  jsr select
  jsr cr
  lda #'A'                  ; marque la banque 0
  sta $4000
  lda #'Z'
  sta $5FFF

  ldx #<ssel1
  ldy #>ssel1
  jsr print
  lda #1                    ; banque 1 en $4000 : vierge (00)
  ldx #$40
  jsr select
  ldx #<sr
  ldy #>sr
  jsr print
  lda $4000
  jsr hex
  jsr cr
  lda #'B'
  sta $4000

  ldx #<ssel06
  ldy #>ssel06
  jsr print
  lda #0                    ; banque 0 en $6000 : retrouve A ... Z
  ldx #$60
  jsr select
  ldx #<sr
  ldy #>sr
  jsr print
  lda $6000
  jsr hex
  lda #' '
  jsr wchar
  lda $7FFF
  jsr hex
  jsr cr

  jsr info                  ; INFO 00 6000 04 2000

  ldx #<ssel1
  ldy #>ssel1
  jsr print
  lda #1                    ; banque 1 en $4000 : retrouve B (write-back)
  ldx #$40
  jsr select
  ldx #<sr
  ldy #>sr
  jsr print
  lda $4000
  jsr hex
  jsr cr

  ldx #<sbad4
  ldy #>sbad4
  jsr print
  lda #2                    ; banque inconnue (BANK_COUNT = 2)
  ldx #$40
  jsr select
  jsr cr

  ldx #<salign
  ldy #>salign
  jsr print
  lda #1                    ; adresse non alignée ($4080)
  sta API_PARAMETERS
  lda #$80
  sta API_PARAMETERS+1
  lda #$40
  sta API_PARAMETERS+2
  jsr call18
  jsr cr

  ldx #<shigh
  ldy #>shigh
  jsr print
  lda #1                    ; $E100 + $2000 > $FF00
  ldx #$E1
  jsr select
  jsr cr

  ldx #<ssel2
  ldy #>ssel2
  jsr print
  lda #1                    ; $DF00 + $2000 = $FF00 : dernière fenêtre possible
  ldx #$DF
  jsr select
  jsr cr

  ldx #<sunmap
  ldy #>sunmap
  jsr print
  lda #$FF                  ; démonte
  ldx #0
  jsr select
  jsr cr

  jsr info                  ; INFO FF 0000 04 2000

  ldx #<sfin
  ldy #>sfin
  jsr print
halt:
  jmp halt

; --- 1,18 : A = banque, X = octet haut de l'adresse ; affiche l'erreur
select:
  sta API_PARAMETERS
  stz API_PARAMETERS+1
  stx API_PARAMETERS+2
call18:
  lda #18
  ldx #1
  jsr api
  lda API_ERROR
  jmp hex

; --- 1,19 : "INFO bb aaaa nn ssss"
info:
  ldx #<sinfo
  ldy #>sinfo
  jsr print
  lda #19
  ldx #1
  jsr api
  lda API_PARAMETERS
  jsr hex
  lda #' '
  jsr wchar
  lda API_PARAMETERS+2
  jsr hex
  lda API_PARAMETERS+1
  jsr hex
  lda #' '
  jsr wchar
  lda API_PARAMETERS+3
  jsr hex
  lda #' '
  jsr wchar
  lda API_PARAMETERS+5
  jsr hex
  lda API_PARAMETERS+4
  jsr hex
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

sinfo:  .text "INFO ", 0
ssel0:  .text "SEL0 ", 0
ssel1:  .text "SEL1 ", 0
ssel06: .text "SEL0@6000 ", 0
ssel2:  .text "SEL1@DF00 ", 0
sr:     .text " R ", 0
sbad4:  .text "BAD2 ", 0
salign: .text "ALIGN ", 0
shigh:  .text "HIGH ", 0
sunmap: .text "UNMAP ", 0
sfin:   .text "END", 13, 0
