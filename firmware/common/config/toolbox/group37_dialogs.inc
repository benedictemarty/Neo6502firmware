// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group37_dialogs.inc
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      18th September 2026
//      Purpose :   Toolbox group 37, Dialog Manager (ADR-01, F-45)
//
// ***************************************************************************************
// ***************************************************************************************

GROUP 37 Dialog Manager
    FUNCTION 1 New Dialog
        {
            uint8_t id = 0,window = 0;
            *DERROR = DLNew(DSPGetInt16(DPARAMS,0),&id,&window);
            DPARAMS[2] = id;DPARAMS[3] = window;
        }
    DOCUMENTATION
        Toolbox (Trinity, T-12, ADR-01 of the fork). Builds a modal dialog from the descriptor at Parameters:0,1 in 6502 RAM
        and returns its number (1-2) in Parameter:2 and its window (group 34) in Parameter:3 (error 1 bad
        descriptor, 2 no dialog, window or control left). The descriptor is read in place and must stay intact :
        the screen Rect of the window, a flags byte (bit 0 title bar), the length-prefixed title, then up to 12
        items ended by a 0 byte, each : kind (1 button, 2 check box, 3 radio, 4 scroll bar, 5 text field, 6 static
        text), flags (bit 0 disabled, bit 1 default - fired by Return, drawn with a ring -, bit 2 cancel - fired
        by Escape), Rect relative to the content, a 16 bit maximum (scroll bar value / text field length) and the
        length-prefixed title, text or buffer (a text field reserves 1 + maximum bytes ; static text : one line per
        character 13). Items 1-5 are controls of group 36 (see Get Dialog Item) ; the dialog is drawn at once and
        again on its update events through Dialog Event. Keys go to the first enabled text field (Tab : next one).

    FUNCTION 2 Dispose Dialog
        *DERROR = DLDispose(DPARAMS[0]);
    DOCUMENTATION
        Closes dialog Parameter:0 : its window and controls are disposed, the windows it covered get an update.

    FUNCTION 3 Dialog Event
        {
            uint8_t item = 0,consumed = 0;
            *DERROR = DLEvent(DPARAMS[0],DSPGetInt16(DPARAMS,1),&item,&consumed);
            DPARAMS[3] = item;DPARAMS[4] = consumed;
        }
    DOCUMENTATION
        Gives dialog Parameter:0 the EventRecord (group 33) at Parameters:1,2. Returns in Parameter:3 the item that
        acted (1.. in descriptor order, 0 none) : a button released inside it, the default item on Return, the
        cancel item on Escape, a box toggled, a scroll bar moved, a text field clicked or edited ; and in
        Parameter:4 whether the event belonged to the dialog (1 : every mouse and key event while it is up, its
        own update - redrawn here - and activate events ; 0 : update/activate of other windows, timers, which
        the program still handles). The program keeps calling this for every event until it gets the item it waits
        for, then Dispose Dialog.

    FUNCTION 4 Get Dialog Item
        {
            uint8_t control = 0,kind = 0;
            *DERROR = DLGetItem(DPARAMS[0],DPARAMS[1],&control,&kind);
            DPARAMS[2] = control;DPARAMS[3] = kind;
        }
    DOCUMENTATION
        Returns in Parameter:2 the control (group 36 : Set/Get Control Value, Set Control Flags) behind item
        Parameter:1 of dialog Parameter:0 (0 for static text) and its kind in Parameter:3.

    FUNCTION 5 Draw Dialog
        *DERROR = DLDraw(DPARAMS[0]);
    DOCUMENTATION
        Redraws the controls and static texts of dialog Parameter:0 (after the program changed a text in the
        descriptor, for instance).

    FUNCTION 6 Alert
        {
            uint8_t id = 0,window = 0;
            *DERROR = DLAlert(DSPGetInt16(DPARAMS,0),DPARAMS[2],&id,&window);
            DPARAMS[3] = id;DPARAMS[4] = window;
        }
    DOCUMENTATION
        Opens an alert centred on the screen with the length-prefixed message at Parameters:0,1 (one line per
        character 13, 6x8 font) and, by Parameter:2, the buttons 0 OK, 1 OK and Cancel, 2 Yes and No ; returns the
        dialog number in Parameter:3 and its window in Parameter:4. Item 1 is the first button (OK / Yes, default,
        at the right), item 2 the second (Cancel / No, cancel), item 3 the message : run Dialog Event until item
        1 or 2, then Dispose Dialog. The message must stay intact while the alert is up.
