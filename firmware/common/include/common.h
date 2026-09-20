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
#endif
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
#include "interface/cdcserial.h"
#include "interface/bootmenu.h"

#endif

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
