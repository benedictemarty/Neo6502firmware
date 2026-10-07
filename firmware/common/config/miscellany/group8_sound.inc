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
        in Parameters:2,3 hundredths of a second (0 = at once) by a linear ramp (Trinity T-22, F-12 of the fork) : fade in, fade out
        or release without restarting the wave. The notes still queued keep the volume given when they were queued,
        and a new note cancels the ramp. Sets the error flag if the channel does not exist or is silent.

    FUNCTION 10 Get Channel Volume
        {
            int v = SNDGetChannelVolume(DPARAMS[0]);
            *DERROR = (v < 0) ? 1 : 0;
            DPARAMS[1] = (v < 0) ? 0 : (v * 100 + 63) / 127;
        }
    DOCUMENTATION
        Returns in Parameter:1 the current volume (0-100) of the channel in Parameter:0, 0 when silent (Trinity T-22, F-12 of the fork).
        Sets the error flag if the channel does not exist.


    FUNCTION 11 Stream Start
        *DERROR = (DPARAMS[6] > 100) ? 1 : SNDStreamStart(DSPGetInt16(DPARAMS,0),DSPGetInt16(DPARAMS,2),DSPGetInt16(DPARAMS,4),DPARAMS[6] * 127 / 100);
    DOCUMENTATION
        Starts playing a stream of 8 bit unsigned PCM samples ($80 = silence) mixed with the four channels (Trinity T-79).
        The buffer is in 6502 RAM at Parameters:0,1 and made of two halves of Parameters:2,3 bytes each, one after the
        other ; Parameters:4,5 is the rate of the samples in Hz (11025 or 22050 for instance, the firmware follows it
        whatever the output rate of the board) and Parameter:6 the volume (0-100, as Function 8,9). Fill both halves
        before the call : they are taken as full. The firmware plays half 0 then half 1 then half 0 again..., handing
        each half back when it has played it (Function 8,13) ; the program refills it and marks it full (Function 8,14).
        A half not refilled in time is not replayed : the stream is silent until it is marked full, and the underrun is
        counted. The stream counts as one more channel for the volume scaling. Sets the error flag if a size or the rate
        is 0, the volume above 100, or the buffer does not end below $FF00. Function 8,1 stops the stream as well.

    FUNCTION 12 Stream Stop
        SNDStreamStop();
    DOCUMENTATION
        Stops the stream at once (Trinity T-79). The buffer is no longer read and may be reused.

    FUNCTION 13 Stream Status
        {
            uint16_t u = 0;
            DPARAMS[0] = SNDStreamStatus(&u);
            DSPSetInt16(DPARAMS,1,u);
        }
    DOCUMENTATION
        Returns in Parameter:0 the state of the stream (Trinity T-79) : bit 7 set while it plays, bit 0 set when half 0
        has been played and waits to be refilled, bit 1 the same for half 1 ; 0 when stopped. Parameters:1,2 count the
        underruns since the start (a half reached before it was marked full, 65535 at most).

    FUNCTION 14 Stream Filled
        *DERROR = SNDStreamFilled(DPARAMS[0]);
    DOCUMENTATION
        Marks half Parameter:0 (0 or 1) of the stream as refilled (Trinity T-79), after the program has written its
        samples. Sets the error flag if the half is not 0 or 1 or the stream is stopped.

    FUNCTION 15 Get Master Volume
        {
            uint8_t v;bool m;
            SNDGetMasterVolume(&v,&m);
            DPARAMS[0] = v;DPARAMS[1] = m ? 1 : 0;
        }
    DOCUMENTATION
        Returns the master volume (0-16, 16 at reset) in Parameter:0 and 1 in Parameter:1 if the sound is muted (Trinity
        T-86). The master volume scales everything the sound system outputs, the four channels and the stream. The Mute,
        Volume - and Volume + media keys of a USB keyboard change it : Mute toggles, each Volume key moves the volume by 2
        and unmutes. The keys are still returned by Function 2,22.

    FUNCTION 16 Set Master Volume
        *DERROR = SNDSetMasterVolume(DPARAMS[0],DPARAMS[1] != 0);
    DOCUMENTATION
        Sets the master volume to Parameter:0 (0-16) and mutes the sound if Parameter:1 is not zero (Trinity T-86). Sets
        the error flag if the volume is above 16.

    FUNCTION 17 Stream Start Graphics
        *DERROR = (DPARAMS[6] > 100) ? 1 : SNDStreamStartGraphics(DSPGetInt16(DPARAMS,0),DSPGetInt16(DPARAMS,2),DSPGetInt16(DPARAMS,4),DPARAMS[6] * 127 / 100,(DPARAMS[7] & 1) != 0);
    DOCUMENTATION
        Plays a sample of 8 bit unsigned PCM ($80 = silence) held in graphics memory, mixed with the four channels
        (Trinity T-119). Parameters:0,1 is its offset in graphics memory, Parameters:2,3 its length in bytes, Parameters:4,5
        its rate in Hz and Parameter:6 the volume (0-100) ; bit 0 of Parameter:7 set plays it again and again, clear plays
        it once. The 6502 has nothing to do while it plays : Function 8,13 returns $80 until a sample played once is over,
        then 0 ; Function 8,14 is refused. It replaces a stream started by Function 8,11 and is stopped by Function 8,12 or
        8,1. Sets the error flag if the length or the rate is 0, the volume above 100, or the sample goes past the end of
        graphics memory.
