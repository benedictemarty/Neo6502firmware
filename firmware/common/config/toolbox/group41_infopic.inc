// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group41_infopic.inc
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      24th November 2026
//      Purpose :   Toolbox group 41, Infocom v6 pictures (T-164)
//
// ***************************************************************************************
// ***************************************************************************************

GROUP 41 Infocom Pictures
    FUNCTION 1 Picture Info
        {
            uint16_t w = 0,h = 0,f = 0;
            uint8_t what = 0;
            *DERROR = IPInfo(DPARAMS[0],DSPGetInt16(DPARAMS,1),&w,&h,&f,&what);
            DSPSetInt16(DPARAMS,0,w);DSPSetInt16(DPARAMS,2,h);DSPSetInt16(DPARAMS,4,f);DPARAMS[6] = what;
        }
    DOCUMENTATION
        Trinity (T-164). Picture number Parameters:1,2 of the Infocom v6 picture file (IBM .EG1 / .MG1 : 16 byte
        header, directory, LZW pixels) already open on file channel Parameter:0 (Function 3,4). Nothing is drawn.
        Returns in Parameters:0,1 the width, Parameters:2,3 the height (pixels of the file), Parameters:4,5 the
        flags of the directory entry (bit 0 : transparency, the transparent colour is in bits 12-15), Parameter:6
        bit 0 : the entry has pixel data (0 : a size only), bit 1 : the entry has a palette (MG1). The directory
        is read from the start of the file on each call. Errors : 1 bad channel or not a picture file, 2 storage
        error, 3 no such picture.

    FUNCTION 2 Draw Picture
        *DERROR = IPDraw(DPARAMS[0],DSPGetInt16(DPARAMS,1),(int16_t)DSPGetInt16(DPARAMS,3),(int16_t)DSPGetInt16(DPARAMS,5),DPARAMS[7],DPARAMS[8]);
    DOCUMENTATION
        Trinity (T-164). Decodes picture number Parameters:1,2 of the file open on channel Parameter:0 and draws it
        on the draw page (Function 5,11) with its top left corner at the signed screen position X Parameters:3,4,
        Y Parameters:5,6 ; whatever is outside the screen is not drawn. Modes with 4 or 8 bits per pixel only (mode
        2 is the target). The colours 0-15 of the file are written as they are. Options, Parameter:7 : bit 0 :
        divide the width by 2 (even columns kept, for the 640 pixel wide EGA pictures) ; bit 1 : if the picture
        is transparent do not write its transparent colour ; bit 2 : load the picture palette (MG1) in the palette
        entries 2 to n+1 (n at most 14 ; entries 0 and 1 are not changed) ; bit 3 : load the 16 standard EGA
        colours in the palette entries 0 to 15 (done before bit 2) ; bit 4 (T-165) : base colour, Parameter:8 : the colours 0-15 of the
        file are written as base + 0 to 15 and the palette of bit 2 goes to the entries base + 2 to base + n + 1 (bit 3 to base + 0 to 15),
        entries past 255 are not set ; the transparent colour is still the file's own index. Each picture can thus keep its own
        palette bank in the 256 entries of mode 0 and mode 4. The file is read from the address of the
        picture, piece by piece : only an 8 KB table is taken in the heap for the call (graphics memory $90 and
        the 6502 RAM are not touched). The 6502 is held while it decodes. Errors : 1 bad channel, not a picture
        file or mode not supported, 2 storage error, 3 no such picture, 4 entry without data, 5 no heap for the
        table, 6 corrupt or truncated data (what was decoded stays drawn).
