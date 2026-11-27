// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      msc_recovery.cpp
//      Author :    bmarty
//      Date :      27th November 2026
//      Reviewed :  No
//      Purpose :   Recovery of a USB key that stopped answering : enumerate it again.
//
// ***************************************************************************************
// ***************************************************************************************
//
//		T-168 : a command that times out, or whose transfer fails (T-167), leaves the key unusable.
//		The Bulk-Only Reset Recovery of the Mass Storage specification (0.16.117) did not bring it
//		back on the board : unanswered after a data sequence error, and accepted but followed by
//		other timeouts after a spontaneous one. A software reboot did. So the port of the key on the
//		hub is reset and the key enumerated again, as if it had just been plugged in.
//
//		The FatFs volume stays mounted meanwhile, so that the current directory and the open files
//		survive. It is given up if the key does not come back in time, or if another one answers :
//		same address, vendor, product and size are required.
//
//		Apart from usb_storage.cpp so that it can be tested on a PC (tests/usb/test_recovery.cpp).

#include "tusb.h"
#include "host/hub.h"
#include "pico/time.h"
#include "msc_recovery.h"

#define ABORT_SETTLE_US 	(1000)  											// The controller may try the aborted transaction once more
#define REPLUG_TIMEOUT_US 	(3000000)  											// Port reset, enumeration, mount of the class

static struct {
	uint32_t blocks;
	uint16_t vid,pid;
	uint8_t addr;  																// 0 : no recovery under way
	bool held,back;  															// Removed with its volume kept ; mounted again
} key;

static void giveUp(void) {
	uint8_t addr = key.addr;
	key.addr = 0;
	if (key.held) MSCRecoveryLost(addr);
}

bool MSCRecoveryHoldsMount(uint8_t devAddr) {
	if (devAddr == 0 || devAddr != key.addr) return false;
	key.held = true;
	return true;
}

bool MSCRecoveryReturned(uint8_t devAddr) {
	if (key.addr == 0 || !key.held) return false;
	uint16_t vid = 0,pid = 0;
	tuh_vid_pid_get(devAddr,&vid,&pid);
	bool same = (vid == key.vid && pid == key.pid && tuh_msc_get_block_count(devAddr,0) == key.blocks);
	if (same && devAddr == key.addr) {
		key.back = true;
		key.addr = 0;
		return true;
	}
	if (same || devAddr == key.addr) giveUp();  								// Moved, or another key in its place
	return false;
}

bool MSCRecoverKey(uint8_t devAddr) {
	if (key.addr != 0) return false;
	tuh_bus_info_t bus;
	if (!tuh_bus_info_get(devAddr,&bus) || bus.hub_addr == 0) return false;  	// Only behind a hub
	tuh_vid_pid_get(devAddr,&key.vid,&key.pid);
	key.blocks = tuh_msc_get_block_count(devAddr,0);
	uint8_t interface,epIn,epOut;
	if (!tuh_msc_abort(devAddr,&interface,&epIn,&epOut)) return false;
	uint32_t t0 = time_us_32();
	while (time_us_32() - t0 < ABORT_SETTLE_US) MSCRecoveryWait();  				// Completions already queued are dropped here
	key.held = key.back = false;
	key.addr = devAddr;
	if (!tuh_trinity_hub_replug(bus.hub_addr,bus.hub_port)) {
		key.addr = 0;
		return false;
	}
	t0 = time_us_32();
	while (key.addr != 0) {
		MSCRecoveryWait();
		if (key.addr != 0 && time_us_32() - t0 > REPLUG_TIMEOUT_US) giveUp();
	}
	return key.back;
}
