// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      main.cpp
//      Author :    Paul Robson (paul@robsons.org.uk)
//      Date :      20th November 2023
//      Reviewed :  No
//      Purpose :   Main program.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "system/processor.h"
#include "system/tick.h"
#include "hardware/watchdog.h"

// ***************************************************************************************
//
//                                  Reset the RP2040
//
// ***************************************************************************************

void ResetSystem(void) {
    CONWriteString("Resetting.\n");
    watchdog_enable(1,1);                                                       // Enable the watchdog timer
    while (true) {}                                                             // Ignore it.
}

// ***************************************************************************************
//
//                                  Main program
//
// ***************************************************************************************

int main() {
    DSPReset();                                                                 // Initialises everything.
    THWStart();
    while (1) CPUExecute();                                                     // Doesn't have to loop but can.
}

// ***************************************************************************************
//
//                                  Dummy debug write
//
// ***************************************************************************************

//		T-74 : the console echo (2,20) now goes to the debug port rather than straight to the
//		6502's serial port. Same wire, but through the ring : CONWrite must never wait on it.
void FDBWrite(uint8_t c) {
    char s[2] = { (char)c,'\0' };
    DBGWrite(s);
}

// ***************************************************************************************
//
//      Date        Revision
//      ====        ========
//
// ***************************************************************************************

