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
        2 fill stack overflow. The fill stack uses graphics memory just after the plane (offsets 26 880 to
        30 975, 4 096 bytes). Graphics memory is shared with sprites, tiles and images : a program
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

    FUNCTION 5 Draw Cel
        *DERROR = AGIDrawCel(DSPGetInt16(DPARAMS,0),DPARAMS[2],DPARAMS[3],DPARAMS[4],DPARAMS[5],DSPGetInt16(DPARAMS,6));
    DOCUMENTATION
        Trinity (T-109). Draws the AGI view cel whose header (width, height, then a byte in PC format : bit 7
        mirror, bits 6-4 normal loop, low nibble transparent colour ; only the transparent colour is read, then
        the RLE lines : colour in the high nibble, length in the low nibble, 0 at the end of each line) is at
        address Parameters:0,1 of 6502 RAM, its left side at X Parameter:2, its bottom line at Y Parameter:3
        (AGI pixels), with the object priority Parameter:4 ; bit 0 of Parameter:5 mirrors it horizontally. The
        plane top is at screen line Parameters:6,7. A pixel is drawn when it is not transparent and the object
        priority is greater than or equal to the plane priority at that point ; on a control line (priority 0 to
        3) the first point below which is not one is used, 15 if the bottom is reached (assumptions, see
        Neo6502AGI docs/specs/view-v2.md). The cel is clipped to the plane ; a line longer than the width is cut.
        Draw Cel never writes the plane. The caller handles the order of the objects : after Restore Rect has
        erased an object, the objects it overlapped must be drawn again. Errors : 1 empty cel, position outside
        the plane, header outside 6502 RAM or unsupported mode ; 2 cel data cut by the end of 6502 RAM.

    FUNCTION 6 Restore Rect
        *DERROR = AGIRestoreRect(DPARAMS[0],DPARAMS[1],DPARAMS[2],DPARAMS[3],DSPGetInt16(DPARAMS,4));
    DOCUMENTATION
        Trinity (T-109). Copies the visual colours of the AGI plane to the screen on the rectangle X Parameter:0, Y
        Parameter:1, width Parameter:2, height Parameter:3 (AGI pixels, clipped to the plane), the plane top
        being at screen line Parameters:4,5 : this erases the objects drawn there. Error 1 if the width or the
        height is 0 or the mode is not supported.

    FUNCTION 7 Add Cel To Plane
        *DERROR = AGIAddCelToPlane(DSPGetInt16(DPARAMS,0),DPARAMS[2],DPARAMS[3],DPARAMS[4],DPARAMS[5]);
    DOCUMENTATION
        Trinity (T-122), for the AGI command add.to.pic. Same parameters as Draw Cel (cel header at Parameters:0,1
        of 6502 RAM, left X Parameter:2, bottom Y Parameter:3, priority Parameter:4, bit 0 of Parameter:5 =
        mirror) without screen line : the cel is written into the AGI plane, not to the screen. Each point which
        is not transparent and passes the priority test of Draw Cel (priority greater than or equal to the plane
        priority, control lines included) receives the cel colour and the given priority. The priority must be 4
        to 15 (0, "from Y", is computed by the caller). Same clipping as Draw Cel ; Restore Rect then shows the
        result. Errors : 1 priority outside 4-15 and the errors 1 and 2 of Draw Cel (the screen mode is not used).

    FUNCTION 8 Fill Plane Priority
        *DERROR = AGIFillPlanePriority(DPARAMS[0],DPARAMS[1],DPARAMS[2],DPARAMS[3],DPARAMS[4]);
    DOCUMENTATION
        Trinity (T-122). Sets to Parameter:4 (0 to 15) the priority of the points of the AGI plane in the rectangle
        X Parameter:0, Y Parameter:1, width Parameter:2, height Parameter:3 (AGI pixels, clipped to the plane) ;
        the visual colours are not changed. Used for the margin of add.to.pic (priority 0 to 3). Error 1 if the
        width or the height is 0 or the priority is above 15.
