// multiboot.h — API side of the RP2040 multi-boot (F-81). Implementation specific.
#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H
uint8_t HWRebootImage(uint8_t slot);  											// Does not return on the board ; 1 = unsupported / bad slot
uint8_t HWGetImageName(uint8_t slot, char *name, int maxLen);  					// 0 + ASCIIZ name, 1 if none
#endif
