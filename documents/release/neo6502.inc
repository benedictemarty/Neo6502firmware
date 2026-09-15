ControlPort = $ff00
SemSignal = $ffc1
SemWait = $ffc4
TaskTicks = $ffc7
TaskUnlock = $ffca
TaskLock = $ffcd
TaskExit = $ffd0
TaskSleep = $ffd3
TaskYield = $ffd6
TaskCreate = $ffd9
TaskInit = $ffdc
ChainBasicOnlyProgram = $ffdf
ChainBasicProgram = $ffe2
WriteCharacterInline = $ffe5
LoadExtended = $ffe8
ReadLine = $ffeb
ReadCharacter = $ffee
WriteCharacter = $fff1
WaitMessage = $fff4
SendMessage = $fff7


DoSendMessage .macro
	jsr 	SendMessage
	.endm

DoWaitMessage .macro
	jsr 	WaitMessage
	.endm
