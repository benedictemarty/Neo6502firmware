// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      sound.h
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      21st November 2023
//      Reviewed :  No
//      Purpose :   Audio support for Neo6502
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _XSOUND_H
#define _XSOUND_H

#define SOUND_CHANNELS_MAX (4)
#define SOUND_QUEUE_SIZE (32)

#define SOUNDTYPE_SQUARE (0)
#define SOUNDTYPE_NOISE (1)

typedef struct _sound_queue_item {
	uint16_t frequency;
	uint16_t timeCS;
	int16_t slide;
	uint8_t  type;
	uint8_t volume;
} SOUND_QUEUE_ELEMENT;

typedef struct _sound_channel {
	uint8_t currentType;
	int  currentFrequency;
	int  currentSlide;
	int  currentVolume;
	int  fadeTarget;  															// F-12 : volume ramp (0-127), reached by fadeStep per 50 Hz tick
	int  fadeStep;  															// 0 = no ramp in progress
	bool isPlayingNote;
	int  tick50Remaining;
	int  queueCount;
	SOUND_QUEUE_ELEMENT queue[SOUND_QUEUE_SIZE];
} SOUND_CHANNEL;

typedef struct _sound_update {
	int frequency;
	int slide;
	int timeCS;
	int type;
	int volume;
} SOUND_UPDATE;

void SNDInitialise(void);
int SNDGetSampleFrequency(void);

int SNDGetChannelCount(void);
int16_t SNDGetNextSample(void);
void SNDUpdateSoundChannel(uint8_t channel,SOUND_CHANNEL *c);

void SNDManager(void);
void SNDMuteAllChannels(void);
void SNDResetAll(void);
uint8_t SNDResetChannel(int channel);
uint8_t SNDPlay(int channelID,SOUND_UPDATE *u);
uint8_t SNDSetChannelVolume(int channelID,int volume,int timeCS);  				// F-12 : volume 0-127 of the playing note, ramped over timeCS
int SNDGetChannelVolume(int channelID);  											// F-12 : current volume 0-127, -1 if bad channel
void SNDSetCreatorVolume(uint8_t channel,int volume);  								// F-12 : change the output level without restarting the wave
uint8_t SNDStreamStart(uint16_t address,uint16_t half,uint16_t rate,int volume);  	// T-79 : 8,11
void SNDStreamStop(void);  														// T-79 : 8,12
uint8_t SNDStreamStatus(uint16_t *underruns);  										// T-79 : 8,13
uint8_t SNDStreamFilled(uint8_t half);  											// T-79 : 8,14
void SNDStreamClockChanged(void);  												// T-79 : output rate changed
void SNDGetMasterVolume(uint8_t *volume,bool *mute);  								// T-86 : 8,15
uint8_t SNDSetMasterVolume(uint8_t volume,bool mute);  							// T-86 : 8,16
void SNDMasterKey(uint16_t usage);  												// T-86 : media key press (consumer usage)
void SNDStartup(void);
int SNDGetNoteCount(int channelID);

uint8_t SFXPlay(int channelID,int effect);
#endif

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
