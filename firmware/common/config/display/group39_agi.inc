// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group39_agi.inc
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      2nd October 2026
//      Purpose :   Toolbox group 39, Sierra AGI pictures (T-107)
//
// ***************************************************************************************
// ***************************************************************************************

GROUP 39 AGI Pictures
    FUNCTION 1 Draw Picture
        *DERROR = AGIDrawPicture(DSPGetInt16(DPARAMS,0),DSPGetInt16(DPARAMS,2),DPARAMS[4]);
    DOCUMENTATION
        Trinity (T-107). Decodes the Sierra AGI v2 PICTURE resource at address Parameters:0,1 of 6502 RAM, length
        Parameters:2,3, into the 160x168 AGI plane kept at offset 0 of graphics memory (page $90, 26 880 bytes, one
        byte per pixel : priority in the high nibble, visual colour in the low nibble). If bit 0 of Parameter:4 is
        set the plane is first cleared (priority 4, visual 15). Generic : works for any AGI game. The 6502 is held
        for the whole decoding (a few ms to about 40 ms). Errors : 1 address range outside 6502 RAM or length 0,
        2 fill stack overflow. Graphics memory is shared with sprites, tiles and images : a program
        using this group must not use them.

    FUNCTION 2 Show Picture
        *DERROR = AGIShowPicture(DSPGetInt16(DPARAMS,0));
    DOCUMENTATION
        Copies the visual colours (0-15) of the AGI plane to the screen draw page, from line Parameters:0,1, each
        pixel doubled in width (320 x 168). Modes 320 pixels wide with 8 or 4 bits per pixel. The palette is not
        changed. Error 1 if the mode is not supported or the picture does not fit.

    FUNCTION 3 Picture Duration
        {
            uint32_t v = AGILastDuration();
            DPARAMS[0] = v & 0xFF;DPARAMS[1] = (v >> 8) & 0xFF;DPARAMS[2] = (v >> 16) & 0xFF;DPARAMS[3] = v >> 24;
        }
    DOCUMENTATION
        Returns in Parameters:0-3 the duration of the last Draw Picture in microseconds (board only, 0 on the
        emulators).

    FUNCTION 4 Read Picture Point
        *DERROR = AGIReadPoint(DPARAMS[0],DPARAMS[1],DPARAMS+2,DPARAMS+3);
    DOCUMENTATION
        Returns the visual colour (Parameter:2) and the priority (Parameter:3) of the point Parameter:0,
        Parameter:1 (X 0-159, Y 0-167) of the AGI plane. Error 1 if out of range.
