// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      debugport.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      24th September 2026
//      Purpose :   T-73 : a debug channel on UART0, for faults the screen cannot report.
//
//      Why not the USB-C socket (bmarty's first idea) : the RP2040 has a single USB
//      controller, already CFG_TUSB_RHPORT0_MODE = OPT_MODE_HOST for the keyboard, the hub
//      and the key, and the programming socket IS that controller. Host and CDC device are
//      exclusive, and switching would cost the very devices whose activity we are chasing.
//      UART0 (GPIO 28/29, on the UEXT) is free of all that.
//
//      The rule of the project applies here too : nothing slow on the path that serves the
//      6502 bus. DBGWrite only appends to a RAM ring — no register access, no waiting — and
//      DBGFlush pushes at most what the hardware FIFO accepts, from DSPSync. A trace that
//      stalled the bus would change the very timing it is meant to measure.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include <unistd.h>
#include "interface/usbsettle.h"  															// sbrk, for the heap report

#define DBG_UART 		uart0
#define DBG_TX_PIN 		(28)  													// As serial.cpp : UEXT
#define DBG_RX_PIN 		(29)
#define DBG_BAUD 		(115200)  												// Standard, and the 95 Hz flush caps us at ~3 KB/s anyway
#define DBG_RING 		(2048)  												// Power of two ; a DIR is ~1,5 KB

static char ring[DBG_RING];
static volatile uint16_t head = 0,tail = 0;  									// head = write, tail = read
static bool dbgOn = false;
volatile uint32_t stoSectorCount = 0;  											// T-73 : sectors moved (storage drivers add to it)

//		Started once, in P0. If the 6502 later reprograms the port (group 10) the trace
//		follows its baud rate and becomes unreadable — that is the accepted price of
//		sharing the only free UART, and it only matters to a program that uses the port.

void DBGInitialise(void) {
	uart_init(DBG_UART,DBG_BAUD);
	gpio_set_function(DBG_TX_PIN,GPIO_FUNC_UART);
	gpio_set_function(DBG_RX_PIN,GPIO_FUNC_UART);
	uart_set_hw_flow(DBG_UART,false,false);
	uart_set_fifo_enabled(DBG_UART,true);  										// 32 bytes of hardware buffer
	uart_set_format(DBG_UART,8,1,UART_PARITY_NONE);
	dbgOn = true;
	DBGWrite("\r\n-- Trinity debug port (T-73), built " __DATE__ " --\r\n");
}

//		The video mode sets clk_sys (252 or 270 MHz), and clk_peri follows it — so the UART
//		divisor computed at P0 stops being right the moment the first mode starts. serial.cpp
//		has the same problem and solves it through SERClockChanged, but only for a port the
//		6502 has opened (currentBaudRate != 0), which is never the case here. Hence our own
//		hook, called from HWClockChanged : without it the port talks at the wrong speed and
//		the PC sees nothing usable.

//		Told to IOInitialise, which would otherwise hand these pins back to the 6502 as plain
//		inputs and kill the port. False when the port is off, so nothing is reserved for
//		nothing.

bool DBGOwnsGPIO(int gpio) {
	return dbgOn && (gpio == DBG_TX_PIN || gpio == DBG_RX_PIN);
}

void DBGClockChanged(void) {
	if (dbgOn) uart_set_baudrate(DBG_UART,DBG_BAUD);
}

//		Appending is a pair of writes and a masked increment : safe to call from anywhere on
//		core 0, including from an interrupt. Full ring drops the character rather than wait.

void __not_in_flash_func(DBGWrite)(const char *s) {
	if (!dbgOn) return;
	while (*s != '\0') {
		uint16_t next = (head + 1) & (DBG_RING - 1);
		if (next == tail) return;  												// Full : drop, never block
		ring[head] = *s++;
		head = next;
	}
}

void DBGWriteNumber(const char *label,uint32_t value) {
	char buffer[32];
	snprintf(buffer,sizeof(buffer),"%s=%lu ",label,(unsigned long)value);
	DBGWrite(buffer);
}

//		Called from DSPSync (~95 Hz). Pushes only into free FIFO space, so the cost is a few
//		register writes and the bus loop never waits on the wire.

void __not_in_flash_func(DBGFlush)(void) {
	if (!dbgOn) return;
	while (tail != head && uart_is_writable(DBG_UART)) {
		uart_get_hw(DBG_UART)->dr = (uint8_t)ring[tail];
		tail = (tail + 1) & (DBG_RING - 1);
	}
}

// ***************************************************************************************
//
//		T-73 : one telemetry line per second, and the question it answers.
//
//		When the screen goes black during a DIR in mode 1 (T-71), one cannot tell from the
//		screen whether the DVI signal has dropped or the firmware has stopped. The trace
//		does : if these lines keep coming, core 0 is alive and the fault is in the display
//		path ; if they stop, the firmware is stuck and the display is only the symptom.
//
//		No snprintf here — it lives in flash, and this runs from DSPSync. Hex by hand, in
//		RAM, costs a few dozen cycles and a dozen bytes of ring.
//
// ***************************************************************************************

static const char hexDigits[] = "0123456789ABCDEF";

static void __not_in_flash_func(DBGHex)(char *out,uint32_t value) {
	for (int i = 0;i < 8;i++) out[i] = hexDigits[(value >> ((7 - i) * 4)) & 0xF];
}

void __not_in_flash_func(DBGTelemetry)(uint32_t late,uint32_t rejects,uint32_t sectors,uint32_t mode) {
	char line[64];
	int p = 0;
	line[p++] = 'L';line[p++] = '=';DBGHex(line + p,late);p += 8;
	line[p++] = ' ';
	line[p++] = 'R';line[p++] = '=';DBGHex(line + p,rejects);p += 8;  				// T-71 : lines dropped by the callback
	line[p++] = ' ';
	line[p++] = 'S';line[p++] = '=';DBGHex(line + p,sectors);p += 8;
	line[p++] = ' ';
	line[p++] = 'M';line[p++] = '=';line[p++] = hexDigits[mode & 0xF];
	line[p++] = '\r';line[p++] = '\n';line[p] = '\0';
	DBGWrite(line);
}

//		Called from DSPSync at ~95 Hz ; emits once a second. The tick counter and the
//		comparison are the whole cost when there is nothing to say.

void __not_in_flash_func(DBGTelemetryTick)(void) {
	static uint16_t ticks = 0;
	if (!dbgOn) return;
	if (++ticks < 95) return;
	ticks = 0;
	DBGTelemetry(RNDLateScanlines(),RNDPublishRejects(),stoSectorCount,(uint32_t)GFXGetMode());
}

// ***************************************************************************************
//
//		T-74 : a terminal on the debug port (bmarty 2026-09-25).
//
//		Characters arriving on the UART are pushed into the keyboard queue, so the Neo can be
//		driven from the PC exactly as from its own keyboard ; the console text comes back the
//		other way through FDBWrite (2,20, which already existed). The point is not comfort :
//		it keeps working when the screen is black, which is precisely the state T-71 leaves
//		the machine in and which no amount of screen instrumentation can report.
//
//		A line starting with '!' is read by the firmware instead — one letter, no line
//		buffer. '!!' sends a real '!' through to the 6502.
//
//		Everything here runs from DSPSync, so everything here is in RAM and there is no
//		snprintf : numbers go out as hex through DBGHex. That is the T-46 rule, and it is
//		the rule that a debug channel is most tempted to break.
//
// ***************************************************************************************

extern "C" char __StackLimit,__bss_end__;  										// Provided by the SDK linker script

static void __not_in_flash_func(DBGPair)(const char *label,uint32_t value) {
	char out[16];
	int p = 0;
	while (*label != '\0') out[p++] = *label++;
	out[p++] = '=';
	DBGHex(out + p,value);p += 8;
	out[p++] = ' ';out[p] = '\0';
	DBGWrite(out);
}

//		Starvation : late DVI scanlines (T-57), the two PIO stall counters (T-49) and the
//		sectors moved. A screen that breaks up while L climbs is core 1 starving ; L flat
//		with S climbing says the display path is innocent.

static void __not_in_flash_func(DBGReportStarvation)(void) {
	DBGWrite("\r\nfamines : ");
	DBGPair("late",RNDLateScanlines());
	DBGPair("txstall",HWBusStalls(0));
	DBGPair("rxstall",HWBusStalls(1));
	DBGPair("secteurs",stoSectorCount);
	DBGWrite("\r\n");
}

//		Memory : the heap is what is left between the end of .bss and the stack limit, minus
//		what malloc holds — the TMDS buffers live there, so this is the number that decides
//		whether another video mode can be started.

static void __not_in_flash_func(DBGReportMemory)(void) {
	char *heapEnd = (char *)sbrk(0);
	DBGWrite("\r\nmemoire : ");
	DBGPair("bss_end",(uint32_t)(uintptr_t)&__bss_end__);
	DBGPair("brk",(uint32_t)(uintptr_t)heapEnd);
	DBGPair("pile",(uint32_t)(uintptr_t)&__StackLimit);
	DBGPair("libre",(uint32_t)(&__StackLimit - heapEnd));
	DBGWrite("\r\n");
}


//		Video (T-71). The frame counter is the one number that settles the black screen :
//		core 1 increments it at the start of every frame, so if it climbs while the screen
//		is black the signal is alive and the picture is merely wrong ; if it freezes, the
//		encoder has stopped and the screen is only reporting that.

static void __not_in_flash_func(DBGReportVideo)(void) {
	DBGWrite("\r\nvideo : ");
	DBGPair("mode",(uint32_t)GFXGetMode());
	DBGPair("trames",(uint32_t)RNDGetFrameCount());
	DBGPair("late",RNDLateScanlines());
	DBGPair("rejets",RNDPublishRejects());  									// T-71 lot 1 : lines the callback dropped rather than
	DBGPair("tampons",RNDMonoBuffers());
	DBGPair("dma_secours",RNDDmaWaitEscapes());
	DBGPair("dma_relances",RNDDmaRestarts());  							// T-71 lot 5  							// T-71 lot 4 : doit rester a zero  										// block in the IRQ ; and lot 2 : 2 or 4 line buffers
	DBGPair("x",gMode.xGSize);
	DBGPair("y",gMode.yGSize);
	DBGPair("stride",gMode.stride);
	DBGWrite("\r\n");
}

//		Keyboard (T-68). Whether the keyboard came up at all, and whether Escape ever
//		reached the firmware — the question the boot menu could not answer from the screen.

static void __not_in_flash_func(DBGReportKeyboard)(void) {
	DBGWrite("\r\nclavier : ");
	DBGPair("present",KBDIsPresent() ? 1 : 0);
	DBGPair("esc_vu",KBDEscapeSeen() ? 1 : 0);
	DBGPair("file",KBDIsKeyAvailable() ? 1 : 0);
	DBGWrite("\r\n");
}

//		Placement (T-46, T-48). A function in RAM answers 0x2000xxxx, one in flash
//		0x10xxxxxx. DSPSync and DBGFlush must be in RAM : this is the check that the debug
//		port has not itself broken the rule it exists to police. The flash figure is there
//		for comparison, and because T-48 says placement is what makes the board fail.

static void __not_in_flash_func(DBGReportPlacement)(void) {
	DBGWrite("\r\nplacement : ");
	DBGPair("DSPSync",(uint32_t)(uintptr_t)&DSPSync);
	DBGPair("DBGFlush",(uint32_t)(uintptr_t)&DBGFlush);
	DBGPair("DBGInit",(uint32_t)(uintptr_t)&DBGInitialise);
	DBGWrite("(2xxxxxxx = RAM, 10xxxxxx = flash)\r\n");
}


//		USB (T-70, T-31). Only what the firmware already holds in RAM : the barrier, how many
//		enumeration events it saw, and whether the keyboard came up. Asking TinyUSB itself
//		would mean calling into flash from DSPSync, which is the fault T-46 exists to forbid.

static void __not_in_flash_func(DBGReportUSB)(void) {
	DBGWrite("\r\nusb : ");
	DBGPair("barriere",USBIsSettled() ? 1 : 0);
	DBGPair("evenements",USBEventCount());
	DBGPair("clavier",KBDIsPresent() ? 1 : 0);
	DBGWrite("\r\n");
}

//		Storage (T-54, T-31). One line per FatFs drive : the USB address serving it, 0 when
//		free. A drive still marked busy long after a transfer is the signature T-31 chases.

static void __not_in_flash_func(DBGReportStorage)(void) {
	DBGWrite("\r\nstockage : ");
	for (int drive = 0;drive < 2;drive++) {  									// FF_VOLUMES : the FatFs header is not for this file
		char label[8] = { 'd','r',(char)('0' + drive),'\0' };
		DBGPair(label,STODebugDrive(drive));
	}
	DBGPair("occupe0",STODebugBusy(0) ? 1 : 0);
	DBGPair("secteurs",stoSectorCount);
	DBGWrite("\r\n");
}

static void __not_in_flash_func(DBGCommand)(uint8_t c) {
	switch (c) {
		case 's': DBGReportStarvation();break;
		case 'm': DBGReportMemory();break;
		case 'v': DBGReportVideo();break;
		case 'k': DBGReportKeyboard();break;
		case 'p': DBGReportPlacement();break;
		case 'u': DBGReportUSB();break;
		case 'f': DBGReportStorage();break;
		case 'a': DBGReportStarvation();DBGReportVideo();DBGReportKeyboard();
				  DBGReportUSB();DBGReportStorage();DBGReportMemory();DBGReportPlacement();break;
		case 'z': HWBusStallsReset();DBGWrite("\r\ncompteurs de bus remis a zero\r\n");break;
		//		T-71 lot 2 : the two arms of the buffer experiment, in ONE binary. 0.16.6 was judged
		//		against 0.16.9 while the memory map moved under it (T-48) ; here nothing moves.
		case '2': RNDSetMonoBuffers(2);DBGWrite("\r\nmode 1 : deux tampons de ligne\r\n");break;
		case '4': RNDSetMonoBuffers(4);DBGWrite("\r\nmode 1 : quatre tampons de ligne\r\n");break;
		case '!': KBDInsertQueue('!');break;  									// !! : a real '!' for the 6502
		default:
			DBGWrite("\r\n!s famines  !v video  !k clavier  !m memoire  !p placement\r\n"
					 "!u usb  !f stockage  !a tout  !z remise a zero  !! un '!'\r\n"
					 "!2 !4 tampons de ligne du mode 1 (T-71)\r\n");
			break;
	}
}

//		Called from DSPSync. Reads everything waiting rather than one character per tick,
//		so pasting a line into the terminal does not take a second to arrive.

void __not_in_flash_func(DBGPoll)(void) {
	static bool escaped = false;
	if (!dbgOn) return;
	while (uart_is_readable(DBG_UART)) {
		uint8_t c = (uint8_t)uart_get_hw(DBG_UART)->dr;
		if (escaped) { escaped = false;DBGCommand(c);continue; }
		if (c == '!') { escaped = true;continue; }
		if (c == 10) continue;  												// LF : the terminal sends CR LF, the Neo wants CR
		KBDInsertQueue(c == 127 ? 8 : c);  										// Backspace of the terminal
	}
}

// ***************************************************************************************
//
//		T-75 : find out which UEXT pin the wire is actually on.
//
//		The port stayed silent through two real firmware faults and every cabling permutation,
//		and there is no multimeter here to ask the board directly (bmarty, 2026-09-25). So the
//		firmware answers instead : for the first 30 seconds it moves the UART TX function from
//		one UEXT pin to the next, once a second, announcing each one. Whatever pin the wire is
//		on, one of those announcements lands in the terminal and names it.
//
//		Writing goes straight to the hardware here, not through the ring : the message must be
//		out of the FIFO before the pin changes under it, so we wait for the shift register.
//		That wait is why this only runs during the scan, never in normal operation.
//
//		The candidates are the eight UEXT GPIO. 22-27 are the firmware's I2C and SPI pins ;
//		driving them as UART for one second is harmless with nothing plugged into the port.
//
// ***************************************************************************************

static const uint8_t scanPins[] = { 28,29,22,23,24,25,26,27 };
#define SCAN_COUNT 	(sizeof(scanPins)/sizeof(scanPins[0]))

static void DBGScanSay(uint8_t pin) {
	char text[24] = "\r\nUEXT GPIO=";
	int p = 13;
	text[p++] = '0' + pin / 10;
	text[p++] = '0' + pin % 10;
	text[p++] = '\r';text[p++] = '\n';text[p] = '\0';
	for (const char *c = text;*c != '\0';c++) uart_putc_raw(DBG_UART,*c);
	uart_tx_wait_blocking(DBG_UART);  											// Out of the wire before the pin moves
}

//		Returns true while scanning, so DSPSync knows the normal port is not in charge yet.

bool DBGScanTick(void) {
	static uint16_t ticks = 0;
	static uint8_t index = 0;
	static bool done = false;
	if (done || !dbgOn) return false;
	if (++ticks < 95) return true;  											// One pin per second
	ticks = 0;
	DBGScanSay(scanPins[index]);
	gpio_set_function(scanPins[index],GPIO_FUNC_SIO);  							// Release this one
	gpio_set_dir(scanPins[index],GPIO_IN);
	index++;
	if (index >= SCAN_COUNT) {
		index = 0;
		static uint8_t passes = 0;
		if (++passes >= 4) {  													// 4 passes, then back to normal
			done = true;
			gpio_set_function(DBG_TX_PIN,GPIO_FUNC_UART);
			gpio_set_function(DBG_RX_PIN,GPIO_FUNC_UART);
			DBGWrite("\r\n-- fin du balayage, port sur GPIO 28/29 --\r\n");
			return false;
		}
	}
	gpio_set_function(scanPins[index],GPIO_FUNC_UART);  						// Talk on the next one
	return true;
}
