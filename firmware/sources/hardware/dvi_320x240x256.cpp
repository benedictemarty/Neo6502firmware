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
//   	Mode 2 : 320x256 x 4 bpp, 640x480p60 (252 MHz), native lines, 112 black lines
//   	         above and below, pixels doubled horizontally by the 16 bpp encoder
//
//   	NOT YET RUN ON A BOARD (written without hardware, see docs-bmarty/F-52-hercules.md
//   	for the checks to make on the first board).
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "system/dvi_video.h"
#include "system/wdc65C02cpu.h"  											// wdc65C02cpu_set_irq (F-10)

#include "pico/multicore.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
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
	{ &dvi_timing_640x480p_60hz, 1, 480, 112 },  									// Mode 2 : 256 lines centred in 480
	{ &dvi_timing_720x480p_60hz, 1, 480, 65 },  									// Mode 3 : as mode 1 (80x43 text, F-57)
};

// ***************************************************************************************
//
//                        DVI Generation Data
//
// ***************************************************************************************

struct dvi_inst dvi0;                                                			// DVI information structure

uint16_t palette[256];                                            				// Current DVI palette (RGB565)
uint8_t  *screenMemory;                                           				// Page being displayed
static uint8_t *pendingDisplayMemory = NULL;  									// Page to display from the next frame (F-55)
static struct GraphicsMode *currentMode = NULL;  								// Mode being displayed
static const struct DisplayTiming *currentTiming = NULL;
static uint8_t inkChannels = 7;  												// Mode 1 : bit 0 blue, bit 1 green, bit 2 red

uint16_t buffer1[MAX_SCAN_WIDTH+32],buffer2[MAX_SCAN_WIDTH+32];               	// 2 x 16 bpp scanline buffers used alternatively
static uint32_t monoLine1[MONO_LINE_WORDS/8+4],monoLine2[MONO_LINE_WORDS/8+4]; 	// 2 x 1 bpp scanline buffers (word aligned copies)
static uint8_t memLine[320];  													// ADR-04 : line rendered from 6502 RAM (8 bpp indexes, 320 wide at most : SRAM budget R22)
static uint32_t monoEncoded[MONO_LINE_WORDS+MONO_ENCODE_PAD]; 					// 1 bpp encode target (core 1 only)
static uint32_t blackChannel[MONO_LINE_WORDS];  								// One channel of black symbols

uint16_t frameCounter = 0,lineCounter = 0;                              		// Tracking line/frame counts.
extern volatile bool frameIrqOn,irqAsserted;  									// tick.cpp (F-10, F-60)

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
		if (frameIrqOn) { irqAsserted = true;wdc65C02cpu_set_irq(true); }  		// F-10 : vsync IRQ (gpio_put is core safe ; ~10 cycles on core 1)
		if (pendingDisplayMemory != NULL) screenMemory = pendingDisplayMemory;	// Page flip at frame start (F-55)
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
	if (currentMode->layout != LAYOUT_FRAMEBUFFER) {  								// ADR-04 : line rendered from the 6502 RAM.
		MEMRenderLine(memLine,y);
		screenPos = memLine;
	}
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

static void __not_in_flash_func(_encode_loop)(void) {
	while (1) {
		uint32_t scanbuf,*tmdsbuf;
		queue_remove_blocking_u32(&dvi0.q_colour_valid, &scanbuf);
		queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
		uint pixwidth = dvi0.timing->h_active_pixels;
		uint words_per_channel = pixwidth / DVI_SYMBOLS_PER_WORD;
		if (scanbuf == 0) {  														// Black line : copy the constant channel.
			for (int c = 0;c < 3;c++) memcpy(tmdsbuf + c * words_per_channel,blackChannel,words_per_channel * 4);
		} else if (currentMode->bitsPerPixel == 1) {  								// 1 bpp : one encode, copied to the lit channels.
			tmds_encode_1bpp((const uint32_t *)scanbuf, monoEncoded, pixwidth);
			for (int c = 0;c < 3;c++) {
				memcpy(tmdsbuf + c * words_per_channel,(inkChannels & (1 << c)) ? monoEncoded : blackChannel,words_per_channel * 4);
			}
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
	dvi_register_irqs_this_core(&dvi0, DMA_IRQ_1);                      		// Enable IRQs
	dvi_start(&dvi0);                                           				// Start DVI library
	_encode_loop();
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

void HWClockChanged(void) {
	SERClockChanged();
	SNDClockChanged();
}

void DVIStart(void) {                                                             // Public and not inlined : Phosphoneo co-sim hooks this symbol (HLE).
	vreg_set_voltage(VREG_VSEL);                                      			// Set Voltage on CPU
	sleep_ms(10);
	set_sys_clock_khz(currentTiming->timing->bit_clk_khz, true);                // Set the correct clock speed.
	HWClockChanged();  															// Re-derive UART baud rate and sound sample rate.

	for (int i = 0;i < MONO_LINE_WORDS;i++) blackChannel[i] = TMDS_BLACK_WORD; 	// Constant black channel.

	dvi0.timing = currentTiming->timing;                                        // Set up timing, config, callback.
	dvi0.ser_cfg = pico_neo6502_cfg;
	dvi0.scanline_callback = _scanline_callback;
	dvi0.vertical_repeat = currentTiming->verticalRepeat;  						// bmarty PicoDVI patch (runtime vertical repeat)

	dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());// Initialise DVI.

	lineCounter = 2;                                                  			// We send two lines to kick off.
	uint32_t scanline;                                               			// Send junk, only lasts one frame.
	scanline = (uint32_t)buffer1;queue_add_blocking_u32(&dvi0.q_colour_valid, &scanline);
	scanline = (uint32_t)buffer2;queue_add_blocking_u32(&dvi0.q_colour_valid, &scanline);
	multicore_launch_core1(core1_main);                               			// Start DVI worker core (RP2040 core 1)
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
	multicore_reset_core1();  													// Core 1 (encoder loop + DMA IRQs) stops here.
	dvi_serialiser_enable(&dvi0.ser_cfg, false);  								// PIO serialisers and pixel clock off.
	for (int i = 0;i < N_TMDS_LANES;i++) {  										// DMA : abort, disable IRQ, release.
		dma_channel_set_irq1_enabled(dvi0.dma_cfg[i].chan_data, false);
		dma_channel_abort(dvi0.dma_cfg[i].chan_ctrl);
		dma_channel_abort(dvi0.dma_cfg[i].chan_data);
		dma_channel_unclaim(dvi0.dma_cfg[i].chan_ctrl);
		dma_channel_unclaim(dvi0.dma_cfg[i].chan_data);
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
	if (colour == 1) inkChannels = (b >= 128 ? 1 : 0) | (g >= 128 ? 2 : 0) | (r >= 128 ? 4 : 0);
}

// ***************************************************************************************
//
//    Date   Revision
//    ====   ========
//	  15-09-26  Multi mode renderer (F-52/F-53), untested on hardware.
//
// ***************************************************************************************
