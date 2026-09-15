// ***************************************************************************************
// ***************************************************************************************
//
//		Name : 		dvi_video.h	
//		Author :	Paul Robson (paul@robsons.org.uk)
//		Date : 		20th November 2023
//		Reviewed :	No
//		Purpose :	Common DVI Video functions
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _DVI_VIDEO_h
#define _DVI_VIDEO_h

void HWClockChanged(void);  													// System clock changed (mode switch) : re-derive rates.
void SERClockChanged(void);
void SNDClockChanged(void);

#define DVI_TIMING dvi_timing_640x480p_60hz  										// Boot timing (mode 0)

#endif

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
