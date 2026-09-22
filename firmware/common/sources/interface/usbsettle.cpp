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

static uint32_t lastEvent = 0;                                                  // TMRRead() of the last enumeration event
static uint16_t eventCount = 0;
static bool settled = false;

void USBNoteEvent(void) {
	lastEvent = TMRRead();
	eventCount++;
	settled = false;                                                            // A device plugged later reopens the question
}

bool USBIsSettled(void) { return settled; }

void USBWaitSettled(void) {
	uint32_t start = TMRRead();
	eventCount = 0;
	while (true) {
		KBDSync();                                                              // Serve the USB host : this is where enumeration happens
		uint32_t now = TMRRead();
		if (now - start >= USB_CEILING) break;                                  // Bounded : nothing (or a slow device) does not hang the boot
		if (eventCount > 0 && now - lastEvent >= USB_QUIET) break;              // Quiet bus after the last mount
	}
	settled = true;
	CONWriteString("USB settled (%d ms, %d dev)\r",(int)((TMRRead() - start) * 10),eventCount);
}
