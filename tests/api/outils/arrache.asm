; arrache.asm — T-34 : clé arrachée pendant une lecture, sur la carte.
; Lit sdbench.dat en boucle (3,4 ouverture, 3,27 par tranches de 4 Ko, 3,5 fermeture) pendant 60 s au plus,
; et s'arrête à la première erreur. L'écran affiche un point par fichier lu en entier.
; Résultats en $0A00 : mot 0 = tranches lues sans erreur, octet 2 = erreur de l'appel fautif (API_ERROR),
; octet 3 = appel fautif (4 ouverture, 27 lecture, 0 aucun : 60 s écoulées), mot 4 = durée (centièmes),
; octet 6 = $A5 quand le programme a fini (il rend la main à NeoDOS par RTS).
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

t0    = $F4
blk   = $F6
RES   = $0A00
DEST  = $4000

start:
  stz RES
  stz RES+1
  stz RES+2
  stz RES+3
  stz RES+6
  jsr now
floop:
  jsr since                 ; 60 s écoulées : fin sans erreur
  cpx #>6000
  bcc go
  bne fini
  cmp #<6000
  bcs fini
go:
  stz API_PARAMETERS        ; 3,4 ouverture, canal 0, lecture
  lda #<fname
  sta API_PARAMETERS+1
  lda #>fname
  sta API_PARAMETERS+2
  stz API_PARAMETERS+3
  lda #4
  ldx #3
  jsr api
  lda API_ERROR
  beq lu
  ldx #4
  bra erreur
lu:
  lda #64
  sta blk
r1:
  stz API_PARAMETERS        ; 3,27 : 4 Ko vers DEST
  stz API_PARAMETERS+1
  lda #<DEST
  sta API_PARAMETERS+2
  lda #>DEST
  sta API_PARAMETERS+3
  stz API_PARAMETERS+4
  lda #$10
  sta API_PARAMETERS+5
  lda #27
  ldx #3
  jsr api
  lda API_ERROR
  beq ok
  ldx #27
  bra erreur
ok:
  inc RES
  bne r2
  inc RES+1
r2:
  dec blk
  bne r1
  jsr close
  lda #'.'
  jsr wchar
  bra floop

erreur:
  sta RES+2
  stx RES+3
  jsr close
fini:
  jsr since
  sta RES+4
  stx RES+5
  lda #$A5
  sta RES+6
  rts

close:
  stz API_PARAMETERS
  lda #5
  ldx #3
  jmp api

now:
  lda #1
  ldx #1
  jsr api
  lda API_PARAMETERS
  sta t0
  lda API_PARAMETERS+1
  sta t0+1
  rts
since:                      ; A/X = horloge - t0 (centièmes)
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

fname:  .ptext "sdbench.dat"
