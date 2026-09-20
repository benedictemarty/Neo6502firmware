// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group35_menus.inc
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      18th September 2026
//      Purpose :   Toolbox group 35, Menu Manager (ADR-01, F-44)
//
// ***************************************************************************************
// ***************************************************************************************

GROUP 35 Menu Manager
    FUNCTION 1 New Menu
        {
            uint8_t id = 0;
            *DERROR = MNNewMenu(DSPGetInt16(DPARAMS,0),&id);
            DPARAMS[2] = id;
        }
    DOCUMENTATION
        Toolbox (Trinity, T-12, ADR-01 of the fork). Adds a menu to the menu bar from the descriptor at Parameters:0,1 in 6502 RAM
        and returns its number (1-6) in Parameter:2 (error 2 if none left). The descriptor is read in place and must
        stay intact : a length-prefixed title, then up to 16 items, each a flags byte (bit 0 disabled, bit 1
        checked, bit 7 separator) followed by a length-prefixed text (31 characters at most), ended by a $FF byte.
        The menu bar (12 lines at the top of the screen, titles in the 6x8 font) is redrawn ; keep windows below it.

    FUNCTION 2 Draw Menu Bar
        MNDrawBar();
    DOCUMENTATION
        Redraws the menu bar (after the program drew over it, for instance).

    FUNCTION 3 Menu Select
        DPARAMS[4] = MNSelect((int16_t)DSPGetInt16(DPARAMS,0),(int16_t)DSPGetInt16(DPARAMS,2));
    DOCUMENTATION
        To call on a mouse down at Parameters:0,1 (X), Parameters:2,3 (Y) : if a menu title is hit, the menu is pulled
        down and its number returned in Parameter:4 (0 otherwise). The program then calls Menu Track on every mouse
        move and Menu Track End on the mouse up. Nothing is saved under the pull-down : when it closes, the area is
        erased and the windows it covered get an update event (group 33).

    FUNCTION 4 Menu Track
        MNTrack((int16_t)DSPGetInt16(DPARAMS,0),(int16_t)DSPGetInt16(DPARAMS,2));
    DOCUMENTATION
        Highlights the item under Parameters:0,1 (X), Parameters:2,3 (Y) in the pulled down menu ; moving over
        another title pulls that menu down instead.

    FUNCTION 5 Menu Track End
        {
            uint8_t m,i;
            MNTrackEnd(&m,&i);
            DPARAMS[0] = m;DPARAMS[1] = i;
        }
    DOCUMENTATION
        Closes the pulled down menu and returns the chosen menu in Parameter:0 and item in Parameter:1 (1..), both
        0 if nothing enabled was chosen.

    FUNCTION 6 Set Item Flags
        *DERROR = MNSetItemFlags(DPARAMS[0],DPARAMS[1],DPARAMS[2]);
    DOCUMENTATION
        Sets the flags (bit 0 disabled, bit 1 checked) of item Parameter:1 of menu Parameter:0 to Parameter:2, in the
        descriptor ; the item is redrawn if its menu is pulled down.

    FUNCTION 7 Dispose Menu
        *DERROR = MNDisposeMenu(DPARAMS[0]);
    DOCUMENTATION
        Removes menu Parameter:0 from the menu bar.

    FUNCTION 8 Get Item Flags
        {
            uint8_t f = 0;
            *DERROR = MNGetItemFlags(DPARAMS[0],DPARAMS[1],&f);
            DPARAMS[2] = f;
        }
    DOCUMENTATION
        Returns the flags of item Parameter:1 of menu Parameter:0 in Parameter:2.

