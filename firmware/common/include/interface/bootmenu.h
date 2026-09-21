// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      bootmenu.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      19th September 2026
//      Purpose :   Trinity boot menu : after the storage is up and before the 6502 starts,
//                  the files of the "boot" directory of the storage (.neo programs, .bin
//                  images for $800, e.g. neobasic.bin) are listed with NeoDOS ; a digit picks one
//                  (default : the embedded NeoDOS after BOOT_TIMEOUT). boot/auto.txt names the
//                  entry started automatically (Escape within 1 s shows the menu). The choice is
//                  applied by the first 1,3 the kernel issues at reset (jmp (0)) ; later 1,3 reload
//                  NeoDOS (Trinity 0.4.0).
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define BOOT_DIR        "boot"
#define BOOT_MAX        8                                                       // Entries besides NeoDOS
#define BOOT_NAME_MAX   24
#define BOOT_TIMEOUT    300                                                     // 100 Hz ticks : 3 s
#define BOOT_AUTO       "auto.txt"                                              // Name of the entry to start automatically
#define BOOT_KEYBOARD_WAIT 800                                                  // Up to 8 s for a USB keyboard to enumerate (T-28)
#define BOOT_AUTO_TIMEOUT 300                                                   // 3 s to press Escape for the menu (bmarty 2026-09-21 : 1 s was too short, the USB keyboard enumerates meanwhile)

void BOOTSelect(void);                                                          // DSPReset : show the menu, if any
bool BOOTLoadChoice(void);                                                      // MEMLoadBasic : load the choice once, true if done
