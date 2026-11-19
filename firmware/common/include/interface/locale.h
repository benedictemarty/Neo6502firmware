// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      locale.h
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      29th January 2024
//      Reviewed :  No
//      Purpose :   Locale handling
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _NLOCALE_H
#define _NLOCALE_H

uint8_t LOCSetLocale(char c1,char c2);										// T-149 : 0 if set, 1 if the locale is unknown
void LOCGetLocale(uint8_t *c1,uint8_t *c2);									// T-149 : upper-case letters, 0,0 if none
uint8_t LOCLocaleMapping(uint8_t asciiCode,uint8_t keyCode,uint8_t modifiers);

#endif

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
