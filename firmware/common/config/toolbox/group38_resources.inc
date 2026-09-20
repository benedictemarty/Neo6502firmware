// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group38_resources.inc
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      18th September 2026
//      Purpose :   Toolbox group 38, Resource Manager (ADR-01, F-46)
//
// ***************************************************************************************
// ***************************************************************************************

GROUP 38 Resource Manager
    FUNCTION 1 Open Resource File
        *DERROR = RSOpen(DPARAMS[0],DSPGetInt16(DPARAMS,1));
    DOCUMENTATION
        Toolbox (Trinity, T-12, ADR-01 of the fork). Opens on file channel Parameter:0 the resource file whose length-prefixed
        name is at Parameters:1,2 (format NR1, built by tools/mkres.py of the fork archive : a table of resources, each
        with a 4 character type, a 16 bit id, an offset and a size, then the data). One resource file is open at
        a time (opening another closes it) ; nothing is cached in the RP2040, the table is read from storage on
        each call. Errors : 1 bad channel or not a resource file, 2 storage error. Resources are numbered 1..count
        in table order.

    FUNCTION 2 Close Resource File
        *DERROR = RSClose();
    DOCUMENTATION
        Closes the resource file and its channel (error 1 if none is open).

    FUNCTION 3 Count Resources
        {
            uint8_t count = 0;
            *DERROR = RSCount(&count);
            DPARAMS[0] = count;
        }
    DOCUMENTATION
        Returns the number of resources of the open file in Parameter:0.

    FUNCTION 4 Find Resource
        {
            uint8_t index = 0;
            *DERROR = RSFind(DPARAMS,DSPGetInt16(DPARAMS,4),&index);
            DPARAMS[6] = index;
        }
    DOCUMENTATION
        Looks for the resource of type Parameters:0-3 (4 ASCII characters, padded with spaces) and id
        Parameters:4,5 ; returns its number in Parameter:6 (0 and error 3 if not found).

    FUNCTION 5 Get Resource Info
        {
            struct RSEntry e;
            *DERROR = RSInfo(DPARAMS[0],&e);
            if (*DERROR == 0) { memcpy(DPARAMS+1,e.type,4);DSPSetInt16(DPARAMS,5,e.id); }
        }
    DOCUMENTATION
        Returns the type of resource Parameter:0 in Parameters:1-4 and its id in Parameters:5,6 (error 3 if the
        number is out of range).

    FUNCTION 6 Get Resource Size
        {
            struct RSEntry e;
            *DERROR = RSInfo(DPARAMS[0],&e);
            if (*DERROR == 0) *((uint32_t *)(DPARAMS+1)) = e.size;
        }
    DOCUMENTATION
        Returns the size in bytes of resource Parameter:0 in Parameters:1-4 (32 bits).

    FUNCTION 7 Load Resource
        {
            uint16_t size = DSPGetInt16(DPARAMS,4);
            *DERROR = RSLoad(DPARAMS[0],DPARAMS[1],DSPGetInt16(DPARAMS,2),&size);
            DSPSetInt16(DPARAMS,6,size);
        }
    DOCUMENTATION
        Reads resource Parameter:0 into memory page Parameter:1 ($00 6502 RAM, $80/$81 video RAM, $90 graphics
        RAM ; pages of Function 12,2) at address Parameters:2,3, at most
        Parameters:4,5 bytes ; Parameters:6,7 receive the number of bytes read (the whole resource when it fits).
        Error 1 if the range leaves the page.

    FUNCTION 8 Use Font
        {
            uint16_t size = DSPGetInt16(DPARAMS,4);
            *DERROR = RSUseFont(DPARAMS[0],DPARAMS[1],DSPGetInt16(DPARAMS,2),&size);
            DSPSetInt16(DPARAMS,6,size);
        }
    DOCUMENTATION
        Load Resource (same parameters) followed by Function 32,15 Set Font on the loaded NF1 font : the
        resource becomes the QuickDraw font (error 1 if it is not a valid font).
