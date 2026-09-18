// *******************************************************************************************************************************
//
//      Name :      cdc_host_tty.cpp
//      Author :    bmarty <bmarty@mailo.com>
//      Purpose :   HWCDC* for the PC hosts (emulator neo, Phosphoneo) : the "USB CDC device" is a
//                  host tty / pty / file named by the environment variable NEO_CDC_TTY (device 0)
//                  and NEO_CDC_TTY1 (device 1). Not set -> no device connected. F-90.
//
// *******************************************************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "common.h"

static int cdcFd[CDC_MAX_DEVICES] = { -2, -2 };  													// -2 = not tried, -1 = none

static int CDCFd(uint8_t dev) {
	if (dev >= CDC_MAX_DEVICES) return -1;
	if (cdcFd[dev] == -2) {
		const char *env = getenv(dev == 0 ? "NEO_CDC_TTY" : "NEO_CDC_TTY1");
		cdcFd[dev] = env ? open(env, O_RDWR | O_NONBLOCK | O_NOCTTY) : -1;
		if (env && cdcFd[dev] < 0) fprintf(stderr, "CDC : impossible d'ouvrir %s\n", env);
		if (cdcFd[dev] >= 0) {  																	// Raw mode when it is a terminal.
			struct termios t;
			if (tcgetattr(cdcFd[dev], &t) == 0) { cfmakeraw(&t); tcsetattr(cdcFd[dev], TCSANOW, &t); }
		}
	}
	return cdcFd[dev];
}

int HWCDCConnected(uint8_t dev) { return CDCFd(dev) >= 0; }

uint16_t HWCDCReadAvailable(uint8_t dev) {
	int fd = CDCFd(dev), n = 0;
	if (fd < 0 || ioctl(fd, FIONREAD, &n) != 0 || n < 0) return 0;
	return n > 0xFFFF ? 0xFFFF : (uint16_t)n;
}

uint16_t HWCDCRead(uint8_t dev, uint8_t *buffer, uint16_t max) {
	int fd = CDCFd(dev);
	if (fd < 0 || max == 0) return 0;
	ssize_t n = read(fd, buffer, max);
	return n > 0 ? (uint16_t)n : 0;
}

uint16_t HWCDCWrite(uint8_t dev, const uint8_t *buffer, uint16_t count) {
	int fd = CDCFd(dev);
	if (fd < 0) return 0;
	ssize_t n = write(fd, buffer, count);
	return n > 0 ? (uint16_t)n : 0;
}

uint8_t HWCDCSetLineCoding(uint8_t dev, uint32_t baud, uint8_t dataBits, uint8_t parity, uint8_t stopBits) {
	int fd = CDCFd(dev);
	if (fd < 0) return 1;
	struct termios t;
	if (tcgetattr(fd, &t) != 0) return 0;  														// Not a tty (pty/file) : accepted, nothing to do.
	speed_t sp = B115200;
	switch (baud) { case 300: sp = B300; break; case 1200: sp = B1200; break; case 2400: sp = B2400; break; case 9600: sp = B9600; break;
		case 19200: sp = B19200; break; case 38400: sp = B38400; break; case 57600: sp = B57600; break; case 115200: sp = B115200; break; default: break; }
	cfsetispeed(&t, sp); cfsetospeed(&t, sp);
	t.c_cflag &= ~(CSIZE | PARENB | PARODD | CSTOPB);
	t.c_cflag |= (dataBits == 7) ? CS7 : CS8;
	if (parity) t.c_cflag |= PARENB | (parity == 1 ? PARODD : 0);
	if (stopBits == 2) t.c_cflag |= CSTOPB;
	return tcsetattr(fd, TCSANOW, &t) == 0 ? 0 : 1;
}
