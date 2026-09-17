// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group1_system.inc
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      22nd November 2023
//      Reviewed :  No
//      Purpose :   System group dispatcher.
//
// ***************************************************************************************
// ***************************************************************************************

// ***************************************************************************************
//
//                                  Group 1 (System)
//
// ***************************************************************************************

GROUP 1 System
    FUNCTION 0 DSP Reset
        DSPReset();
    DOCUMENTATION
        Resets the messaging system and component systems. Normally, should not be used.

    FUNCTION 1 Timer
        *((uint32_t *)DPARAMS) = TMRRead(); // (assumes littleendian)
    DOCUMENTATION
        Deposit the value (32-bits) of the 100Hz system timer into Parameters:0..3.

    FUNCTION 2 Key Status
        i1 = DPARAMS[0];
        DPARAMS[0] = (i1 < KBD_MAX_KEYCODE) ? KBDGetStateArray()[i1] : 0;
        DPARAMS[1] = KBDGetModifiers();
    DOCUMENTATION
        Deposits the state of the specified keyboard key into Parameter:0.
        State of keyboard modifiers (Shift/Ctrl/Alt/Meta) is returned in Parameter:1.
        The key which to query is specified in Parameter:0.

    FUNCTION 3 Basic
        MEMLoadBasic();
    DOCUMENTATION
        Loads and allows the execution of BASIC via a indirect jump through address zero.

    FUNCTION 4 Credits
        CONWriteString((char *)whoList);
    DOCUMENTATION
        Print the Neo6502 project contributors (stored in flash memory).

    FUNCTION 5 Serial Status
        SERCheckDataAvailable();
    DOCUMENTATION
        Check the serial port to see if there is a data transmission.

    FUNCTION 6 Locale
        LOCSetLocale(DPARAMS[0],DPARAMS[1]);
    DOCUMENTATION
        Set the locale code specified in Parameters:0,1 as upper-case ASCII letters.
        Parameter:0 takes the first letter and Parameter:1 takes the second letter.

        For example: French (FR) would require Parametr 0 being $46 and Parameter 1 being $52

    FUNCTION 7 System Reset
        ResetSystem();
    DOCUMENTATION
        System Reset. This is a full hardware reset. It resets the RP2040 using the Watchdog timer, and this also resets the 65C02.

    FUNCTION 8 MOS
        *DERROR = MOSExecute(cpuMemory+DSPGetInt16(DCOMMAND,4));
    DOCUMENTATION
        Do a MOS command (a '* command') these are specified in the Wiki as they will be steadily expanded.

    FUNCTION 12 Set Interrupt Tick
        *DERROR = IRQSetTick(DSPGetInt16(DCOMMAND,4));
    DOCUMENTATION
        Start (or stop with 0) a periodic interrupt to the 65C02 at Parameters:0,1 Hz (1..1000), delivered as a short pulse on IRQB (F-60).
        The interrupt vector $FFFE/$FFFF is in RAM : install a handler there first, then CLI. No acknowledge is needed ; a tick
        arriving while interrupts are disabled is lost. An interrupt handler may call the API but must save and restore
        $FF01-$FF0B if the main program could be preparing a call. The DSP Reset (1,0) stops the tick.

    FUNCTION 13 Get Interrupt Tick
        DPARAMS[0] = IRQGetTick() & 0xFF;DPARAMS[1] = IRQGetTick() >> 8;
    DOCUMENTATION
        Return the current interrupt tick rate in Parameters:0,1 (0 = off).

    FUNCTION 14 Reboot Image
        *DERROR = HWRebootImage(DPARAMS[0]);
    DOCUMENTATION
        Multi-boot (F-80/F-81) : restart the RP2040 on the firmware image in flash slot Parameter:0 (0 = this Neo6502 firmware,
        1-3 = other images such as reload-emulator machines). Does not return when it succeeds ; sets the error flag if the
        slot is empty or if multi-boot is not installed (emulators).

    FUNCTION 15 Get Image Name
        {
            char name[32];
            *DERROR = HWGetImageName(DPARAMS[0], name, sizeof(name));
            if (*DERROR == 0) DSPSetStdString(DPARAMS, 1, std::string(name));
        }
    DOCUMENTATION
        Multi-boot : Parameter:0 = slot, Parameters:1,2 = length-prefixed buffer (byte 0 = its size) receiving the image name.
        Sets the error flag if the slot is empty or if multi-boot is not installed.

    FUNCTION 18 Select Bank
        *DERROR = BNKSelect(DPARAMS[0],DSPGetInt16(DPARAMS,1));
    DOCUMENTATION
        Memory banks (bmarty fork, F-23). Copies bank Parameter:0 into the 8k window at Parameters:1,2 of 6502 RAM,
        after writing the bank currently mapped (if any) back to its storage. The window address is page aligned and
        the window must end below $FF00. Bank $FF only writes the current bank back and unmaps it. Sets the error flag
        if the bank or the address is invalid. Bank storage lives outside 6502 RAM (see Function 1,19 for the bank
        count and size) and is preserved across mappings, so a program can keep code or data in several banks and
        swap them through one window : the copy takes place while the 65C02 waits for the call (no wait is needed
        afterwards, the window is complete when the call returns). The window must not hold the code executing the
        call, its stack or its zero page. The banks are zeroed at start-up and the mapping is forgotten on DSP Reset.

    FUNCTION 19 Get Bank Info
        {
            uint8_t b;uint16_t a;
            BNKGetState(&b,&a);
            DPARAMS[0] = b;DSPSetInt16(DPARAMS,1,a);
            DPARAMS[3] = BANK_COUNT;DSPSetInt16(DPARAMS,4,BANK_SIZE);
        }
    DOCUMENTATION
        Returns the current bank in Parameter:0 ($FF if none) and its window address in Parameters:1,2, the number of
        banks in Parameter:3 and the bank size in Parameters:4,5 (bmarty fork, F-23).

    FUNCTION 10 Write character to debug
        FDBWrite(DPARAMS[0]);
    DOCUMENTATION
        Writes a single character to the debug port (the UART on the Pico, or stderr on the emulator). This allows maximum flexibility.
        

    FUNCTION 11 Return Version Information
        DPARAMS[0] = VERSION_MAJOR;
        DPARAMS[1] = VERSION_MINOR;
        DPARAMS[2] = VERSION_PATCH;                
    DOCUMENTATION
        Reads the current version Major.Minor.Patch into Parameters:0..2 These values are guaranteed to be in the range 0.255
        