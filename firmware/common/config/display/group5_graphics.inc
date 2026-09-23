// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group5_graphics.inc
//      Authors :   Paul Robson (paul@robsons.org.uk)
//                  Bill Auger
//      Date :      22nd November 2023
//      Reviewed :  No
//      Purpose :   Graphics dispatcher.
//
// ***************************************************************************************
// ***************************************************************************************

// ***************************************************************************************
//
// Group 5 (Graphics)
//
// ***************************************************************************************

GROUP 5 Graphics
    FUNCTION 1 Set Defaults
        GFXSetDefaults(DCOMMAND);
    DOCUMENTATION
        Configure the global graphics system settings.
        Not all parameters are relevant for all graphics commands; but all parameters will be set by this command. So mind their values.
        Refer to Section "Graphics Settings" for details.

        The parameters are And, Or, Fill Flag, Extent, and Flip. Bit 0 of flip sets the horizontal flip, Bit 1 sets the vertical flip.

    FUNCTION 2 Draw Line
        GFXGraphicsCommand(2,DCOMMAND);
    DOCUMENTATION
        Draw a line between the screen coordinates specified in  Parameters:0,1,Parameters:2,3 (begin X,Y) and Parameters:4,5,Parameters:6,7 (end X,Y).

    FUNCTION 3 Draw Rectangle
        GFXGraphicsCommand(3,DCOMMAND);
    DOCUMENTATION
        Draw a rectangle spanning the screen coordinates specified in  Parameters:0,1,Parameters:2,3 (corner X,Y) and Parameters:4,5,Parameters:6,7 (opposite corner X,Y).

    FUNCTION 4 Draw Ellipse
        GFXGraphicsCommand(4,DCOMMAND);
    DOCUMENTATION
        Draw an ellipse spanning the screen coordinates specified in  Parameters:0,1,Parameters:2,3 (corner  X,Y) and Parameters:4,5,Parameters:6,7 (opposite corner X,Y).

    FUNCTION 5 Draw Pixel
        GFXGraphicsCommand(5,DCOMMAND);
    DOCUMENTATION
        Draw a single pixel at the screen coordinates specified in Parameters:0,1,Parameters:2,3 (X,Y).

    FUNCTION 6 Draw Text
        GFXGraphicsCommand(6,DCOMMAND);
    DOCUMENTATION
        Draw the length-prefixed string of text stored at the memory location specified in Parameters:4,5 at the screen character cell specified in Parameters:0,1,Parameters:2,3 (X,Y).

    FUNCTION 7 Draw Image
        GFXGraphicsCommand(7,DCOMMAND);
    DOCUMENTATION
        Draw the image with image ID in Parameter:4 at the screen coordinates Parameters:0,1,Parameters:2,3 (X,Y). The extent and flip settings influence this command.

    FUNCTION 8 Draw Tilemap
        *DERROR = TMPDrawTileMap(DCOMMAND);
    DOCUMENTATION
        Draw the current tilemap at the screen coordinates specified in Parameters:0,1,Parameters:2,3 (top-left X,Y) and Parameters:4,5,Parameters:6,7 (bottom-right X,Y) using current graphics settings.

    FUNCTION 9 Set Graphics Mode
        *DERROR = GFXSetMode(DPARAMS[0]);
    DOCUMENTATION
        Select the display mode in Parameter:0 and reinitialise the screen, palette and console (F5 / ADR-02).
        0 is the standard 320x240 256 colour mode. 1 is the Hercules mode (720x350 monochrome, 80x25 text in 9x14 cells).
        2 is 320x256 in 16 colours (40x32 text in 8x8 cells). Sprites, tilemaps and images are only available in mode 0.
        Sets the error flag if the mode is unknown or not supported by this hardware.

    FUNCTION 10 Get Graphics Mode
        DPARAMS[0] = GFXGetMode();DPARAMS[1] = gMode.xGSize & 0xFF;DPARAMS[2] = gMode.xGSize >> 8;
        DPARAMS[3] = gMode.yGSize & 0xFF;DPARAMS[4] = gMode.yGSize >> 8;DPARAMS[5] = gMode.bitsPerPixel;
        DPARAMS[6] = gMode.xCSize;DPARAMS[7] = gMode.yCSize;
    DOCUMENTATION
        Return the current display mode in Parameter:0, the pixel width in Parameters:1,2, the pixel height in Parameters:3,4,
        the bits per pixel in Parameter:5 and the console size in characters in Parameters:6,7.

    FUNCTION 11 Set Draw Page
        *DERROR = GFXSetDrawPage(DPARAMS[0]);
    DOCUMENTATION
        Select the page (Parameter:0) that console and graphics commands draw into (F-55).
        Mode 0 has one page, modes 1 and 2 have two (0 and 1). Sets the error flag if the page does not exist.

    FUNCTION 12 Set Display Page
        *DERROR = GFXSetDisplayPage(DPARAMS[0]);
    DOCUMENTATION
        Select the page (Parameter:0) shown on screen from the next frame (F-55). Use Function 37 (Frame Count) to wait for the swap.
        Sets the error flag if the page does not exist.

    FUNCTION 32 Set Palette
        GFXSetPalette(DPARAMS[0],DPARAMS[1],DPARAMS[2],DPARAMS[3]);
    DOCUMENTATION
        Set the palette colour at the index spcified in Parameter:0 to the values in Parameter:1,Parameter:2,Parameter:3 (RGB).

    FUNCTION 33 Read Pixel
        GFXGraphicsCommand(33,DCOMMAND);
    DOCUMENTATION
        Read a single pixel at the screen coordinates specified in Parameters:0,1,Parameters:2,3 (X,Y).
        When the routine completes, the result will be in Parameter:0. If sprites are in use, this will be the background only (0..15), if sprites are not in use it may return (0..255)

    FUNCTION 34 Reset Palette
        GFXDefaultPalette();
    DOCUMENTATION
        Reset the palette to the defaults.

    FUNCTION 35 Set Tilemap
        TMPSelectTileMap(cpuMemory+DSPGetInt16(DCOMMAND,4),DSPGetInt16(DCOMMAND,6),DSPGetInt16(DCOMMAND,8));
    DOCUMENTATION
        Set the current tilemap.
        Parameters:0,1 is the memory address of the tilemap, and Parameters:2,3,Parameters:4,5 (X,Y) specifies the offset into the tilemap, in units of pixels, of the top-left pixel of the tile.

    FUNCTION 36 Read Sprite Pixel
        GFXGraphicsCommand(36,DCOMMAND);
    DOCUMENTATION
        Read Pixel from the sprite layer at the screen coordinates specified in Parameters:0,1,Parameters:2,3 (X,Y).
        When the routine completes, the result will be in Parameter:0.
        Refer to Section "Pixel Colors" for details.

    FUNCTION 37 Frame Count
        *((uint32_t *)DPARAMS) = RNDGetFrameCount();
    DOCUMENTATION
        Deposit into Parameters:0..3, the number of v-blanks (full screen redraws) which have occurred since power-on. This is updated at the start of each v-blank period.

    FUNCTION 38 Get Palette
        GFXGetPalette(DPARAMS[0],&DPARAMS[1],&DPARAMS[2],&DPARAMS[3]);
    DOCUMENTATION
        Get the palette colour at the index spcified in Parameter:0. Values are returned in Parameter:1,Parameter:2,Parameter:3 (RGB).

    FUNCTION 39 Write Pixel
        GFXSetDrawColour(DPARAMS[4]);GFXGraphicsCommand(5,DCOMMAND);
    DOCUMENTATION
        Write Pixel index Parameter:4 to the screen coordinate specified in Parameters:0,1,Parameters:2,3 (X,Y).

    FUNCTION 64 Set Color
        GFXSetDrawColour(DPARAMS[0]);
    DOCUMENTATION
        Set Color
        Sets the current drawing colour to Parameter:0

    FUNCTION 65 Set Solid Flag
        GFXSetSolidFlag(DPARAMS[0]);
    DOCUMENTATION
        Set Solid Flag

        Sets the solid flag to Parameter:0, which indicates either solid fill (for shapes) or solid background (for images and fonts)

    FUNCTION 66 Set Draw Size
        GFXSetDrawSize(DPARAMS[0]);
    DOCUMENTATION
        Set Draw Size

        Sets the drawing scale for images and fonts to Parameter:0

    FUNCTION 67 Set Flip Bits
        GFXSetFlipBits(DPARAMS[0]);
    DOCUMENTATION
        Set Flip Bits
        Sets the flip bits for drawing images. Bit 0 set causes a horizontal flip, bit 1 set causes a vertical flip.

    FUNCTION 42 Get Bus Timing
        {
            uint32_t v = HWBusTiming(DPARAMS[0]);
            DPARAMS[0] = v & 0xFF;DPARAMS[1] = (v >> 8) & 0xFF;DPARAMS[2] = (v >> 16) & 0xFF;DPARAMS[3] = v >> 24;
        }
    DOCUMENTATION
        Returns in Parameters:0-3 a bus timing measure of Trinity T-50, chosen by Parameter:0 on entry : 0 the
        longest DSPSync ever seen in microseconds (keyboard, USB task, blink), 1 the longest API command
        (DSPHandler). Both are the time the firmware spends away from the 6502 bus : while it is there the PIO
        stalls and the processor clock stops, which is normal for an API call and pathological when it lasts.
        Counts of stalls alone (Function 5,41) cannot tell the two apart, hence these. Reset with 5,41 and $FF.
        0 in the emulators.

    FUNCTION 41 Get Bus Stalls
        {
            if (DPARAMS[0] == 0xFF) HWBusStallsReset();
            uint32_t t = HWBusStalls(0),r = HWBusStalls(1);
            DPARAMS[0] = t & 0xFF;DPARAMS[1] = (t >> 8) & 0xFF;DPARAMS[2] = (t >> 16) & 0xFF;DPARAMS[3] = t >> 24;
            DPARAMS[4] = r & 0xFF;DPARAMS[5] = (r >> 8) & 0xFF;DPARAMS[6] = (r >> 16) & 0xFF;DPARAMS[7] = r >> 24;
        }
    DOCUMENTATION
        Returns how often the firmware was too slow to serve the 6502 bus (Trinity T-49, diagnostic ; 0 in the
        emulators, which have no PIO). Parameters:0-3 count the sampling windows where the data of a read was not
        ready in time (PIO TXSTALL), Parameters:4-7 those where the address FIFO was not drained in time (RXSTALL).
        The PIO state machine generates the 6502 clock, so a stall stretches the processor rather than feeding it a
        wrong byte. Sampled about 95 times a second, so these are windows containing at least one stall, not exact
        counts : 0 means a healthy binary, anything growing means a starved firmware. Pass $FF in Parameter:0 to
        reset both counters before measuring.

    FUNCTION 40 Get Late Scanlines
        {
            uint32_t n = RNDLateScanlines();
            DPARAMS[0] = n & 0xFF;DPARAMS[1] = (n >> 8) & 0xFF;DPARAMS[2] = (n >> 16) & 0xFF;DPARAMS[3] = n >> 24;
        }
    DOCUMENTATION
        Returns in Parameters:0-3 the number of scanlines the display could not encode in time since the last mode
        change (Trinity T-32c, Function 5,40, diagnostic : these show as coloured streaks on the board ; 0 in the emulators). A
        counter that keeps growing while a program runs means the second core is starved.
