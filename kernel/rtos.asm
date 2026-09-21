; ***************************************************************************************
; ***************************************************************************************
;
;      Name :      rtos.asm
;      Author :    bmarty <bmarty@mailo.com>
;      Purpose :   Preemptive round robin scheduler for the 65C02 (F-61), driven by the
;                  firmware interrupt tick (API 1,12 — F-60). Lives in the kernel (RAM
;                  $FC00-$FFFF) : code, task control blocks and the IRQ handler.
;
;      Contract :  4 tasks, the $0100 stack split in 4 x 64 bytes (task 0 = the caller
;                  of KTaskInit, $01C0-$01FF ; task n : $01FF - n*64 down), a private
;                  zero page window $E0-$EF saved/restored per task, kernel zero page
;                  $FC-$FF. The API mailbox is not reentrant : wrap API calls between
;                  KTaskLock and KTaskUnlock (the tick is counted but no switch happens
;                  while locked). Semaphores are single bytes anywhere in memory.
;
;      Vectors :   KTaskInit    A/X = tick Hz lo/hi ; installs the IRQ handler, starts
;                               the tick, task 0 = caller. Interrupts enabled on return.
;                  KTaskCreate  A/X = task address lo/hi -> C=0, A = id ; C=1 if no slot
;                  KTaskYield   give the CPU to the next ready task
;                  KTaskSleep   A = ticks to sleep (1-255)
;                  KTaskExit    terminate the current task (never returns)
;                  KTaskLock    no preemption until KTaskUnlock (nestable)
;                  KTaskUnlock
;                  KTaskTicks   -> A/X = tick counter lo/hi
;                  KSemWait     A/X = semaphore address : wait until > 0 then decrement
;                  KSemSignal   A/X = semaphore address : increment
;
; ***************************************************************************************
; ***************************************************************************************

RT_MAX_TASKS = 4
RT_STACK_SIZE = 64
RT_ZP_BASE = $E0 						; private window copied on every switch
RT_ZP_SIZE = 16
RT_ZPK = $FC 							; kernel zero page : $FC/$FD pointer, $FE/$FF scratch

RT_FREE = 0 							; task states
RT_READY = 1
RT_SLEEP = 2
RT_SEM = 3

RT_TCB_SIZE = 24 						; state, sp, sleep lo/hi, sem lo/hi, zp[16], pad
RT_T_STATE = 0
RT_T_SP = 1
RT_T_SLEEP = 2
RT_T_SEM = 4
RT_T_ZP = 6

; Kernel data (rtActive, rtCurrent, rtLock, rtTicks, rtTCB) : see rtos_data.asm ($FF10).

WAI = $CB 								; 64tass runs in Rockwell mode : WDC opcode as a byte

; ***************************************************************************************
;
;								 IRQ entry (vector $FFFE)
;
; ***************************************************************************************

KIrqHandler:
		bit 	rtActive 				; scheduler not started : an IRQ/BRK resets, as before.
		bmi 	rtLIrqActive
		jmp 	start
rtLIrqActive:
		pha
		phx
		phy
		jsr 	rtTick 					; count the tick, wake the sleepers
		lda 	rtLock 					; locked : no switch, just return
		bne 	rtLIrqReturn
		jmp 	KSchedule 				; switch (frame A,X,Y,P,PC is on the stack)
rtLIrqReturn:
		ply
		plx
		pla
		rti

; ***************************************************************************************
;
;		Scheduler : the current task's frame (Y,X,A,P,PC) is on its stack, I is set.
;		Save SP and the zero page window, pick the next ready task, restore, RTI.
;		When nothing is ready : acknowledge the tick (read $FFFF releases IRQB), WAI for
;		the next one, run the tick bookkeeping and look again.
;
; ***************************************************************************************

KSchedule:
		ldx 	rtCurrent 				; save SP of the current task
		jsr 	rtLTcbIndex
		tsx
		txa
		ldx 	RT_ZPK+2
		sta 	rtTCB+RT_T_SP,x
		ldy 	#0 						; save its zero page window
rtLSaveZP:
		lda 	RT_ZP_BASE,y
		sta 	rtTCB+RT_T_ZP,x
		inx
		iny
		cpy 	#RT_ZP_SIZE
		bne 	rtLSaveZP

rtLPick:
		ldy 	rtCurrent 				; round robin from the next task
		ldx 	#RT_MAX_TASKS
rtLPickLoop:
		iny
		cpy 	#RT_MAX_TASKS
		bne 	rtLPickCheck
		ldy 	#0
rtLPickCheck:
		phx
		phy
		tya
		tax
		jsr 	rtLTcbIndex 				; RT_ZPK+2 = TCB offset of candidate y
		ldx 	RT_ZPK+2
		lda 	rtTCB+RT_T_STATE,x
		cmp 	#RT_READY
		beq 	rtLPickFound
		cmp 	#RT_SEM 				; blocked on a semaphore : is it free now ?
		bne 	rtLPickNext
		lda 	rtTCB+RT_T_SEM,x
		sta 	RT_ZPK
		lda 	rtTCB+RT_T_SEM+1,x
		sta 	RT_ZPK+1
		lda 	(RT_ZPK)
		beq 	rtLPickNext
		dec 	a
		sta 	(RT_ZPK) 				; take the semaphore for that task
		lda 	#RT_READY
		sta 	rtTCB+RT_T_STATE,x
rtLPickFound:
		ply
		plx
		sty 	rtCurrent
		bra 	rtLResume
rtLPickNext:
		ply
		plx
		dex
		bne 	rtLPickLoop
		.byte 	WAI 					; nothing ready : wait for a tick (I=1 : resumes without vectoring)
		lda 	$FFFF 					; acknowledge it (reading $FFFF releases IRQB, F-60)
		jsr 	rtTick 					; same bookkeeping as the IRQ entry
		bra 	rtLPick

rtTick: 								; tick counter and sleepers (IRQ entry and idle loop)
		inc 	rtTicks
		bne 	rtLTickSleepers
		inc 	rtTicks+1
rtLTickSleepers:
		ldx 	#0
rtLTickLoop:
		lda 	rtTCB+RT_T_STATE,x
		cmp 	#RT_SLEEP
		bne 	rtLTickNext
		lda 	rtTCB+RT_T_SLEEP,x
		bne 	rtLTickDec
		dec 	rtTCB+RT_T_SLEEP+1,x
rtLTickDec:
		dec 	rtTCB+RT_T_SLEEP,x
		lda 	rtTCB+RT_T_SLEEP,x
		ora 	rtTCB+RT_T_SLEEP+1,x
		bne 	rtLTickNext
		lda 	#RT_READY 				; sleep over
		sta 	rtTCB+RT_T_STATE,x
rtLTickNext:
		txa
		clc
		adc 	#RT_TCB_SIZE
		tax
		cpx 	#RT_MAX_TASKS * RT_TCB_SIZE
		bne 	rtLTickLoop
		rts

rtLResume:
		ldx 	rtCurrent 				; restore the zero page window and SP
		jsr 	rtLTcbIndex
		ldx 	RT_ZPK+2
		ldy 	#0
rtLRestoreZP:
		lda 	rtTCB+RT_T_ZP,x
		sta 	RT_ZP_BASE,y
		inx
		iny
		cpy 	#RT_ZP_SIZE
		bne 	rtLRestoreZP
		ldx 	RT_ZPK+2
		lda 	rtTCB+RT_T_SP,x
		tax
		txs
		ply
		plx
		pla
		rti

rtLTcbIndex: 							; X = task id -> RT_ZPK+2 = id * RT_TCB_SIZE
		txa
		asl 	a 						; * 8
		asl 	a
		asl 	a
		sta 	RT_ZPK+2
		asl 	a 						; * 16
		clc
		adc 	RT_ZPK+2 				; * 24
		sta 	RT_ZPK+2
		rts

; ***************************************************************************************
;
;								 Public entry points
;
; ***************************************************************************************

KTaskInit:
		sei
		pha 							; Hz lo
		phx 							; Hz hi
		ldx 	#RT_MAX_TASKS * RT_TCB_SIZE - 1
		lda 	#0
rtLInitClear:
		sta 	rtTCB,x
		dex
		bpl 	rtLInitClear
		sta 	rtCurrent
		sta 	rtLock
		sta 	rtTicks
		sta 	rtTicks+1
		lda 	#RT_READY 				; task 0 = the caller, running
		sta 	rtTCB+RT_T_STATE
		lda 	#<KIrqHandler
		sta 	$FFFE
		lda 	#>KIrqHandler
		sta 	$FFFF
		lda 	#$80
		sta 	rtActive
		plx
		pla
		sta 	DParameters 			; 1,12 Set Interrupt Tick
		stx 	DParameters+1
		lda 	#12
		sta 	DFunction
		lda 	#1
		sta 	DCommand
		jsr 	KWaitMessage
		cli
		rts

KTaskCreate:
		sta 	RT_ZPK 					; task address
		stx 	RT_ZPK+1
		php
		sei
		ldy 	#1 						; find a free slot (task 0 is the caller)
		ldx 	#RT_TCB_SIZE
rtLCreateFind:
		lda 	rtTCB+RT_T_STATE,x
		beq 	rtLCreateSlot
		txa
		clc
		adc 	#RT_TCB_SIZE
		tax
		iny
		cpy 	#RT_MAX_TASKS
		bne 	rtLCreateFind
		plp
		sec 							; no free slot
		rts
rtLCreateSlot:
		lda 	#RT_READY
		sta 	rtTCB+RT_T_STATE,x
		phx 							; build the initial frame on the task's stack :
		tya 							; top = $FF - id*64 ; push PC hi, PC lo, P, A, X, Y
		asl 	a
		asl 	a
		asl 	a
		asl 	a
		asl 	a
		asl 	a
		eor 	#$FF
		clc
		adc 	#1
		tax 							; X = $100 - id*64 ... i.e. top of that stack + 1
		dex 							; X = top ($FF - id*64)
		lda 	RT_ZPK+1
		sta 	$100,x 					; PC hi
		dex
		lda 	RT_ZPK
		sta 	$100,x 					; PC lo
		dex
		lda 	#$20 					; P : I=0, no decimal
		sta 	$100,x
		dex
		lda 	#0
		sta 	$100,x 					; A
		dex
		sta 	$100,x 					; X
		dex
		sta 	$100,x 					; Y
		dex
		txa
		plx
		sta 	rtTCB+RT_T_SP,x 		; saved SP = below the frame
		tya 							; A = id
		plp
		clc
		rts

KTaskYield: 							; turn the JSR return into an RTI frame and schedule
		php
		sei
		pha
		phx
		phy
		tsx 							; stack : Y X A P retlo rethi
		lda 	$105,x 					; return address is JSR+2 : RTI needs +1
		clc
		adc 	#1
		sta 	$105,x
		bcc 	rtLYieldGo
		inc 	$106,x
rtLYieldGo:
		jmp 	KSchedule

KTaskSleep:
		php
		sei
		tay 							; A = ticks (rtLTcbIndex clobbers A)
		ldx 	rtCurrent
		jsr 	rtLTcbIndex
		ldx 	RT_ZPK+2
		tya
		sta 	rtTCB+RT_T_SLEEP,x
		stz 	rtTCB+RT_T_SLEEP+1,x
		lda 	#RT_SLEEP
		sta 	rtTCB+RT_T_STATE,x
		plp
		jmp 	KTaskYield

KTaskExit:
		sei
		ldx 	rtCurrent
		jsr 	rtLTcbIndex
		ldx 	RT_ZPK+2
		stz 	rtTCB+RT_T_STATE,x 		; free the slot
		lda 	#>rtLExitLoop 			; fake frame so that KSchedule can save this task
		pha
		lda 	#<rtLExitLoop
		pha
		php
		pha
		phx
		phy
		jmp 	KSchedule
rtLExitLoop:
		bra 	rtLExitLoop 				; never resumed (state = free)

KTaskLock:
		inc 	rtLock
		rts

KTaskUnlock:
		dec 	rtLock
		rts

KTaskTicks:
		lda 	rtTicks
		ldx 	rtTicks+1
		rts

KSemWait:
		sta 	RT_ZPK
		stx 	RT_ZPK+1
		php
		sei
		lda 	(RT_ZPK)
		beq 	rtLSemBlock
		dec 	a
		sta 	(RT_ZPK)
		plp
		rts
rtLSemBlock:
		ldx 	rtCurrent 				; block on this semaphore, the scheduler will take it
		jsr 	rtLTcbIndex
		ldx 	RT_ZPK+2
		lda 	RT_ZPK
		sta 	rtTCB+RT_T_SEM,x
		lda 	RT_ZPK+1
		sta 	rtTCB+RT_T_SEM+1,x
		lda 	#RT_SEM
		sta 	rtTCB+RT_T_STATE,x
		plp
		jmp 	KTaskYield

KSemSignal:
		sta 	RT_ZPK
		stx 	RT_ZPK+1
		php
		sei
		lda 	(RT_ZPK)
		inc 	a
		sta 	(RT_ZPK)
		plp
		rts
