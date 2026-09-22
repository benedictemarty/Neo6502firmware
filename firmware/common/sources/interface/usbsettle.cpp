// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      usbsettle.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      22nd September 2026
//      Purpose :   USB enumeration barrier (see usbsettle.h).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

static uint32_t lastEvent = 0;                                                  // TMRRead() of the last enumeration event
static bool settled = false;

void USBNoteEvent(void) {
	lastEvent = TMRRead();
	settled = false;                                                            // A device plugged later reopens the question
}

bool USBIsSettled(void) { return settled; }

void USBWaitSettled(void) {
	uint32_t start = TMRRead();
	lastEvent = start;
	while (true) {
		KBDSync();                                                              // Serve the USB host : this is where enumeration happens
		uint32_t now = TMRRead();
		if (now - start >= USB_CEILING) break;                                  // Bounded : never wait longer than this
		if (now - start >= USB_FLOOR && now - lastEvent >= USB_QUIET) break;    // Quiet bus after a minimum of discovery
	}
	settled = true;
	CONWriteString("USB settled (%d ms)\r",(int)((TMRRead() - start) * 10));
}
