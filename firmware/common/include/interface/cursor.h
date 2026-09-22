// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      cursor.h
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      13th April 2024
//      Reviewed :  No
//      Purpose :   Cursor header
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _CURSOR_H
#define _CURSOR_H

#define CURSOR_IMAGE_BYTES (256)  												// One cursor image : 16 x 16 bytes (T-44)

void CURInitialise(void);
const uint8_t *CURGetCurrent(uint8_t *xHit,uint8_t *yHit);  					// Points into cursor_data : flash on the board
bool CURSetCurrent(uint8_t cursorID);


#endif

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
