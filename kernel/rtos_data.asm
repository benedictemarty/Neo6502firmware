; ***************************************************************************************
;
;      Name :      rtos_data.asm
;      Author :    bmarty <bmarty@mailo.com>
;      Purpose :   Scheduler data (F-61), in the free RAM between the control port
;                  ($FF00-$FF0F) and the vector table. The kernel image is RAM.
;
; ***************************************************************************************

	* = $FF10

rtActive: 		.byte 0 				; $80 once KTaskInit has run
rtCurrent: 		.byte 0 				; current task id
rtLock: 		.byte 0 				; preemption lock depth
rtTicks: 		.word 0 				; tick counter
rtTCB: 			.fill RT_MAX_TASKS * RT_TCB_SIZE, 0
