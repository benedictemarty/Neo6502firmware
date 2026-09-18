// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group14_cdc.inc
//      Author :    bmarty <bmarty@mailo.com>
//      Purpose :   USB CDC serial dispatcher (modems, USB-serial adapters) — F-90.
//
// ***************************************************************************************
// ***************************************************************************************

GROUP 14 USB Serial (CDC)
    FUNCTION 1 Serial Status
        *DERROR = CDCStatus(DPARAMS);
    DOCUMENTATION
        USB CDC-ACM serial device (modem, USB-serial adapter) on the Neo6502 host port. Parameter:7 selects the device (0 = first).
        Returns Parameter:0 = 1 if the device is connected, Parameters:1,2 = number of bytes waiting to be read.

    FUNCTION 2 Serial Read Byte
        *DERROR = CDCReadByte(DPARAMS);
    DOCUMENTATION
        Read one byte from the CDC device (Parameter:7) into Parameter:0. Sets the error flag if no byte is available or no device is connected.

    FUNCTION 3 Serial Write Byte
        *DERROR = CDCWriteByte(DPARAMS);
    DOCUMENTATION
        Write the byte in Parameter:0 to the CDC device (Parameter:7). Sets the error flag if no device is connected.

    FUNCTION 4 Serial Read Block
        *DERROR = CDCReadBlock(DPARAMS,cpuMemory);
    DOCUMENTATION
        Read up to Parameters:2,3 bytes into memory at Parameters:0,1 from the CDC device (Parameter:7) ; Parameters:2,3 return the count actually read (may be 0).

    FUNCTION 5 Serial Write Block
        *DERROR = CDCWriteBlock(DPARAMS,cpuMemory);
    DOCUMENTATION
        Write Parameters:2,3 bytes from memory at Parameters:0,1 to the CDC device (Parameter:7) ; Parameters:2,3 return the count actually written.

    FUNCTION 6 Serial Set Line Coding
        *DERROR = CDCSetLineCoding(DPARAMS);
    DOCUMENTATION
        Set the line coding of the CDC device (Parameter:7) : Parameters:0..3 = baud rate (32 bits), Parameter:4 = data bits (default 8),
        Parameter:5 = parity (0 none, 1 odd, 2 even), Parameter:6 = stop bits (1 or 2). Many USB modems ignore it.
