// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group36_controls.inc
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      18th September 2026
//      Purpose :   Toolbox group 36, Control Manager (ADR-01, F-44)
//
// ***************************************************************************************
// ***************************************************************************************

GROUP 36 Control Manager
    FUNCTION 1 New Control
        {
            uint8_t id = 0;
            *DERROR = CTNew(DPARAMS[0],DPARAMS[1],DSPGetInt16(DPARAMS,2),DSPGetInt16(DPARAMS,4),DSPGetInt16(DPARAMS,6),&id);
            DPARAMS[0] = id;
        }
    DOCUMENTATION
        Toolbox (Trinity, T-12, ADR-01 of the fork). Creates a control of kind Parameter:0 (1 button, 2 check box, 3 radio button,
        4 scroll bar - vertical when higher than wide -, 5 text field) in window Parameter:1, with the Rect at
        Parameters:2,3 relative to the content of the window, the length-prefixed title (or, for a text field,
        the text buffer) at Parameters:4,5 read in place, and Parameters:6,7 = maximum value of a scroll bar or
        maximum length of a text field ; returns the control number (1-24) in Parameter:0 (error 1 bad parameters,
        2 none left). The control is drawn at once ; redraw all the controls of a window with Draw Controls when
        handling its update event. Radio buttons are not grouped : the program clears the others.

    FUNCTION 2 Dispose Control
        *DERROR = CTDispose(DPARAMS[0]);
    DOCUMENTATION
        Erases and frees control Parameter:0. Disposing a window frees its controls.

    FUNCTION 3 Draw Controls
        *DERROR = CTDrawAll(DPARAMS[0]);
    DOCUMENTATION
        Draws every control of window Parameter:0 (to call after Set Port in the update handler).

    FUNCTION 4 Draw Control
        *DERROR = CTDraw(DPARAMS[0]);
    DOCUMENTATION
        Redraws control Parameter:0.

    FUNCTION 5 Set Control Value
        *DERROR = CTSetValue(DPARAMS[0],(int16_t)DSPGetInt16(DPARAMS,1));
    DOCUMENTATION
        Sets the value of control Parameter:0 to Parameters:1,2 (0/1 for boxes, 0..max for scroll bars) and redraws it.

    FUNCTION 6 Get Control Value
        {
            int16_t v = 0;
            *DERROR = CTGetValue(DPARAMS[0],&v);
            DSPSetInt16(DPARAMS,1,(uint16_t)v);
        }
    DOCUMENTATION
        Returns the value of control Parameter:0 in Parameters:1,2.

    FUNCTION 7 Set Control Flags
        *DERROR = CTSetFlags(DPARAMS[0],DPARAMS[1]);
    DOCUMENTATION
        Sets the flags of control Parameter:0 (bit 0 disabled : drawn grey, ignores tracking and keys) and redraws it.

    FUNCTION 8 Find Control
        {
            uint8_t id,part;
            CTFind((int16_t)DSPGetInt16(DPARAMS,0),(int16_t)DSPGetInt16(DPARAMS,2),&id,&part);
            DPARAMS[4] = id;DPARAMS[5] = part;
        }
    DOCUMENTATION
        Returns in Parameter:4 the control of the frontmost window under Parameters:0,1 (X), Parameters:2,3 (Y)
        (0 if none) and in Parameter:5 the part : 1 body ; scroll bars : 2 first arrow, 3 second arrow, 4 page
        before the thumb, 5 page after, 6 thumb.

    FUNCTION 9 Track Control
        {
            uint8_t acted = 0;
            *DERROR = CTTrack(DPARAMS[0],(int16_t)DSPGetInt16(DPARAMS,1),(int16_t)DSPGetInt16(DPARAMS,3),DPARAMS[5],&acted);
            DPARAMS[6] = acted;
        }
    DOCUMENTATION
        Tracks control Parameter:0 from the mouse : Parameters:1,2 (X), Parameters:3,4 (Y), Parameter:5 = phase
        (0 mouse down, 1 move, 2 mouse up). Parameter:6 is 1 when the control acted : button released inside it
        (highlighted while the pointer is inside), box toggled or radio set on mouse up, scroll bar value changed
        (arrows by 1, pages by max/10, thumb dragged), text field clicked (give it the keys with Key Control).

    FUNCTION 10 Key Control
        {
            uint8_t changed = 0;
            *DERROR = CTKey(DPARAMS[0],DPARAMS[1],&changed);
            DPARAMS[2] = changed;
        }
    DOCUMENTATION
        Sends the key Parameter:1 to text field Parameter:0 : printable characters are appended to its buffer (up
        to the maximum length), 8 or 127 deletes the last one ; Parameter:2 is 1 if the buffer changed. The buffer
        is the length-prefixed string given to New Control, in 6502 RAM.

