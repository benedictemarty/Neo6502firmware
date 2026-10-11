; ***************************************************************************************
; ***************************************************************************************
;
;      Name :      kernel.asm
;      Authors :   Paul Robson (paul@robsons.org.uk)
;      Date :      23rd November 2023
;      Reviewed :  No
;      Purpose :   Monitor main program
;
; ***************************************************************************************
; ***************************************************************************************

	* = $fc00

; ***************************************************************************************
;
;							Access the message control port
;
; ***************************************************************************************

	ControlPort = $FF00

	DCommand = ControlPort+0
	DFunction = ControlPort+1
	DError = ControlPort+2
	DStatus = ControlPort+3
	DParameters = ControlPort+4
	DTopOfStack = ControlPort+12

	Test = 1

start
	cld 									; set up
	sei
	ldx 	#$ff
	txs

	jsr 	KSendMessage  					; beep
	.byte 	8,3
	jsr 	KWaitMessage

	jmp 	KBoot 							; T-36 : boot sequence at $FF80 (no room left below $FF00)

	.include 	"support.asm"
	.include 	"rtos.asm"

	.cerror * > ControlPort, "kernel code overlaps the API control port"

	* = ControlPort
	.word 	0,0,0,0,0,0,0,0

	.include 	"rtos_data.asm"

	.cerror * > $FF80, "scheduler data overlap KBoot"

;
;		T-36 : apply the boot menu choice (1,27), else load NeoDOS (1,3), and start it. Placed in
;		the free gap of page $FF (after the scheduler data, which end at $FF74 ; before the jump
;		table at $FFC1) : the main code filled $FC00-$FEFA, and these 13 bytes pushed it to $FF07,
;		over the API control port — the first API call then overwrote KTaskLock (test rtos).
;
	* = $FF80
KBoot:
	jsr 	KSendMessage  					; apply the boot menu choice (1,27)
	.byte 	1,27
	jsr 	KWaitMessage
	lda 	DParameters 					; 1 : a program is loaded, (0) points to it
	bne 	KStartIt
	jsr 	KSendMessage  					; call "Load BASIC" (NeoDOS)
	.byte 	1,3
	jsr 	KWaitMessage
KStartIt:
	jmp 	(0)								; and start it.

;
;		T-126 / T-127 (requests D-01 / D-02 of NeoWin) : public scheduler vectors. No room below
;		$FF00 ($FEF3-$FEFF free) : placed here, after KBoot, before the jump table.
;
;		KTaskSelf    -> A = current task id (0..RT_MAX_TASKS-1) ; X, Y preserved
;		KTaskStop    stops the tick (1,12 with 0 Hz) and the scheduler (rtActive = 0) ; I=1 on
;		             return. The caller goes on alone, on its own stack ; the other tasks are
;		             abandoned. An IRQ or a BRK resets again, as before KTaskInit. The frame
;		             interrupt (F-10) is not stopped.
;
KTaskSelf:
	lda 	rtCurrent
	rts

KTaskStop:
	sei
	jsr 	KWaitMessage 					; the control port must be free before DParameters
	stz 	rtActive
	stz 	DParameters 					; 1,12 Set Interrupt Tick, 0 Hz : stops it, releases IRQB
	stz 	DParameters+1
	jsr 	KSendMessage
	.byte 	1,12
	jmp 	KWaitMessage
KPageFFEnd:

	.include "build/_vectors.inc"

	.cerror KPageFFEnd > KVectorTable, "code after KBoot overlaps the jump table"
	
	* = $FFFA
	.word 	start 							; NMI
	.word 	start 							; RESET
	.word 	KIrqHandler 					; IRQ/BRK : reset unless the scheduler is active (F-61)
