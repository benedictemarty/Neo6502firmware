// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      cdcserial.cpp
//      Author :    bmarty <bmarty@mailo.com>
//      Purpose :   Group 14 : USB CDC serial (modems). Parameter 7 of every function is the
//                  device index (0 = first CDC device) so that several adapters can coexist.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

static uint8_t CDCDevice(uint8_t *p) { return (p[7] < CDC_MAX_DEVICES) ? p[7] : 0; }

uint8_t CDCStatus(uint8_t *p) {  												// P0 = connected, P1-2 = bytes available
	uint8_t d = CDCDevice(p);
	p[0] = HWCDCConnected(d) ? 1 : 0;
	uint16_t n = p[0] ? HWCDCReadAvailable(d) : 0;
	p[1] = n & 0xFF;p[2] = n >> 8;
	return 0;
}

uint8_t CDCReadByte(uint8_t *p) {  												// P0 = byte ; error 1 if nothing / no device
	uint8_t d = CDCDevice(p), b;
	if (!HWCDCConnected(d) || HWCDCRead(d, &b, 1) != 1) return 1;
	p[0] = b;
	return 0;
}

uint8_t CDCWriteByte(uint8_t *p) {
	uint8_t d = CDCDevice(p);
	if (!HWCDCConnected(d)) return 1;
	return HWCDCWrite(d, &p[0], 1) == 1 ? 0 : 1;
}

uint8_t CDCReadBlock(uint8_t *p, uint8_t *cpuMem) {  							// P0-1 address, P2-3 max -> P2-3 count read
	uint8_t d = CDCDevice(p);
	uint16_t addr = p[0] | (p[1] << 8), max = p[2] | (p[3] << 8), n = 0;
	if (!HWCDCConnected(d)) return 1;
	if (addr + max > 0x10000) max = 0x10000 - addr;
	n = HWCDCRead(d, cpuMem + addr, max);
	p[2] = n & 0xFF;p[3] = n >> 8;
	return 0;
}

uint8_t CDCWriteBlock(uint8_t *p, uint8_t *cpuMem) {  							// P0-1 address, P2-3 count -> P2-3 count written
	uint8_t d = CDCDevice(p);
	uint16_t addr = p[0] | (p[1] << 8), count = p[2] | (p[3] << 8), n;
	if (!HWCDCConnected(d)) return 1;
	if (addr + count > 0x10000) count = 0x10000 - addr;
	n = HWCDCWrite(d, cpuMem + addr, count);
	p[2] = n & 0xFF;p[3] = n >> 8;
	return 0;
}

uint8_t CDCSetLineCoding(uint8_t *p) {  										// P0-3 baud, P4 data bits, P5 parity (0 N 1 O 2 E), P6 stop bits (1/2)
	uint8_t d = CDCDevice(p);
	uint32_t baud = p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
	if (!HWCDCConnected(d)) return 1;
	return HWCDCSetLineCoding(d, baud, p[4] ? p[4] : 8, p[5], p[6] ? p[6] : 1);
}

// ---------------------------------------------------------------------------
// Routage UART <-> CDC (F-93) : les fonctions 10,13-10,18 (UART UEXT) peuvent
// viser le premier périphérique CDC (modem USB) au lieu du matériel UART, afin
// que les programmes série existants (netsetup.neo, prophet.neo) fonctionnent
// avec un modem USB sans modification. Mode : 0 = UART matériel, 1 = CDC, 2 =
// AUTO (CDC si un modem est présent, sinon matériel). Défaut AUTO.
// ---------------------------------------------------------------------------
#define UART_ROUTE_HW    0
#define UART_ROUTE_CDC   1
#define UART_ROUTE_AUTO  2

static uint8_t sUARTRoute = UART_ROUTE_AUTO;

void UARTRouteSet(uint8_t mode) { sUARTRoute = (mode <= UART_ROUTE_AUTO) ? mode : UART_ROUTE_AUTO; }
uint8_t UARTRouteGet(void) { return sUARTRoute; }

// Le CDC est-il la cible effective ? (device 0)
static bool UARTUseCDC(void) {
	if (sUARTRoute == UART_ROUTE_CDC) return true;
	if (sUARTRoute == UART_ROUTE_AUTO) return HWCDCConnected(0) != 0;
	return false;
}

void UARTRSetFormat(uint32_t baud, uint32_t protocol) {
	if (UARTUseCDC()) HWCDCSetLineCoding(0, baud, 8, 0, 1);   // 8N1 ; les modems l'ignorent souvent
	else IOUARTInitialise(baud, protocol);
}

int UARTRWriteBlock(uint8_t *data, size_t size) {
	if (!UARTUseCDC()) return IOUARTWriteBlock(data, size);
	while (size) { uint16_t n = HWCDCWrite(0, data, size > 0xFFFF ? 0xFFFF : size); if (!n) return 1; data += n; size -= n; }
	return 0;
}

int UARTRReadBlock(uint8_t *data, size_t size) {
	if (!UARTUseCDC()) return IOUARTReadBlock(data, size);
	while (size) {
		uint32_t timeOut = TMRRead() + 500;                  // 5 s, comme l'UART matériel
		uint16_t n = 0;
		while ((n = HWCDCRead(0, data, size > 0xFFFF ? 0xFFFF : size)) == 0) {
			KBDSync();                                       // sert l'hote USB (tuh_task) : sinon rien n'arrive
			if (TMRRead() > timeOut) return 1;               // dans le FIFO CDC pendant l'attente (carte 2026-09-19)
		}
		data += n; size -= n;
	}
	return 0;
}

void UARTRWriteByte(uint8_t b) {
	if (UARTUseCDC()) HWCDCWrite(0, &b, 1); else SERWriteByte(b);
}

bool UARTRByteAvailable(void) {
	return UARTUseCDC() ? (HWCDCReadAvailable(0) != 0) : SERIsByteAvailable();
}

// Lit un octet (P0) ; renvoie 1 si rien n'est disponible.
uint8_t UARTRReadByte(uint8_t *out) {
	if (UARTUseCDC()) { return HWCDCRead(0, out, 1) == 1 ? 0 : 1; }
	if (!SERIsByteAvailable()) return 1;
	*out = SERReadByte();
	return 0;
}
