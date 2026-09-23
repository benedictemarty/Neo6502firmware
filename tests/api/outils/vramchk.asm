; vramchk.asm — T-55 : la mémoire vidéo est-elle corrompue, ou le signal ?
; Les traits rouges apparaissent pendant les accès disque, sans qu'aucun compteur matériel ne
; bouge (5,40 5,41 tous à 0). Deux explications restent : la VRAM contient vraiment des octets
; parasites, ou l'image est bonne et c'est l'encodage/la transmission qui fautent.
; Cet outil tranche : il efface l'écran, provoque des accès disque en rafale (catalogue, comme
; la complétion de NeoDOS), puis relit toute la moitié basse de la VRAM. Si un octet n'est plus
; nul, la mémoire est corrompue et son offset est affiché — sinon la mémoire est saine.
; Le texte reste dans la moitié haute pour ne pas polluer la zone scrutée.
; Échap : sortie.
; Auteur : bmarty <bmarty@mailo.com>

* = $800

API_COMMAND    = $FF00
API_FUNCTION   = $FF01
API_ERROR      = $FF02
API_PARAMETERS = $FF04

ptr    = $F0
buf    = $900                 ; 256 octets relus de la VRAM
addr   = $A00                 ; offset courant dans la VRAM (24 bits)
passes = $A04
dname  = $A10                 ; nom du répertoire courant, préfixé longueur

start:
  stz dname                   ; chaîne vide = répertoire courant (3,17)
  lda #12                     ; CLS : toute la VRAM à 0 (papier 0)
  jsr wchar
  stz API_PARAMETERS          ; 2,19 APRÈS le CLS, qui remet le curseur visible :
  lda #19                     ; un curseur qui descendrait salirait la zone scrutée
  ldx #2
  jsr api
  ldx #<shead
  ldy #>shead
  jsr print
  jsr cr
  stz passes

round:
  jsr disk                    ; rafale d'accès disque
  jsr scan                    ; scan de la moitié basse ; C = 1 si octet parasite
  bcc next
  jsr home2
  ldx #<sbad                  ; trouvé : offset et valeur
  ldy #>sbad
  jsr print
  lda addr+2
  jsr hex
  lda addr+1
  jsr hex
  lda addr
  jsr hex
  lda #' '
  jsr wchar
  lda #'='
  jsr wchar
  lda buf                     ; la valeur fautive a été recopiée en buf
  jsr hex
  jsr cr
  rts
next:
  inc passes
  jsr home2                   ; toujours la même ligne : le texte ne doit jamais défiler
  ldx #<sok
  ldy #>sok
  jsr print
  lda passes
  jsr hex
  jsr cr
  lda #1                      ; 2,1 : Échap sort
  ldx #2
  jsr api
  lda API_PARAMETERS
  cmp #27
  beq bye
  jmp round
bye:
  rts

; disk — un catalogue complet du répertoire courant (3,17 / 3,18 / 3,19), répété 8 fois :
; c'est exactement ce que fait la complétion de NeoDOS sur Tab.
disk:
  lda #8
  sta passes+1
d1:
  lda #<dname                 ; 3,17 Open Directory, nom vide = répertoire courant
  sta API_PARAMETERS
  lda #>dname
  sta API_PARAMETERS+1
  lda #17
  ldx #3
  jsr api
d2:
  lda #<buf                   ; 3,18 Read Directory dans buf (écrasé ensuite par le scan)
  sta API_PARAMETERS
  lda #>buf
  sta API_PARAMETERS+1
  lda #18
  ldx #3
  jsr api
  lda API_ERROR
  bne d3
  lda buf                     ; nom vide = fin du catalogue
  bne d2
d3:
  lda #19                     ; 3,19 Close Directory
  ldx #3
  jsr api
  dec passes+1
  bne d1
  rts

; scan — parcourt la VRAM de $9600 (ligne 120) à $12BFF par blocs de 256 via 12,2.
; C = 1 et addr = offset du bloc fautif, buf = le bloc.
scan:
  stz addr
  lda #$96                    ; 38400 = $9600
  sta addr+1
  stz addr+2
s1:
  lda #$80                    ; 12,2 : page $80 (VRAM) + offset 24 bits -> $00:buf
  sta API_PARAMETERS
  lda addr
  sta API_PARAMETERS+1
  lda addr+1
  sta API_PARAMETERS+2
  lda addr+2
  sta API_PARAMETERS+3
  lda #<buf
  sta API_PARAMETERS+4
  lda #>buf
  sta API_PARAMETERS+5
  stz API_PARAMETERS+6
  lda #1                      ; 256 octets
  sta API_PARAMETERS+7
  lda #2
  ldx #12
  jsr api
  ldy #0
s2:
  lda buf,y
  bne s3                      ; octet parasite
  iny
  bne s2
  inc addr+1                  ; bloc suivant
  bne s4
  inc addr+2
s4:
  lda addr+2                  ; fin à $012C00 (76800)
  cmp #1
  bne s1
  lda addr+1
  cmp #$2C
  bne s1
  clc                         ; rien trouvé
  rts
s3:
  sty ptr                     ; offset exact dans le bloc
  lda addr
  clc
  adc ptr
  sta addr
  bcc s5
  inc addr+1
s5:
  lda buf,y                   ; la valeur en tête de buf pour l'affichage
  sta buf
  sec
  rts

; home2 — curseur en (0,2) : le compte-rendu reste sur une seule ligne, loin de la zone scrutée
home2:
  stz API_PARAMETERS
  lda #2
  sta API_PARAMETERS+1
  lda #7
  ldx #2
  jmp api

api:
  sta API_FUNCTION
  stx API_COMMAND
wait:
  lda API_COMMAND
  bne wait
  rts

cr:
  lda #13
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

shead:  .text "VRAM CHECK (Echap = sortie)", 0
sok:    .text "VRAM saine, passe ", 0
sbad:   .text "CORROMPUE offset ", 0
