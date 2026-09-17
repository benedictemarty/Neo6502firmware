// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group8_sound.inc
//      Authors :   Paul Robson (paul@robsons.org.uk)
//                  Bill Auger
//      Date :      22nd November 2023
//      Reviewed :  No
//      Purpose :   Sound dispatcher
//
// ***************************************************************************************
// ***************************************************************************************

// ***************************************************************************************
//
//                                  Group 8 (Sound)
//
// ***************************************************************************************

GROUP 8 Sound
    FUNCTION 1 Reset Sound
        SNDResetAll();
    DOCUMENTATION
        Reset the sound system. This empties all channel queues and silences all channels immediately.

    FUNCTION 2 Reset Channel
        *DERROR = SNDResetChannel(*DPARAMS);
    DOCUMENTATION
        Reset the sound channel specified in Parameter:0.

    FUNCTION 3 Beep
        SNDStartup();
    DOCUMENTATION
        Play the startup beep immediately.

    FUNCTION 4 Queue Sound
        su.frequency = DSPGetInt16(DCOMMAND,5);
        su.timeCS = DSPGetInt16(DCOMMAND,7);
        su.slide = DSPGetInt16(DCOMMAND,9);
        su.type = 0;
        su.volume = 100;
        *DERROR = SNDPlay(DPARAMS[0],&su);
    DOCUMENTATION
        Queue a sound. Refer to Section \#\ref{sound} "Sound" for details.

        The parameters are : Channel, Frequency Low, Frequency High, Duration Low, Duration High, Slide Low, Slide High and Source.

    FUNCTION 5 Play Sound
        *DERROR = SFXPlay(DPARAMS[0],DPARAMS[1]);
    DOCUMENTATION
        Play the sound effect specified in Parameter:1 on the channel specified in Parameter:0 immediately, clearing the channel queue.

    FUNCTION 6 Sound Status
        i1 = SNDGetNoteCount(DPARAMS[0]);
        DPARAMS[0] = i1 & 0xFF;
        *DERROR = (i1 < 0) ? 1 : 0;
    DOCUMENTATION
        Deposit in Parameter:0 the number of notes outstanding before silence in the queue of the channel specified in Parameter:0, including the current playing sound, if any.

    FUNCTION 7 Queue Sound Extended
        su.frequency = DSPGetInt16(DCOMMAND,5);
        su.timeCS = DSPGetInt16(DCOMMAND,7);
        su.slide = DSPGetInt16(DCOMMAND,9);
        su.type = DCOMMAND[11];
        su.volume = DCOMMAND[12] * 127 / 100;
        *DERROR = SNDPlay(DPARAMS[0],&su);
    DOCUMENTATION
        Queue a sound. Refer to Section \#\ref{sound} "Sound" for details. This is an extension of call 4 to support different waveform types and volumes. The source parameter is no longer used.

        The parameters are : Channel, Frequency Low, Frequency High, Duration Low, Duration High, Slide Low, Slide High, Sound Type and Sound Volume. All these 
        are 16 bit parameters except the sound type and volume, and the channel number.

    FUNCTION 8 Get Channel Count
        DPARAMS[0] = SNDGetChannelCount();
    DOCUMENTATION
        This returns the number of channels in Parameter #0

    FUNCTION 9 Set Channel Volume
        *DERROR = SNDSetChannelVolume(DPARAMS[0],DPARAMS[1] * 127 / 100,DSPGetInt16(DPARAMS,2));
    DOCUMENTATION
        Sets the volume of the note currently playing on the channel in Parameter:0 to Parameter:1 (0-100), reached
        in Parameters:2,3 hundredths of a second (0 = at once) by a linear ramp (bmarty fork, F-12) : fade in, fade out
        or release without restarting the wave. The notes still queued keep the volume given when they were queued,
        and a new note cancels the ramp. Sets the error flag if the channel does not exist or is silent.

    FUNCTION 10 Get Channel Volume
        {
            int v = SNDGetChannelVolume(DPARAMS[0]);
            *DERROR = (v < 0) ? 1 : 0;
            DPARAMS[1] = (v < 0) ? 0 : (v * 100 + 63) / 127;
        }
    DOCUMENTATION
        Returns in Parameter:1 the current volume (0-100) of the channel in Parameter:0, 0 when silent (bmarty fork, F-12).
        Sets the error flag if the channel does not exist.

