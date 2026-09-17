// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group32_quickdraw.inc
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      17th September 2026
//      Purpose :   Toolbox group 32, QuickDraw (ADR-01, F-40 skeleton)
//
//      Conventions : structures in 6502 RAM passed by 16 bit address ; Rect = left, top,
//      right, bottom (int16, right and bottom exclusive) ; errors 1 = bad parameter.
//
// ***************************************************************************************
// ***************************************************************************************

GROUP 32 QuickDraw
    FUNCTION 1 Init Graf
        QDInitGraf();
    DOCUMENTATION
        Toolbox (bmarty fork, ADR-01). Resets the drawing port : the clip rectangle is the whole screen, the pen is
        at (0,0) with colour 15. Structures of this group live in 6502 RAM and are passed by address : a Rect is
        left, top, right, bottom as signed 16 bit values, right and bottom exclusive. Drawing is always clipped to
        the clip rectangle and goes to the draw page, ignoring the sprite layer.

    FUNCTION 2 Set Clip
        *DERROR = QDSetClip(DSPGetInt16(DPARAMS,0));
    DOCUMENTATION
        Sets the clip rectangle to the Rect at Parameters:0,1, intersected with the screen. Error if the Rect is
        inverted or its address is not in RAM.

    FUNCTION 3 Get Clip
        *DERROR = QDGetClip(DSPGetInt16(DPARAMS,0));
    DOCUMENTATION
        Writes the current clip rectangle into the Rect at Parameters:0,1.

    FUNCTION 4 Set Pen Colour
        QDSetPenColour(DPARAMS[0]);
    DOCUMENTATION
        Sets the pen colour (palette index) used by Frame Rect and Paint Rect to Parameter:0.

    FUNCTION 5 Move To
        QDMoveTo((int16_t)DSPGetInt16(DPARAMS,0),(int16_t)DSPGetInt16(DPARAMS,2));
    DOCUMENTATION
        Moves the pen to Parameters:0,1 (X) and Parameters:2,3 (Y), signed 16 bit, without drawing.

    FUNCTION 6 Get Pen
        {
            int16_t x,y;uint8_t c;
            QDGetPen(&x,&y,&c);
            DSPSetInt16(DPARAMS,0,(uint16_t)x);DSPSetInt16(DPARAMS,2,(uint16_t)y);DPARAMS[4] = c;
        }
    DOCUMENTATION
        Returns the pen position in Parameters:0,1 (X) and Parameters:2,3 (Y) and the pen colour in Parameter:4.

    FUNCTION 7 Frame Rect
        *DERROR = QDFrameRect(DSPGetInt16(DPARAMS,0));
    DOCUMENTATION
        Draws the one pixel outline of the Rect at Parameters:0,1 in the pen colour, inside the rectangle, clipped.

    FUNCTION 8 Paint Rect
        *DERROR = QDPaintRect(DSPGetInt16(DPARAMS,0));
    DOCUMENTATION
        Fills the Rect at Parameters:0,1 with the pen colour, clipped.

    FUNCTION 9 Erase Rect
        *DERROR = QDEraseRect(DSPGetInt16(DPARAMS,0),DPARAMS[2]);
    DOCUMENTATION
        Fills the Rect at Parameters:0,1 with the colour in Parameter:2 (the background), clipped.

    FUNCTION 10 Invert Rect
        *DERROR = QDInvertRect(DSPGetInt16(DPARAMS,0));
    DOCUMENTATION
        Inverts every pixel of the Rect at Parameters:0,1 (colour index XOR $FF, $0F or 1 depending on the mode),
        clipped.

