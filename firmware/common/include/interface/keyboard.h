// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      keyboard.h
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      21st November 2023
//      Reviewed :  No
//      Purpose :   Keyboard prototypes
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

void KBDInitialise(void);
void HWUSBRecover(void);  														// Board : recover the USB host after a flash write (T-30)
bool KBDMediaClaim(uint8_t dev_addr,uint8_t instance,const uint8_t *desc,uint16_t len);  // T-39 : media/system keys interface
void KBDMediaRelease(uint8_t dev_addr,uint8_t instance);
bool KBDMediaReport(uint8_t dev_addr,uint8_t instance,const uint8_t *report,uint16_t len);
void KBDGetMediaKey(uint16_t *usage,uint8_t *system);  							// 2,22
void KBDSetPresent(bool present);  												// Board : a HID keyboard is mounted (T-28)
bool KBDIsPresent(void);
void KBDSync(void);

void KBDCheckTimer(void);
void KBDEvent(uint8_t isDown,uint8_t keyCode,uint8_t modifiers);
uint8_t *KBDGetStateArray(void);
uint8_t KBDGetModifiers(void);
bool KBDIsKeyAvailable(void);
uint8_t KBDGetKey(void);
void KBDSetLocale(char c1,char c2);
uint8_t KBDSetFunctionKey(int fKey,const char *keyText);
void KBDShowFunctionKeys(void);
uint8_t KBDKeyboardController(void);
void KBDInsertQueue(uint8_t ascii);

//
//		Keyboard repeat rates, in 1/100 sec
//
#define KBD_REPEAT_START 	(90)  												// Time for first repeat
#define KBD_REPEAT_AFTER  	(12)  												// Time for subsequent repeats
#endif

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
