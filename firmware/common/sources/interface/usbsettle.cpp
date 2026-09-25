// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      usbsettle.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      22nd September 2026
//      Purpose :   USB enumeration barrier (see usbsettle.h).
//
//      0.10.1 : waiting for "quiet" only is wrong — the bus is quiet before enumeration
//      starts too, so the barrier ended at its floor (500 ms) while the key and the modem
//      were still coming up (board, bmarty). It now waits for a first event (or the
//      ceiling), then for the bus to be quiet.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

#ifdef PICO
#define TIMECRITICAL(x) __time_critical_func(x)
#else
#define TIMECRITICAL(x) x
#endif

static uint32_t lastEvent = 0;                                                  // TMRRead() of the last enumeration event
static uint16_t eventCount = 0;
static bool settled = false;
static int settleMs = 0;  														// T-66

void USBNoteEvent(void) {
	lastEvent = TMRRead();
	eventCount++;
	settled = false;                                                            // A device plugged later reopens the question
}

bool USBIsSettled(void) { return settled; }

//		T-74 : read by the !u report, which runs from DSPSync — hence in RAM, and hence not
//		a call into TinyUSB (all of which lives in flash, T-48).
uint16_t TIMECRITICAL(USBEventCount)(void) { return eventCount; }

void USBWaitSettled(void) {
	uint32_t start = TMRRead();
	eventCount = 0;
	while (true) {
		KBDSync();                                                              // Serve the USB host : this is where enumeration happens
		uint32_t now = TMRRead();
		if (now - start >= USB_CEILING) break;                                  // Bounded : nothing (or a slow device) does not hang the boot
		if (now - start < USB_FLOOR) continue;                                  // T-66 : hold the logos on screen first
		if (eventCount > 0 && now - lastEvent >= USB_QUIET) break;              // Quiet bus after the last mount
	}
	settled = true;
	settleMs = (int)((TMRRead() - start) * 10);  								// T-66 : reported later, in P2
}

//		The console is quiet while the logos show, so what the barrier found is announced here,
//		once the text phase has begun.
void USBReport(void) {
	//	T-68b : say whether the keyboard was seen at all, and whether Escape reached the
	//	firmware during the silent phase. Escape at boot still did not open the menu on the
	//	board (bmarty 2026-09-24) and guessing has run its course : KEY means the keyboard
	//	produced at least one character, ESC means the flag the boot menu reads is set.
	CONWriteString("USB settled (%d ms, %d dev)%s%s\r",settleMs,eventCount,
						KBDIsKeyAvailable() ? " KEY" : "",KBDEscapeSeen() ? " ESC" : "");
}
