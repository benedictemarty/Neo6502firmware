// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group34_windows.inc
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      17th September 2026
//      Purpose :   Toolbox group 34, Window Manager (ADR-01, F-43)
//
// ***************************************************************************************
// ***************************************************************************************

GROUP 34 Window Manager
    FUNCTION 1 New Window
        {
            uint8_t id = 0;
            *DERROR = WMNewWindow(DSPGetInt16(DPARAMS,0),DSPGetInt16(DPARAMS,2),DPARAMS[4],&id);
            DPARAMS[5] = id;
        }
    DOCUMENTATION
        Toolbox (bmarty fork, ADR-01). Creates a visible window whose frame is the Rect at Parameters:0,1 (screen
        coordinates, see group 32) with the length-prefixed title at Parameters:2,3 (31 characters at most) and
        the flags of Parameter:4 (bit 0 title bar, bit 1 close box, bit 2 grow box) ; returns its number (1-8) in
        Parameter:5, or an error (1 bad parameters, 2 no window left). The new window comes to the front. The
        firmware draws the frames (1 pixel border, 10 line title bar, close box, grow box) ; the content is drawn by
        the program : after every change of the window set the firmware erases the visible contents, repaints the
        frames and posts one update event (group 33, type 9) per visible window, back to front, and activate events
        (type 10). To handle an update : Set Port (34,12), draw with group 32, then End Update (34,11).

    FUNCTION 2 Dispose Window
        *DERROR = WMDisposeWindow(DPARAMS[0]);
    DOCUMENTATION
        Removes window Parameter:0 from the screen and frees its number.

    FUNCTION 3 Show Window
        *DERROR = WMShowWindow(DPARAMS[0],DPARAMS[1]);
    DOCUMENTATION
        Shows (Parameter:1 non zero) or hides window Parameter:0.

    FUNCTION 4 Select Window
        *DERROR = WMSelectWindow(DPARAMS[0]);
    DOCUMENTATION
        Brings window Parameter:0 to the front (and shows it).

    FUNCTION 5 Move Window
        *DERROR = WMMoveWindow(DPARAMS[0],(int16_t)DSPGetInt16(DPARAMS,1),(int16_t)DSPGetInt16(DPARAMS,3));
    DOCUMENTATION
        Moves the top left corner of the frame of window Parameter:0 to Parameters:1,2 (X) and Parameters:3,4 (Y).
        Dragging is done by the program from mouse events : Find Window says when the title bar is hit.

    FUNCTION 6 Size Window
        *DERROR = WMSizeWindow(DPARAMS[0],(int16_t)DSPGetInt16(DPARAMS,1),(int16_t)DSPGetInt16(DPARAMS,3));
    DOCUMENTATION
        Sets the frame of window Parameter:0 to Parameters:1,2 pixels wide and Parameters:3,4 high (3 at least).

    FUNCTION 7 Find Window
        {
            uint8_t id,part;
            WMFindWindow((int16_t)DSPGetInt16(DPARAMS,0),(int16_t)DSPGetInt16(DPARAMS,2),&id,&part);
            DPARAMS[4] = id;DPARAMS[5] = part;
        }
    DOCUMENTATION
        Returns in Parameter:4 the frontmost visible window under the point Parameters:0,1 (X), Parameters:2,3 (Y)
        (0 if none) and in Parameter:5 the part hit : 1 content, 2 title bar or border (drag), 3 close box, 4 grow box.

    FUNCTION 8 Get Content Rect
        *DERROR = WMGetContentRect(DPARAMS[0],DSPGetInt16(DPARAMS,1));
    DOCUMENTATION
        Writes the content rectangle of window Parameter:0 (screen coordinates, inside the frame and below the title
        bar) into the Rect at Parameters:1,2.

    FUNCTION 9 Set Window Title
        *DERROR = WMSetTitle(DPARAMS[0],DSPGetInt16(DPARAMS,1));
    DOCUMENTATION
        Changes the title of window Parameter:0 to the length-prefixed string at Parameters:1,2 and repaints its frame.

    FUNCTION 10 Front Window
        DPARAMS[0] = WMFrontWindow();
    DOCUMENTATION
        Returns the frontmost visible window in Parameter:0 (0 if none).

    FUNCTION 11 End Update
        *DERROR = WMEndUpdate(DPARAMS[0]);
    DOCUMENTATION
        To call after redrawing the content of window Parameter:0 : repaints the frames of the windows above it,
        which the content drawing may have overwritten (contents are clipped to the content rectangle only).

    FUNCTION 12 Set Port
        *DERROR = WMSetPort(DPARAMS[0]);
    DOCUMENTATION
        Sets the QuickDraw clip rectangle (group 32) to the content rectangle of window Parameter:0.

