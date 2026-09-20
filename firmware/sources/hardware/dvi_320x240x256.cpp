// ***************************************************************************************
// ***************************************************************************************
//
//   	Name :   	dvi_320x240x256.cpp
//   	Authors :  	Paul Robson (paul@robsons.org.uk)
//           		Luke Wren (PicoDVI Library)
//            		Phillip Burgess (adafruit extensions used to understand it)
//   	   			bmarty (multi mode : Hercules 720x350x1, 320x256x16 — F-52/F-53)
//   	Date :   	20th November 2023
//   	Reviewed : 	No
//   	Purpose :  	Pico DVI driver, one renderer per display mode (see ADR-02).
//
//   	Mode 0 : 320x240 x 8 bpp, 640x480p60 (252 MHz), each line doubled (unchanged path)
//   	Mode 1 : 720x350 x 1 bpp, 720x480p60 (270 MHz), native lines, 65 black lines above
//   	         and below, ink = palette entry 1 (channel lit when component >= 128)
//
//   	Validated on the board on 2026-09-19/20 (Trinity 0.3.0) : mode 1 Hercules and the hot switch 0 <-> 1.
//   	Lessons : no division in the DMA IRQ (PicoDVI patch uses a mask), one 1 bpp encode per line with the
//   	lanes sharing it (PicoDVI patch : per lane offsets), core 1 parked cooperatively (never reset), DMA
//   	chaining cut before aborting the six channels (an aborted channel was retriggered by its partner).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "system/dvi_video.h"
#include "system/wdc65C02cpu.h"  											// wdc65C02cpu_set_irq (T-14, F-10 of the fork)

#include "pico/multicore.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "dvi.h"
#include "dvi_serialiser.h"
#include "dvi_serialiser.pio.h"
extern "C" {
#include "tmds_encode.h"
}
#include "system/common_dvi_pin_configs.h"
#include "hardware/structs/bus_ctrl.h"

// ***************************************************************************************
//
//                      Configuration for DVI
//
// ***************************************************************************************

// DVDD 1.2V (1.1V seems ok too) for both 252 MHz and 270 MHz
#define VREG_VSEL VREG_VOLTAGE_1_20

#define MAX_SCAN_WIDTH 		(720)  													// Widest mode in pixels
#define MONO_LINE_WORDS  	(MAX_SCAN_WIDTH / 2)  									// TMDS words per channel (2 symbols per word)
#define MONO_ENCODE_PAD  	(16)  													// tmds_encode_1bpp works by 32 pixel blocks : overshoot room
#define TMDS_BLACK_WORD  	(0x7fd00)  												// Two black symbols (balance 0 pair)

struct DisplayTiming {
	const struct dvi_timing *timing;  												// DVI timing
	uint8_t verticalRepeat;  														// 1 = native lines, 2 = each line twice
	uint16_t logicalLines;  														// Lines seen by the scanline callback
	uint16_t yOffset;  																// First framebuffer line on screen
};

static const struct DisplayTiming displayTimings[GFX_MODE_COUNT] = {
	{ &dvi_timing_640x480p_60hz, 2, 240, 0 },  										// Mode 0
	{ &dvi_timing_720x480p_60hz, 1, 480, 65 },  									// Mode 1 : 350 lines centred in 480
};

// ***************************************************************************************
//
//                        DVI Generation Data
//
// ***************************************************************************************

struct dvi_inst dvi0;                                                			// DVI information structure

uint16_t palette[256];                                            				// Current DVI palette (RGB565)
uint8_t  *screenMemory;                                           				// Page being displayed
extern volatile bool frameIrqOn,irqAsserted;  									// tick.cpp (T-14)
static uint8_t *pendingDisplayMemory = NULL;  									// Page to display from the next frame (F-55)
static struct GraphicsMode *currentMode = NULL;  								// Mode being displayed
static const struct DisplayTiming *currentTiming = NULL;
static uint8_t inkChannels = 7;  												// Mode 1 : bit 0 blue, bit 1 green, bit 2 red

uint16_t buffer1[MAX_SCAN_WIDTH+32],buffer2[MAX_SCAN_WIDTH+32];               	// 2 x 16 bpp scanline buffers used alternatively
static uint32_t monoLine1[MONO_LINE_WORDS/8+4],monoLine2[MONO_LINE_WORDS/8+4]; 	// 2 x 1 bpp scanline buffers (word aligned copies)
static const uint32_t monoZero[MONO_LINE_WORDS/8+4] = {0};  						// 1 bpp : an all black line (borders)

uint16_t frameCounter = 0,lineCounter = 0;                              		// Tracking line/frame counts.

bool  isInitialised = false;                                      				// DVI running.

const uint8_t *cursorImage = NULL; 												// Cursor status
uint16_t xCursor,yCursor,wCursor,hCursor;
bool cursorEnabled = false;

// ***************************************************************************************
//
//          Called every logical scanline (IRQ, core 1) : queue the next line
//
//   The pointer queued is what core 1 will encode : a 16 bpp buffer (modes 0/2), a word
//   aligned 1 bpp copy (mode 1) or NULL for a black line (borders of modes 1/2).
//
// ***************************************************************************************

static void __not_in_flash_func(_scanline_callback)(void) {
	uint32_t scanline;
	while (queue_try_remove_u32(&dvi0.q_colour_free, &scanline));           	// Remove unused buffers from queue
	scanline = (lineCounter & 1) ? (uint32_t)buffer1 : (uint32_t)buffer2; 		// Which buffer to send ?
	if (currentMode->bitsPerPixel == 1) scanline = (lineCounter & 1) ? (uint32_t)monoLine1 : (uint32_t)monoLine2;
	if (lineCounter < currentTiming->yOffset ||  									// Outside the framebuffer : black line.
			lineCounter >= currentTiming->yOffset + currentMode->yGSize) scanline = 0;
	queue_add_blocking_u32(&dvi0.q_colour_valid, &scanline);                	// Send buffer to queue

	lineCounter++;               												// Adjust line and frame.
	if (lineCounter == currentTiming->logicalLines) {
		frameCounter++;
		lineCounter = 0;
		if (pendingDisplayMemory != NULL) screenMemory = pendingDisplayMemory;	// Page flip at frame start (F-55)
		if (frameIrqOn) { irqAsserted = true;wdc65C02cpu_set_irq(true); }  		// T-14 : vsync IRQ (gpio_put is core safe, ~10 cycles on core 1)
		uint8_t xHit,yHit;
		cursorEnabled = MSEGetCursorDrawInformation(&xCursor,&yCursor); 		// Get cursor info this frame.
		cursorImage = CURGetCurrent(&xHit,&yHit);
		xCursor -= xHit;yCursor -= yHit;
		if (currentMode->bitsPerPixel == 1) cursorEnabled = false;  			// No mouse cursor overlay in monochrome.
		if (cursorEnabled) {  													// If enabled work out physical drawing height.
				wCursor = hCursor = 16;  										// Could be partially drawn.
				if (xCursor + 16 >= currentMode->xGSize) wCursor = currentMode->xGSize-xCursor;
				if (yCursor + 16 >= currentMode->yGSize) hCursor = currentMode->yGSize-yCursor;
		}
	}
	int y = (int)lineCounter - currentTiming->yOffset;  							// Framebuffer line to prepare (e.g. the other buffer)
	if (y < 0 || y >= currentMode->yGSize) return;  								// Border : nothing to prepare.

	if (currentMode->bitsPerPixel == 1) {  											// Mode 1 : word aligned copy of the packed line.
		uint32_t *mono = (lineCounter & 1) ? monoLine1 : monoLine2;
		memcpy(mono,screenMemory + y * currentMode->stride,currentMode->stride);
		return;
	}

	uint16_t *cursline,*scan;
	cursline = scan = (lineCounter & 1) ? buffer1 : buffer2;
	uint8_t *screenPos = screenMemory + y * currentMode->stride;          			// Data to use in screen memory.
	if (currentMode->bitsPerPixel == 8) {
		for (int i = 0;i < currentMode->xGSize;i++) {                             	// For each pixel
			*scan++ = palette[*screenPos++];                              			// convert using palette => buffer.
		}
	} else {  																		// 4 bpp : two pixels per byte, high nibble first.
		for (int i = 0;i < currentMode->stride;i++) {
			uint8_t p = *screenPos++;
			*scan++ = palette[p >> 4];
			*scan++ = palette[p & 0x0F];
		}
	}
	if (cursorEnabled && y >= yCursor && y < yCursor+hCursor) { 					// Cursor drawing on this line.
		if (xCursor >= 0 && xCursor < currentMode->xGSize-16) {  					// On Screen ?
			const uint8_t *cursorData = cursorImage + (y-yCursor) * 16;
			cursline += xCursor;  												// Position on this line.
			for (uint16_t i = 0;i < wCursor;i++) { 								// Each pixel.
				uint8_t pixel = *cursorData++;
				if (pixel != 0xFF) *cursline = palette[pixel];  				// Check for transparency
				cursline++;
			}
		}
	}
}

// ***************************************************************************************
//
//      Core 1 encoder loop : replaces dvi_scanbuf_main_16bpp so that one loop can
//      serve every mode (16 bpp doubled, 1 bpp full width, black borders by memcpy).
//
// ***************************************************************************************

static volatile bool core1StopRequest = false,core1Parked = false;  			// Cooperative stop of core 1 (mode switch)
static bool core1Launched = false;

static void __not_in_flash_func(_encode_loop)(void) {
	while (1) {
		uint32_t scanbuf,*tmdsbuf;
		if (core1StopRequest) {  													// Mode switch : leave with IRQs off, no lock held.
			irq_set_enabled(DMA_IRQ_1,false);
			return;
		}
		queue_remove_blocking_u32(&dvi0.q_colour_valid, &scanbuf);
		queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
		uint pixwidth = dvi0.timing->h_active_pixels;
		uint words_per_channel = pixwidth / DVI_SYMBOLS_PER_WORD;
		if (currentMode->bitsPerPixel == 1) {  										// 1 bpp : one encode into channel 0 ; the lanes point to it
			tmds_encode_1bpp(scanbuf ? (const uint32_t *)scanbuf : monoZero, tmdsbuf, pixwidth);   // (lit) or to the black channel 2 (dark, prefilled)
		} else if (scanbuf == 0) {  												// Black line : fill the three channels with the
			uint32_t *p = tmdsbuf;  												// constant pair (stores only, no table : T-13).
			for (uint n = 3 * words_per_channel;n > 0;n--) *p++ = TMDS_BLACK_WORD;
		} else {  																	// 16 bpp half resolution (pixel doubled) as PicoDVI does.
			const uint32_t *pix = (const uint32_t *)scanbuf;
			tmds_encode_data_channel_16bpp(pix, tmdsbuf + 0 * words_per_channel, pixwidth / 2, DVI_16BPP_BLUE_MSB,  DVI_16BPP_BLUE_LSB );
			tmds_encode_data_channel_16bpp(pix, tmdsbuf + 1 * words_per_channel, pixwidth / 2, DVI_16BPP_GREEN_MSB, DVI_16BPP_GREEN_LSB);
			tmds_encode_data_channel_16bpp(pix, tmdsbuf + 2 * words_per_channel, pixwidth / 2, DVI_16BPP_RED_MSB,   DVI_16BPP_RED_LSB  );
		}
		queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
		if (scanbuf != 0) queue_add_blocking_u32(&dvi0.q_colour_free, &scanbuf);
	}
}

// ***************************************************************************************
//
//      Start core 1, which spends most of its time in the encoder loop
//
// ***************************************************************************************

static void __not_in_flash_func(core1_main)() {
	while (1) {  																// Restartable : a mode switch parks core 1 here
		dvi_register_irqs_this_core(&dvi0, DMA_IRQ_1);                      	// Enable IRQs
		dvi_start(&dvi0);                                           			// Start DVI library
		_encode_loop();  														// Returns on a stop request
		core1Parked = true;
		while (core1StopRequest) tight_loop_contents();  						// Core 0 tears down and re-inits the DVI meanwhile
		core1Parked = false;
	}
}

// ***************************************************************************************
//
//          Serialiser (e.g. the pinout) for Neo6502
//
// ***************************************************************************************

static const struct dvi_serialiser_cfg pico_neo6502_cfg = {
	.pio = DVI_DEFAULT_PIO_INST,
	.sm_tmds = {0, 1, 2},
	.pins_tmds = {14, 18, 16},
	.pins_clk = 12,
	.invert_diffpairs = true
};

// ***************************************************************************************
//
//       Physically start the DVI hardware for the current mode : starts up core1.
//
// ***************************************************************************************

// Monochrome : the lit lanes read the encoded channel 0 of each TMDS buffer, the dark lanes the black
// channel 2 (filled once at start ; channel 1 absorbs the 32 pixel overshoot of tmds_encode_1bpp). Encoding a 720 pixel line three times (or copying it) was too slow
// for the native line rate of 720x480p60 (red "late" lines on the board, 2026-09-19).
static void _DVISetLanes(void) {
	uint32_t words = dvi0.timing->h_active_pixels / DVI_SYMBOLS_PER_WORD;
	for (int c = 0;c < 3;c++) dvi0.lane_word_offset[c] = (inkChannels & (1 << c)) ? 0 : 2 * words;   // black = channel 2 (channel 1 = encoder overshoot room)
	dvi0.lane_offsets_valid = (currentMode->bitsPerPixel == 1);
}

void HWClockChanged(void) {
	SERClockChanged();
	SNDClockChanged();
}

void DVIStart(void) {                                                             // Public and not inlined : Phosphoneo co-sim hooks this symbol (HLE).
	vreg_set_voltage(VREG_VSEL);                                      			// Set Voltage on CPU
	sleep_ms(10);
	set_sys_clock_khz(currentTiming->timing->bit_clk_khz, true);                // Set the correct clock speed.
	HWClockChanged();  															// Re-derive UART baud rate and sound sample rate.

	dvi0.timing = currentTiming->timing;                                        // Set up timing, config, callback.
	dvi0.ser_cfg = pico_neo6502_cfg;
	dvi0.scanline_callback = _scanline_callback;
	dvi0.vertical_repeat = currentTiming->verticalRepeat;  						// bmarty PicoDVI patch (runtime vertical repeat)

	dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());// Initialise DVI.
	if (currentMode->bitsPerPixel == 1) {  											// Monochrome : black channel 1 in every TMDS buffer, lanes.
		uint32_t words = dvi0.timing->h_active_pixels / DVI_SYMBOLS_PER_WORD;
		uint32_t *bufs[DVI_N_TMDS_BUFFERS];int n = 0;
		while (n < DVI_N_TMDS_BUFFERS && queue_try_remove_u32(&dvi0.q_tmds_free, &bufs[n])) n++;
		for (int i = 0;i < n;i++) {
			for (uint32_t w = 0;w < words;w++) bufs[i][2 * words + w] = TMDS_BLACK_WORD;
			queue_add_blocking_u32(&dvi0.q_tmds_free, &bufs[i]);
		}
	}
	_DVISetLanes();

	lineCounter = 2;                                                  			// We send two lines to kick off.
	uint32_t scanline;                                               			// Send junk, only lasts one frame.
	scanline = (uint32_t)buffer1;queue_add_blocking_u32(&dvi0.q_colour_valid, &scanline);
	scanline = (uint32_t)buffer2;queue_add_blocking_u32(&dvi0.q_colour_valid, &scanline);
	if (!core1Launched) { multicore_launch_core1(core1_main);core1Launched = true; }   // Start DVI worker core (RP2040 core 1)
	else core1StopRequest = false;  											// Mode switch : core 1 re-registers IRQs and restarts the DVI
	isInitialised = true;
}

// ***************************************************************************************
//
//       Stop the DVI hardware so that it can be restarted with another timing :
//       core 1, DMA channels, PIO state machines and program, TMDS buffers, queues.
//       Mirrors dvi_init()/dvi_serialiser_init()/dvi_start() in PicoDVI.
//
// ***************************************************************************************

static void DVIStopMode(void) {
	if (!isInitialised) return;
	core1Parked = false;core1StopRequest = true;  								// Ask core 1 to park outside any spinlock
	uint32_t t0 = TMRRead();  													// (resetting it while it holds a striped lock
	while (!core1Parked && TMRRead() - t0 < 20) tight_loop_contents();  		// used by malloc/queues would hang core 0).
	(void)t0;  																	// Core 1 stays parked (no multicore reset) until DVIStart clears the request.
	dvi_serialiser_enable(&dvi0.ser_cfg, false);  								// PIO serialisers and pixel clock off.
	uint32_t chans = 0;  														// DMA : the control and data channels of each lane
	for (int i = 0;i < N_TMDS_LANES;i++) chans |= (1u << dvi0.dma_cfg[i].chan_ctrl) | (1u << dvi0.dma_cfg[i].chan_data);
	for (uint c = 0;c < NUM_DMA_CHANNELS;c++) if (chans & (1u << c)) {  			// 1. no more chaining (a chained partner
		hw_clear_bits(&dma_hw->ch[c].al1_ctrl,DMA_CH0_CTRL_TRIG_EN_BITS);  		//    would retrigger an aborted channel),
		dma_channel_set_irq1_enabled(c,false);  									//    no IRQ
	}
	dma_hw->abort = chans;  														// 2. abort them all at once, wait
	while (dma_hw->abort) tight_loop_contents();
	for (uint c = 0;c < NUM_DMA_CHANNELS;c++) if (chans & (1u << c)) {
		while (dma_hw->ch[c].ctrl_trig & DMA_CH0_CTRL_TRIG_BUSY_BITS) tight_loop_contents();
		dma_channel_unclaim(c);  													// 3. release
	}
	dma_hw->ints1 = 0xFFFF;  													// Clear any pending DMA IRQ 1.
	for (int i = 0;i < N_TMDS_LANES;i++) {  										// PIO : state machines and program.
		pio_sm_set_enabled(dvi0.ser_cfg.pio, dvi0.ser_cfg.sm_tmds[i], false);
		pio_sm_unclaim(dvi0.ser_cfg.pio, dvi0.ser_cfg.sm_tmds[i]);
	}
	pio_remove_program(dvi0.ser_cfg.pio, &dvi_serialiser_program, dvi0.ser_cfg.prog_offs);
	uint32_t *buf;  															// TMDS buffers were malloc'd by dvi_init.
	while (queue_try_remove_u32(&dvi0.q_tmds_free, &buf)) free(buf);
	while (queue_try_remove_u32(&dvi0.q_tmds_valid, &buf)) free(buf);
	if (dvi0.tmds_buf_release) free(dvi0.tmds_buf_release);
	if (dvi0.tmds_buf_release_next) free(dvi0.tmds_buf_release_next);
	dvi0.tmds_buf_release = dvi0.tmds_buf_release_next = NULL;
	queue_free(&dvi0.q_tmds_free);queue_free(&dvi0.q_tmds_valid);  			// Queue storage (calloc'd by queue_init).
	queue_free(&dvi0.q_colour_free);queue_free(&dvi0.q_colour_valid);
	isInitialised = false;
}

// ***************************************************************************************
//
//                   Called on mode start (every GFXSetMode)
//
// ***************************************************************************************

// T-17 : pause the DVI around a flash write (core 1 parked in RAM, DMA and PIO off), then restart it.
void RNDSuspend(void) { DVIStopMode(); }
void RNDResume(void) { if (!isInitialised) DVIStart(); }

void RNDStartMode0(struct GraphicsMode *gMode) {
	const struct DisplayTiming *t = &displayTimings[gMode->modeID];
	bool restart = isInitialised && t != currentTiming;  							// Timing or repeat changes : restart DVI.
	if (restart) DVIStopMode();
	screenMemory = gMode->graphicsMemory;                             			// Remember where drawing.
	currentMode = gMode;
	currentTiming = t;
	if (!isInitialised) DVIStart();
}

//
//		Every mode has a renderer now. Set to mode 0 only if the board misbehaves in
//		the other modes (untested on hardware).
//
void RNDSetDisplayPage(uint8_t *displayMemory) {
	pendingDisplayMemory = displayMemory;  											// Taken into account at the next frame start.
	if (!isInitialised) screenMemory = displayMemory;
}

int RNDModeSupported(int mode) {
	return mode >= 0 && mode < GFX_MODE_COUNT;
}

// ***************************************************************************************
//
//                      Get frame counter
//
// ***************************************************************************************

int  RNDGetFrameCount(void) {
	return frameCounter;
}

// ***************************************************************************************
//
//       Update the palette of whatever is generating the video. In monochrome the
//       ink is palette entry 1 : a channel is lit when its component is >= 128
//       (white, amber 255,176,0 -> red+green, green 0,255,0 ...).
//
// ***************************************************************************************

void RNDSetPalette(uint8_t colour,uint8_t r,uint8_t g,uint8_t b) {
	palette[colour] = ((r & 0xF8) << (11-3)) + ((g & 0xFC) << (5-2)) + (b >> 3);
	if (colour == 1) {
		inkChannels = (b >= 128 ? 1 : 0) | (g >= 128 ? 2 : 0) | (r >= 128 ? 4 : 0);
		if (isInitialised && currentMode != NULL && currentMode->bitsPerPixel == 1) _DVISetLanes();
	}
}

// ***************************************************************************************
//
//    Date   Revision
//    ====   ========
//	  15-09-26  Multi mode renderer (F-52/F-53), untested on hardware.
//
// ***************************************************************************************
