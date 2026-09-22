// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      cdc.cpp
//      Author :    bmarty <bmarty@mailo.com>
//      Purpose :   USB CDC-ACM host (TinyUSB cdc_host) : modems and USB-serial adapters on
//                  the Neo6502 host port (F-90). The class driver is polled by tuh_task
//                  (KBDSync) ; mount/unmount callbacks keep the table of interfaces.
//                  FTDI / CP210x adapters are handled by the same class when enabled in
//                  tusb_config.h (CFG_TUH_CDC_FTDI / CFG_TUH_CDC_CP210X).
//      NOT YET RUN ON A BOARD (verified on libemul with an emulated CDC modem).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "tusb.h"

static uint8_t cdcItf[CDC_MAX_DEVICES];  										// TinyUSB interface index per device, 0xFF = none
static bool cdcInit = false;

static void CDCInit(void) {
	if (cdcInit) return;
	for (int i = 0; i < CDC_MAX_DEVICES; i++) cdcItf[i] = 0xFF;
	cdcInit = true;
}

extern "C" void tuh_cdc_mount_cb(uint8_t idx) {  								// New CDC interface : first free slot.
	USBNoteEvent();  															// T-32 : enumeration barrier
	CDCInit();
	for (int i = 0; i < CDC_MAX_DEVICES; i++) if (cdcItf[i] == 0xFF) { cdcItf[i] = idx; break; }
	cdc_line_coding_t lc = { 115200, CDC_LINE_CODING_STOP_BITS_1, CDC_LINE_CODING_PARITY_NONE, 8 };
	tuh_cdc_set_line_coding(idx, &lc, NULL, 0);  									// Sane default ; DTR/RTS asserted so that modems talk.
	tuh_cdc_set_control_line_state(idx, CDC_CONTROL_LINE_STATE_DTR | CDC_CONTROL_LINE_STATE_RTS, NULL, 0);
	tuh_itf_info_t info;  														// Say so on the console, like the gamepad driver does.
	uint16_t vid = 0, pid = 0;
	if (tuh_cdc_itf_get_info(idx, &info)) tuh_vid_pid_get(info.daddr, &vid, &pid);
	CONWriteString("USB serial modem found ");CONWriteHex(vid);CONWriteHex(pid);CONWrite('\r');
}

extern "C" void tuh_cdc_umount_cb(uint8_t idx) {
	USBNoteEvent();  															// T-32
	CDCInit();
	for (int i = 0; i < CDC_MAX_DEVICES; i++) if (cdcItf[i] == idx) cdcItf[i] = 0xFF;
	CONWriteString("USB serial modem removed\r");
}

int HWCDCConnected(uint8_t dev) {
	CDCInit();
	if (dev >= CDC_MAX_DEVICES || cdcItf[dev] == 0xFF || !tuh_cdc_mounted(cdcItf[dev])) return 0;
	tuh_itf_info_t info;  														// Device still there ? (also keeps the symbol for the co-sim HLE)
	return tuh_cdc_itf_get_info(cdcItf[dev], &info) ? 1 : 0;
}

uint16_t HWCDCReadAvailable(uint8_t dev) {
	if (!HWCDCConnected(dev)) return 0;
	uint32_t n = tuh_cdc_read_available(cdcItf[dev]);
	return n > 0xFFFF ? 0xFFFF : (uint16_t)n;
}

uint16_t HWCDCRead(uint8_t dev, uint8_t *buffer, uint16_t max) {
	if (!HWCDCConnected(dev)) return 0;
	return (uint16_t)tuh_cdc_read(cdcItf[dev], buffer, max);
}

uint16_t HWCDCWrite(uint8_t dev, const uint8_t *buffer, uint16_t count) {
	if (!HWCDCConnected(dev)) return 0;
	uint32_t room = tuh_cdc_write_available(cdcItf[dev]);  						// (also keeps the symbol for the co-sim HLE)
	if (count > room) count = (uint16_t)room;
	uint32_t n = tuh_cdc_write(cdcItf[dev], buffer, count);
	tuh_cdc_write_flush(cdcItf[dev]);
	return (uint16_t)n;
}

uint8_t HWCDCSetLineCoding(uint8_t dev, uint32_t baud, uint8_t dataBits, uint8_t parity, uint8_t stopBits) {
	if (!HWCDCConnected(dev)) return 1;
	cdc_line_coding_t lc;
	lc.bit_rate = baud;
	lc.stop_bits = (stopBits == 2) ? CDC_LINE_CODING_STOP_BITS_2 : CDC_LINE_CODING_STOP_BITS_1;
	lc.parity = (parity == 1) ? CDC_LINE_CODING_PARITY_ODD : (parity == 2) ? CDC_LINE_CODING_PARITY_EVEN : CDC_LINE_CODING_PARITY_NONE;
	lc.data_bits = dataBits;
	return tuh_cdc_set_line_coding(cdcItf[dev], &lc, NULL, 0) ? 0 : 1;
}

