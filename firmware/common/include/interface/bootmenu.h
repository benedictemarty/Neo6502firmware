// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      bootmenu.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      19th September 2026
//      Purpose :   Trinity boot menu : after the storage is up and before the 6502 starts,
//                  the files of the "boot" directory of the storage (.neo programs, .bin
//                  images for $800) are listed with NeoBASIC ; a digit picks one (default :
//                  NeoBASIC, or neobasic.bin, after BOOT_TIMEOUT). The choice is applied by
//                  the first 1,3 Load BASIC the kernel issues at reset (jmp (0)).
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define BOOT_DIR        "boot"
#define BOOT_MAX        8                                                       // Entries besides NeoBASIC
#define BOOT_NAME_MAX   24
#define BOOT_TIMEOUT    300                                                     // 100 Hz ticks : 3 s

void BOOTSelect(void);                                                          // DSPReset : show the menu, if any
bool BOOTLoadChoice(void);                                                      // MEMLoadBasic : load the choice once, true if done
