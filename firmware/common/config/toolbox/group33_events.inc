// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group33_events.inc
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      17th September 2026
//      Purpose :   Toolbox group 33, Event Manager (ADR-01, F-42)
//
// ***************************************************************************************
// ***************************************************************************************

GROUP 33 Event Manager
    FUNCTION 1 Init Events
        EVTInit(DSPGetInt16(DPARAMS,0));
    DOCUMENTATION
        Toolbox (bmarty fork, ADR-01). Starts the event manager with an empty queue : keyboard, mouse and timer
        events are then collected in one queue of 32 records, polled by Get Next Event. Parameters:0,1 is the mask
        of event types to collect (bit n-1 for type n, 0 = all). Event types : 1 key down, 2 key up, 3 auto key
        (repeat), 4 mouse down, 5 mouse up, 6 mouse move (consecutive moves are merged), 7 wheel, 8 timer,
        9 update (message = window to redraw, see group 34), 10 activate (message = window, message2 = 1 front / 0).
        An EventRecord is 8 bytes in 6502 RAM : what, message, message2, modifiers, X (2), Y (2) ; message is the
        ASCII code for keys (0 if none), the buttons after the change for mouse down/up, the signed delta for the
        wheel, the timer number for timers ; message2 is the key code for keys, the button that changed for mouse
        down/up with bit 7 set for a double click ; modifiers are the keyboard modifiers for keys and the buttons
        held for mouse events ; X,Y is the mouse position when the event was posted. The keyboard queue of group 2
        is unaffected. There is no blocking wait : API calls are synchronous and the hosts read their input between
        calls ; poll, or sleep with the interrupt tick (1,12) and WAI. The manager is stopped by DSP Reset.

    FUNCTION 2 Get Next Event
        {
            uint8_t r = EVTGetNext(DSPGetInt16(DPARAMS,0),DSPGetInt16(DPARAMS,2));
            if (r == 0xFF) *DERROR = 1; else DPARAMS[4] = r;
        }
    DOCUMENTATION
        Removes the oldest queued event whose type is in the mask Parameters:2,3 (0 = all) and writes it in the
        EventRecord at Parameters:0,1 ; Parameter:4 is 1 if an event was returned, 0 if a null event (type 0, with
        the mouse position) was written instead. Due timers post their event first. Error if the address is bad.

    FUNCTION 3 Event Available
        DPARAMS[2] = EVTAvailable(DSPGetInt16(DPARAMS,0));
    DOCUMENTATION
        Returns in Parameter:2 the number of queued events whose type is in the mask Parameters:0,1 (0 = all).

    FUNCTION 4 Flush Events
        EVTFlush(DSPGetInt16(DPARAMS,0));
    DOCUMENTATION
        Discards the queued events whose type is in the mask Parameters:0,1 (0 = all).

    FUNCTION 5 Set Timer
        *DERROR = EVTSetTimer(DPARAMS[0],DSPGetInt16(DPARAMS,1));
    DOCUMENTATION
        Programs timer Parameter:0 (0-3) to post a timer event every Parameters:1,2 ticks of 10 ms (0 stops it).
        Timer events are generated when the queue is polled, at most one per timer per poll.

    FUNCTION 6 Event Status
        {
            uint8_t q,o;
            EVTStatus(&q,&o);
            DPARAMS[0] = q;DPARAMS[1] = o;
        }
    DOCUMENTATION
        Returns the number of queued events in Parameter:0 and, in Parameter:1, 1 if events were lost because the
        queue was full since the last call (the flag is then cleared).

