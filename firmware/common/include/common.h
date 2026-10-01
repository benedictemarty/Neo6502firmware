// ***************************************************************************************
// ***************************************************************************************
//
//		Name : 		common.h	
//		Author :	Paul Robson (paul@robsons.org.uk)
//		Date : 		20th November 2023
//		Reviewed :	No
//		Purpose :	General Include File.
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <ctype.h>
#include <string>

//
//		RP2040 specific includes
//
#ifdef PICO
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/spi.h"
#endif
//
//		PC specific includes
//
#ifdef IBMPC
#include <stdint.h>
#include <stdbool.h>
#include <hardware.h>
#define __time_critical_func(x) x  												// Pico SDK RAM placement : no-op on PC (keyboard.cpp, amont c792b77).
#define __not_in_flash(group)  													// Same for data (T-79 : sound tables read by the PWM interrupt)
#endif
//
//		Debug traces (2026-10-01) : printf on the PC (neo, Phosphoneo), nothing at all on the board, where stdio is
//		disabled (pico_enable_stdio_* 0) and printf used to format every message for nobody — on core 0, with the
//		6502 stopped, at every 12,2 and every program load. The if (0) keeps the arguments "used" (-Werror).
//
#ifdef PICO
#define TRACEF(...) do { if (0) printf(__VA_ARGS__); } while (0)
#else
#define TRACEF(...) printf(__VA_ARGS__)
#endif
//
//		DIAGNOSTIC BUILD (branch diag-coeur1, 2026-10-01) : what core 0 is doing, read by core 1 when a scanline
//		starts being late (dvi_320x240x256.cpp, diagLateOnset[phase]). Set with DiagPhase, which restores the
//		previous phase on the way out (nested calls, early returns).
//
#define DIAG_6502 	0  																// Nothing tagged : the 65C02 runs (CPUExecute)
#define DIAG_API 	1  																// DSPHandler
#define DIAG_SYNC 	2  																// DSPSync (keyboard, USB task, telemetry)
#define DIAG_DISK 	3  																// wait_for_disk_io (USB key)
#define DIAG_P0 	4  																// Boot phases (dispatch.cpp, DSPReset)
#define DIAG_P1 	5
#define DIAG_P2 	6
#define DIAG_P3 	7
#define DIAG_PHASES 8
extern volatile uint8_t diagPhase;
struct DiagPhase {
	uint8_t previous;
	inline DiagPhase(uint8_t phase) { previous = diagPhase;diagPhase = phase; }
	inline ~DiagPhase() { diagPhase = previous; }
};
//
//		Neo6502 Includes
//
#include "interface/keyboard.h"
#include "interface/graphics.h"
#include "interface/console.h"
#include "interface/mouse.h"
#include "interface/cursor.h"
#include "interface/timer.h"
#include "interface/sound.h"
#include "interface/memory.h"
#include "interface/dispatch.h"
#include "interface/maths.h"
#include "interface/fdebug.h"
#include "interface/filesystem.h"
#include "interface/miscellany.h"
#include "interface/mos.h"
#include "interface/sprites.h"
#include "interface/tilemap.h"
#include "interface/serial.h"
#include "interface/turtle.h"
#include "interface/locale.h"
#include "interface/ports.h"
#include "interface/blitter.h"
#include "interface/gamepad.h"
#include "interface/editor.h"
#include "interface/toolbox.h"  														// Toolbox (T-12) : 32 QuickDraw
#include "interface/events.h"  														// 33 Event Manager
#include "interface/windows.h"  														// 34 Window Manager
#include "interface/menus.h"  															// 35 Menu Manager
#include "interface/controls.h"  														// 36 Control Manager
#include "interface/dialogs.h"  														// 37 Dialog Manager
#include "interface/resources.h"  														// 38 Resource Manager
#include "interface/clock.h"  															// Date and time (T-18, F-14 of the fork)
#include "interface/banks.h"  															// Memory banks in flash (T-17)
#include "interface/irq.h"  															// Interrupt tick and frame IRQ (T-14)
#include "interface/usbsettle.h"  														// USB enumeration barrier (T-32)
#include "interface/settings.h"  														// Persistent settings in flash (T-26)
#include "interface/timezone.h"  														// Time zones (T-26)
#include "interface/cdcserial.h"
#include "interface/bootmenu.h"

#endif

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
