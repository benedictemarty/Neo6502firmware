// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group6_sprites.inc
//      Authors :   Paul Robson (paul@robsons.org.uk)
//                  Bill Auger
//      Date :      22nd November 2023
//      Reviewed :  No
//      Purpose :   Sprite dispatcher.
//
// ***************************************************************************************
// ***************************************************************************************

// ***************************************************************************************
//
//                                    Group 6 (Sprites)
//
// ***************************************************************************************

GROUP 6 Sprites
    FUNCTION 1 Sprite Reset
        SPRReset();
    DOCUMENTATION
        Reset the sprite system.

    FUNCTION 2 Sprite Set
        *DERROR = SPRUpdate(DPARAMS);
    DOCUMENTATION
        Set or update the sprite specified in Parameter:0.

        The parameters are : Sprite Number, X Low, X High, Y Low, Y High, Image, Flip and Anchor and Flags

        Bit 0 of flags specifies 32 bit sprites.

        Values that are $80 or $8080 are not updated.

        Trinity (T-114) : bit 6 of the anchor byte forces the sprite to be erased and drawn again even if nothing
        changed ; the rest of the byte is the anchor ($C0 = force, anchor unchanged). The image address is looked
        up again (image rewritten in place) and a sprite hidden by 6,3 is shown again at its last position.
        Note : the position is compared with the top left corner, so with an anchor other than 7 every call with
        a position redraws the sprite.

    FUNCTION 3 Sprite Hide
        SPRHide(DPARAMS);
    DOCUMENTATION
        Hide the sprite specified in Parameter:0.

    FUNCTION 4 Sprite Collision
        DPARAMS[0] = SPRCollisionCheck(DERROR,DPARAMS[0],DPARAMS[1],DPARAMS[2]);
    DOCUMENTATION
        Parameter:0 is non-zero if the distance is less than or equal to Parameter:2 between the center of the sprite with index specified in Parameter:0 and the center of the sprite with index specified in Parameter:1 .

    FUNCTION 5 Sprite Position
        *DERROR = SPRGetSpriteData(DPARAMS);
    DOCUMENTATION
        Deposit into Parameters:1..4, the screen coordinates of the sprite with the index specified in Parameter:0.

    FUNCTION 6 Sprite Draw Mode
        *DERROR = SPRSetDrawMode(DPARAMS[0]);
    DOCUMENTATION
        Trinity (T-111). Sets how sprites are drawn in 8 bit per pixel modes : Parameter:0 = 0 exclusive or into
        the high nibble of the pixels (upstream, default), 1 opaque. In opaque mode colour 0 of an image is
        transparent, the sprite with the lowest number is in front, and erasing a sprite clears the high nibble
        of its area then draws back the other sprites there, so it does not depend on what was in the layer.
        Sprites already drawn are erased and drawn again in the new mode. Kept by 6,1, set back to 0 by a reset
        (1,0). No effect in packed modes (1 and 4 bits per pixel). Error 1 if Parameter:0 is above 1.

    FUNCTION 7 Sprite Set Image
        *DERROR = SPRSetImage(DPARAMS);
    DOCUMENTATION
        Trinity (T-113). Gives sprite Parameter:0 an image of any size taken in graphics memory (page $90) :
        Parameters:1,2 its offset, Parameter:3 its width and Parameter:4 its height (1-255), Parameter:5 its bits per
        pixel (1, 2, 4 or 8 ; lines of whole bytes, leftmost pixel in the high bits), Parameters:6,7 the offset of a
        colour table of 2^bpp bytes ($FFFF : none). A pixel of value 0 is transparent ; any other value v is drawn
        with colour table[v] (or v without a table), of which the sprite layer keeps the low 4 bits (0 = transparent).
        A placed sprite keeps its anchor point and is drawn again. Use 6,2 with $80 as image to move it ; 6,2 with an
        image number goes back to the standard images. Error 1 for a bad sprite, size or bits per pixel, 2 if the
        image or the table does not fit in graphics memory.
