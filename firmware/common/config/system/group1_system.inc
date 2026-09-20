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
        

    FUNCTION 20 Get Date Time
        CLKGetParams(DPARAMS);
    DOCUMENTATION
        Returns the date and time (Trinity T-18, F-14 of the fork) : Parameters:0,1 year (1970-2099), Parameter:2 month (1-12),
        Parameter:3 day (1-31), Parameter:4 hour (0-23), Parameter:5 minute, Parameter:6 second and Parameter:7 the
        source : 0 = not set (the value is then 1970-01-01 plus the time since power on), 1 = software clock set by
        Function 1,21 and advanced by the 100 Hz timer, 2 = PCF8563 real time clock found on the UEXT I2C bus (address $51).

    FUNCTION 21 Set Date Time
        *DERROR = CLKSetParams(DPARAMS);
    DOCUMENTATION
        Sets the date and time (Trinity T-18, F-14 of the fork), same layout as Function 1,20 (Parameter:7 ignored) : the software
        clock is set, and the PCF8563 too when present. Sets the error flag if a field is out of range (1970-2099,
        valid day of month, 24 hour time).

    FUNCTION 18 Select Bank
        *DERROR = BNKSelect(DPARAMS[0],DSPGetInt16(DPARAMS,1));
    DOCUMENTATION
        Memory banks in flash (Trinity T-17, from F-23 of the fork). Copies bank Parameter:0 (0-31, 8k each, kept in
        the RP2040 flash and read in place) into the 8k window at Parameters:1,2 of 6502 RAM : the window address is
        page aligned and the window ends below $FF00. The copy takes place while the 65C02 waits for the call ; the
        window must not hold the code executing the call, its stack or its zero page. The window is a copy : changing
        it does not change the bank (see Function 1,22). Bank $FF forgets the mapping and leaves the window as it is.
        Sets the error flag if the bank or the address is invalid. A bank never written reads as $FF. The banks are
        also blitter pages $A0 (bank 0) to $BF, read only, for Function 12,2 (bank to video RAM for instance).

    FUNCTION 19 Get Bank Info
        {
            uint8_t b;uint16_t a;
            BNKGetState(&b,&a);
            DPARAMS[0] = b;DSPSetInt16(DPARAMS,1,a);
            DPARAMS[3] = BANK_COUNT;DSPSetInt16(DPARAMS,4,BANK_SIZE);
        }
    DOCUMENTATION
        Returns the last bank selected in Parameter:0 ($FF if none since reset) and its window address in
        Parameters:1,2, the number of banks in Parameter:3 and the bank size in Parameters:4,5 (Trinity T-17).

    FUNCTION 22 Write Bank
        *DERROR = BNKWrite(DPARAMS[0],DSPGetInt16(DPARAMS,1));
    DOCUMENTATION
        Programs bank Parameter:0 with the 8k at Parameters:1,2 of 6502 RAM (same constraints as Function 1,18) : on the
        board the flash sectors are erased and written, the display pauses for about 150 ms, and the content is verified ;
        a written bank survives resets and reflashing of the firmware (it lives in the top 256k of the flash). The
        emulators keep the banks in the file "banks.flash" of the storage directory. Sets the error flag if the bank or
        the address is invalid or the verification fails (Trinity T-17). Flash wears : write banks once, not in loops.

    FUNCTION 12 Set Interrupt Tick
        *DERROR = IRQSetTick(DSPGetInt16(DPARAMS,0));
    DOCUMENTATION
        Starts (or stops with 0) a periodic interrupt to the 65C02 at Parameters:0,1 Hz (1..1000) on IRQB (Trinity T-14,
        F-60 of the fork). The interrupt vector $FFFE/$FFFF is in RAM : install a handler there first, then CLI. IRQB is
        held low until the 65C02 fetches the vector at $FFFF, so the handler needs no acknowledge and cannot re-enter ;
        with I=1 the line stays low until CLI (ticks coalesce, none is lost). An interrupt handler may call the API but
        must save and restore $FF01-$FF0B if the main program could be preparing a call. DSP Reset (1,0) stops the tick.
        First delivery on a board : to be validated (bus loop timing, see docs/BACKLOG.md T-14).

    FUNCTION 13 Get Interrupt Tick
        DPARAMS[0] = IRQGetTick() & 0xFF;DPARAMS[1] = IRQGetTick() >> 8;
    DOCUMENTATION
        Returns the current interrupt tick rate in Parameters:0,1 (0 = off).

    FUNCTION 16 Set Frame Interrupt
        *DERROR = IRQSetFrame(DPARAMS[0]);
    DOCUMENTATION
        Parameter:0 non zero : an interrupt is delivered to the 65C02 at the start of every displayed frame (60 Hz vertical
        sync of the DVI output), 0 stops it (Trinity T-14, F-10 of the fork). Same delivery as Function 1,12 : IRQB is held
        low until the vector fetch at $FFFF ; a frame arriving while interrupts are disabled coalesces with the next. The
        tick and the frame interrupt may both be on : the handler cannot tell them apart (use 5,37 Frame Count or 1,1 Timer).
        Stopped by DSP Reset (1,0).

    FUNCTION 17 Get Frame Interrupt
        DPARAMS[0] = IRQGetFrame();
    DOCUMENTATION
        Returns in Parameter:0 whether the frame interrupt (Function 1,16) is on.
