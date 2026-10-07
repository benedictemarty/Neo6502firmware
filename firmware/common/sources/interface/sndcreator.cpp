// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      sndcreator.cpp
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      7th August 2024
//      Reviewed :  No
//      Purpose :   Waveform generator, default sound system.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "interface/whitenoise.h"

#define CHANNEL_COUNT   (4)

struct _ChannelStatus {
    uint32_t step;                                                                  // Pitch : half periods per sample, 16.16
    uint32_t phase;                                                                 // A toggle each time it passes 1.0
    int state;
    int soundType;
    int volume;
    int samplePos;
    int output;                                                                     // T-80 : level held between two toggles
} audio[CHANNEL_COUNT];

//
//      T-79 : PCM stream mixed with the channels. The 6502 fills two halves of a buffer in its
//      own RAM (8 bit unsigned samples) and marks each one full ; this plays them at the
//      declared rate through a 16.16 phase accumulator and hands each half back once played.
//      A half not refilled in time is not replayed : the stream goes silent until it is.
//      The flags are single bytes, each written by one side at a time (no read-modify-write).
//      T-119 : 8,17 plays a sample from graphics memory instead, once or in a loop, as a single
//      'half' that is never handed back : the 6502 has nothing to do.
//
#define STREAM_HALVES   (0)                                                     // 8,11 : two halves refilled by the 6502
#define STREAM_ONCE     (1)                                                     // 8,17 : one sample, stops at its end
#define STREAM_LOOP     (2)                                                     // 8,17 : one sample, played again and again

static struct {
    bool on;
    uint8_t mode;                                                               // T-119 : STREAM_HALVES, _ONCE or _LOOP
    const uint8_t *mem;                                                         // T-119 : cpuMemory or gfxObjectMemory
    uint16_t base,half,rate;                                                    // Buffer (two halves of 'half' bytes), samples/s
    uint8_t volume;                                                             // 0-127, as the channels
    uint32_t step,phase;                                                        // Input samples per output sample, 16.16
    uint16_t pos;uint8_t cur;                                                   // Position in the current half
    volatile uint8_t full[2];                                                   // Half filled by the 6502, not yet played
    volatile uint16_t underruns;                                                // Times a half was reached before being filled
} stream;

//
//      T-86 : master volume (0-16, 16 = as before) and mute, applied to everything that goes out ;
//      set by the Mute / Volume - / Volume + media keys (T-39) and by 8,15 / 8,16.
//
#define MASTER_MAX  (16)
static uint8_t masterVolume = MASTER_MAX;
static bool masterMute = false;

// ***************************************************************************************
//
//            Return number of channels supported by this implementation
//
// ***************************************************************************************

int SNDGetChannelCount(void) {
    return CHANNEL_COUNT;
}

// ***************************************************************************************
//
//                                  Mute all channels
//
// ***************************************************************************************

void SNDMuteAllChannels(void) {    
    for (int i = 0;i < CHANNEL_COUNT;i++) {
        struct _ChannelStatus *cs = &audio[i];
        cs->step = cs->phase = 0;cs->state = cs->soundType = cs->volume = cs->output = 0;
    }
}

// ***************************************************************************************
//
//            	Get the next sample for the driver provided hardware rate.
//
// ***************************************************************************************

int16_t __time_critical_func(SNDGetNextSample)(void) {                      // T-79 : in RAM, the PWM interrupt calls it

    static int activeCount = 0;

    int level = 0;                                                                  // Summative limit
    int channelsActive = activeCount;                                               // We have a very simple form of AGC, more than one channel scales volume
    activeCount = 0;                                                                // Reset the count

    for (int i = 0;i < CHANNEL_COUNT;i++) {                                         // Scan the channels
        struct _ChannelStatus *cs = &audio[i];                                          
        if (cs->volume != 0) {                                                      // Channel on.
            activeCount++;                                                          // Bump active count
            cs->phase += cs->step;                                                  // Pitch in 16.16 : the note is exact on
            if (cs->phase >= 0x10000) {                                             // average, not rounded to whole samples.
                cs->phase -= 0x10000;
                cs->state ^= 0xFF;
                switch (cs->soundType) {
                    case SOUNDTYPE_NOISE:   
                        cs->output = ((noisePattern[cs->samplePos] - 0x80) * cs->volume / 128);
                        if (++cs->samplePos == NOISE_SIZE) cs->samplePos = 0;
                        break;
                    default:                                                        // Square wave
                        cs->output = cs->state ? cs->volume : -cs->volume;break;
                }
            }
            level += cs->output;                                                    // T-80 : every sample, not only on a toggle
        }
	}      
    if (stream.on) {                                                            // T-79 : the stream counts as one more channel
        activeCount++;
        if (stream.full[stream.cur]) {
            const uint8_t *mem = stream.mem;
            uint16_t at = stream.base + stream.cur * stream.half + stream.pos;
            int a = mem[at],b = a;                                              // Linear interpolation to the next sample :
            if (stream.pos + 1 < stream.half) b = mem[at + 1];                  // holding each one (1 or 2 outputs at 22050
            else if (stream.mode == STREAM_LOOP) b = mem[stream.base];          // -> 30882 Hz) put images at -34 dB around
            else if (stream.mode == STREAM_HALVES && stream.full[stream.cur ^ 1])   // 9 kHz, bmarty heard them (T-79)
                b = mem[stream.base + (stream.cur ^ 1) * stream.half];
            int v = a + (((b - a) * (int)(stream.phase >> 8)) >> 8);
            level += (v - 128) * stream.volume / 128;
            stream.phase += stream.step;
            while (stream.phase >= 0x10000) {
                stream.phase -= 0x10000;
                if (++stream.pos >= stream.half) {                              // Half played : hand it back, go to the other
                    stream.pos = 0;
                    if (stream.mode == STREAM_LOOP) continue;                   // T-119 : sample again from its start
                    if (stream.mode == STREAM_ONCE) {                           // T-119 : sample over, the stream stops
                        stream.on = false;stream.full[0] = 0;
                        break;
                    }
                    stream.full[stream.cur] = 0;stream.cur ^= 1;
                    if (!stream.full[stream.cur]) {
                        if (stream.underruns != 0xFFFF) stream.underruns = stream.underruns + 1;
                        break;
                    }
                }
            }
        }
    }
    if (channelsActive > 1) {                                                       // If >= 2 channels scale output by 75% to reduce clipping.
        level = level * 3 / 4;  
    }
    level = masterMute ? 0 : level * masterVolume / MASTER_MAX;                     // T-86 : master volume
    if (level < -127) level = -127;                                                 // Clip into range
    if (level > 127) level = 127;
  	return level;
}

// ***************************************************************************************
//
//									Play note on channel
//
// ***************************************************************************************

void SNDUpdateSoundChannel(uint8_t channel,SOUND_CHANNEL *c) {
    if (c->isPlayingNote && c->currentFrequency != 0) {  
        //      Pitch : the square toggles every half period, freq * 2 times a second. It used to be
        //      a whole number of samples (limit), plus one by the way the counter ran : 440 Hz came
        //      out at 428.9 Hz. A 16.16 phase step keeps it exact on average ; one toggle per sample
        //      at most (above half the sample rate the note cannot be made anyway).
        uint32_t step = (uint32_t)(((uint64_t)c->currentFrequency << 17) / (uint32_t)SNDGetSampleFrequency());
        if (step > 0x10000) step = 0x10000;
        if (step == 0) step = 1;
        audio[channel].step = step;
        audio[channel].phase = 0x10000 - step;                                     // First toggle on the next sample, as before
        audio[channel].soundType = c->currentType;
        audio[channel].volume = c->currentVolume;
        audio[channel].samplePos = 0;
        audio[channel].output = 0;
    } else {
        audio[channel].volume = 0;
    }
}

// ***************************************************************************************
//
//		F-12 : set the output level of a channel in place (no phase reset, so no click)
//
// ***************************************************************************************

void SNDSetCreatorVolume(uint8_t channel,int volume) {
    if (channel < CHANNEL_COUNT) audio[channel].volume = volume;
}

// ***************************************************************************************
//
//      T-79 : PCM stream (8,11 - 8,14)
//
// ***************************************************************************************

static void _SNDStreamStep(void) {
    int fe = SNDGetSampleFrequency();
    stream.step = (fe > 0) ? (uint32_t)(((uint64_t)stream.rate << 16) / (uint32_t)fe) : 0;
}

// 8,11 : both halves are taken as filled (the program fills them before starting)
uint8_t SNDStreamStart(uint16_t address,uint16_t half,uint16_t rate,int volume) {
    if (half == 0 || rate == 0 || volume < 0 || volume > 127) return 1;
    if ((uint32_t)address + 2u * half > 0xFF00) return 1;                       // Below the API page
    stream.on = false;                                                          // The interrupt ignores it while it changes
    stream.mode = STREAM_HALVES;stream.mem = cpuMemory;
    stream.base = address;stream.half = half;stream.rate = rate;stream.volume = volume;
    stream.phase = 0;stream.pos = 0;stream.cur = 0;stream.underruns = 0;
    stream.full[0] = 1;stream.full[1] = 1;
    _SNDStreamStep();
    stream.on = true;
    return 0;
}

// 8,17 (T-119) : a sample of 'length' bytes at 'address' in graphics memory, once or looped
uint8_t SNDStreamStartGraphics(uint16_t address,uint16_t length,uint16_t rate,int volume,bool loop) {
    if (length == 0 || rate == 0 || volume < 0 || volume > 127) return 1;
    if ((uint32_t)address + length > GFX_MEMORY_SIZE) return 1;                 // Inside graphics memory
    stream.on = false;
    stream.mode = loop ? STREAM_LOOP : STREAM_ONCE;stream.mem = gfxObjectMemory;
    stream.base = address;stream.half = length;stream.rate = rate;stream.volume = volume;
    stream.phase = 0;stream.pos = 0;stream.cur = 0;stream.underruns = 0;
    stream.full[0] = 1;stream.full[1] = 0;
    _SNDStreamStep();
    stream.on = true;
    return 0;
}

void SNDStreamStop(void) {                                                      // 8,12 (and 8,1)
    stream.on = false;
    stream.full[0] = 0;stream.full[1] = 0;
}

// 8,13 : bit 0/1 = half 0/1 to refill, bit 7 = playing ; underruns since the start
uint8_t SNDStreamStatus(uint16_t *underruns) {
    *underruns = stream.underruns;
    if (!stream.on) return 0;
    if (stream.mode != STREAM_HALVES) return 0x80;                              // T-119 : nothing to refill
    return 0x80 | (stream.full[0] ? 0 : 1) | (stream.full[1] ? 0 : 2);
}

uint8_t SNDStreamFilled(uint8_t half) {                                         // 8,14
    if (half > 1 || !stream.on || stream.mode != STREAM_HALVES) return 1;
    stream.full[half] = 1;
    return 0;
}

// ***************************************************************************************
//
//      T-86 : master volume (8,15 / 8,16, media keys)
//
// ***************************************************************************************

void SNDGetMasterVolume(uint8_t *volume,bool *mute) {
    *volume = masterVolume;*mute = masterMute;
}

uint8_t SNDSetMasterVolume(uint8_t volume,bool mute) {
    if (volume > MASTER_MAX) return 1;
    masterVolume = volume;masterMute = mute;
    return 0;
}

// Consumer usage of a media key press : Mute toggles, Volume - / + move by 2 and unmute.
void SNDMasterKey(uint16_t usage) {
    switch (usage) {
        case 0xE2: masterMute = !masterMute;break;
        case 0xEA: masterVolume = (masterVolume > 2) ? masterVolume - 2 : 0;masterMute = false;break;
        case 0xE9: masterVolume = (masterVolume < MASTER_MAX - 2) ? masterVolume + 2 : MASTER_MAX;masterMute = false;break;
    }
}

void SNDStreamClockChanged(void) {                                              // The output rate moved (252 <-> 270 MHz)
    if (stream.on) _SNDStreamStep();
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
