// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      msc_recovery.cpp
//      Author :    bmarty
//      Date :      25th November 2026
//      Reviewed :  No
//      Purpose :   Reset Recovery of a USB key (Mass Storage Bulk-Only Transport, 5.3.4).
//
// ***************************************************************************************
// ***************************************************************************************
//
//		T-168 : a command that times out, or whose transfer fails (T-167), leaves the key waiting for
//		the rest of it : the next command is never accepted. The specification recovers with, in order :
//		Bulk-Only Mass Storage Reset, Clear Feature HALT on the bulk IN endpoint, then on the bulk OUT
//		one ; both endpoints then start again at DATA0, on the device and on the host.
//		Apart from usb_storage.cpp so that it can be tested on a PC (tests/usb/test_recovery.cpp).

#include "tusb.h"
#include "host/hcd.h"
#include "pico/time.h"
#include "msc_recovery.h"

#define CONTROL_TIMEOUT_US 	(1000000)  											// Each of the three requests
#define ABORT_SETTLE_US 	(1000)  											// The controller may try the aborted transaction once more

static volatile uintptr_t requestNumber = 0;  									// Of the request awaited : a late answer to a former one is ignored
static volatile bool requestDone;
static volatile bool requestOk;

static void requestComplete(tuh_xfer_t *xfer) {
	if (xfer->user_data != requestNumber) return;
	requestOk = (xfer->result == XFER_RESULT_SUCCESS);
	requestDone = true;
}

static bool request(uint8_t devAddr,uint8_t recipient,uint8_t type,uint8_t code,uint16_t index) {
	tusb_control_request_t setup = {};
	setup.bmRequestType_bit.recipient = recipient;
	setup.bmRequestType_bit.type = type;
	setup.bmRequestType_bit.direction = TUSB_DIR_OUT;
	setup.bRequest = code;
	setup.wValue = 0;  															// For CLEAR_FEATURE : ENDPOINT_HALT
	setup.wIndex = index;
	tuh_xfer_t xfer = {};
	xfer.daddr = devAddr;
	xfer.ep_addr = 0;
	xfer.setup = &setup;
	xfer.complete_cb = requestComplete;
	requestNumber = requestNumber + 1;
	requestDone = false;
	xfer.user_data = requestNumber;
	if (!tuh_control_xfer(&xfer)) return false;
	uint32_t t0 = time_us_32();
	while (!requestDone) {
		MSCRecoveryWait();
		if (!tuh_msc_mounted(devAddr)) return false;
		if (time_us_32() - t0 > CONTROL_TIMEOUT_US) {
			tuh_edpt_abort_xfer(devAddr,0);
			return false;
		}
	}
	return requestOk;
}

bool MSCResetRecovery(uint8_t devAddr) {
	uint8_t interface,epIn,epOut;
	if (!tuh_msc_abort(devAddr,&interface,&epIn,&epOut)) return false;
	uint32_t t0 = time_us_32();
	while (time_us_32() - t0 < ABORT_SETTLE_US) MSCRecoveryWait();  				// Completions already queued are dropped here
	if (!request(devAddr,TUSB_REQ_RCPT_INTERFACE,TUSB_REQ_TYPE_CLASS,MSC_REQ_RESET,interface)) return false;
	if (!request(devAddr,TUSB_REQ_RCPT_ENDPOINT,TUSB_REQ_TYPE_STANDARD,TUSB_REQ_CLEAR_FEATURE,epIn)) return false;
	hcd_edpt_clear_stall(0,devAddr,epIn);
	if (!request(devAddr,TUSB_REQ_RCPT_ENDPOINT,TUSB_REQ_TYPE_STANDARD,TUSB_REQ_CLEAR_FEATURE,epOut)) return false;
	hcd_edpt_clear_stall(0,devAddr,epOut);
	return true;
}
