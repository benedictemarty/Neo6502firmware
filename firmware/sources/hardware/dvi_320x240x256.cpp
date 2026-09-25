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
#include "hardware/structs/bus_ctrl.h"  											// T-56 : DMA priority over the cores
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

//		T-53 : scanline buffers of the colour modes. There used to be two, alternated by parity,
//		and the encoder reads each one THREE times — once per TMDS channel, red last. The line
//		callback runs in an interrupt on the same core as the encoder, so when core 0 saturates
//		memory (FatFs during a directory listing, Tab completion in NeoDOS) the encoder falls
//		behind, the callback comes back round to the buffer it is still reading, and only the
//		third pass sees the new data : blue and green right, red wrong. That is the red streak,
//		and late_scanline_ctr stays at 0 because the TMDS buffer itself is published on time —
//		it is its source that changed underneath. Four buffers now, and they are sized for the
//		widest COLOUR mode (320, mode 0) instead of the widest mode overall (720, which is
//		monochrome and uses monoLine) : four buffers now cost less RAM than the old two.
#define SCAN_MAX_PIXELS (320)  													// Mode 0 ; mode 1 is 1 bpp and uses monoLine
#define SCAN_BUF_COUNT  (4)
static uint16_t scanBuffer[SCAN_BUF_COUNT][SCAN_MAX_PIXELS+32] __attribute__((aligned(4)));   // T-58 : word aligned
#define SCANBUF(n) (scanBuffer[(n) & (SCAN_BUF_COUNT-1)])
//		T-71, lot 2 (0.16.10) : the FOUR 1 bpp line buffers are ALWAYS allocated, and how many
//		are actually used (two or four) is picked at RUN TIME by monoLineMask. 0.16.6 added
//		392 bytes of data and so moved the whole memory map of the binary — and T-48 proved on
//		this very board that placement alone makes the fault come and go. Comparing 0.16.6 with
//		0.16.9 was therefore never a one variable experiment, and withdrawing the four buffers
//		is a verdict still to be re-examined. Here both arms are the SAME binary, same size,
//		same placement : only the mask differs (!2 / !4 on the debug port), applied on a frame
//		boundary so the encoder never sees an index change mid line.
//		With mask 1 the callback comes back to the buffer the encoder is still reading as soon
//		as it falls behind ; with mask 3 it has three lines of slack.
//		VERDICT (0.16.13) : TWO. Measured on the board on 2026-09-25 with the run time switch
//		above, same binary both ways : with four buffers, entering mode 1 is enough to leave only
//		ONE of the three TMDS lanes fed, TXSTALL and TXOVER raised at once, without a single disk
//		access. The experiment is over, so the switch and the two spare buffers go : 392 bytes
//		back, and RAM_LIMIT was down to 52 free.
#define MONO_LINE_COUNT (2)
static uint32_t monoLine[MONO_LINE_COUNT][MONO_LINE_WORDS/8+4]; 					// 2 x 1 bpp scanline buffers (word aligned copies)
#define monoLineMask (MONO_LINE_COUNT-1)
static const uint32_t monoZero[MONO_LINE_WORDS/8+4] = {0};  						// 1 bpp : an all black line (borders)

uint16_t frameCounter = 0,lineCounter = 0;                              		// Tracking line/frame counts.
static volatile uint32_t lateTotal = 0;  										// T-57 : episodes of late scanlines, cumulative
//		T-71, lot 1 (0.16.10) : the line callback used to publish with queue_add_blocking_u32.
//		It runs in an interrupt ON CORE 1, and q_colour_valid (8 slots, picodvi dvi.c) has
//		exactly one consumer : _encode_loop, which runs on that same core 1 and which the
//		interrupt preempts. Once the encoder is eight lines behind, the callback waits for room
//		that only the code it has preempted could make : a deadlock that no amount of bandwidth
//		fixes. That is the shape of the 0.16.6 freeze (frame counter stuck at 661 for eleven
//		seconds, then a spontaneous reboot). Publishing without blocking makes the freeze
//		impossible — a dropped line costs one late line, which picodvi already handles — and
//		the counter says whether the queue ever fills at all. Either answer is a measurement :
//		rejets > 0 holds the mechanism, rejets == 0 during a black DIR kills it for good.
#define DISPLAY_STUCK_TICKS (24)  											// T-71 : DSPSync ticks (95 Hz) before declaring the display dead
static volatile uint32_t publishRejects = 0;  									// T-71 : lines dropped rather than blocking in the IRQ

bool  isInitialised = false;                                      				// DVI running.

const uint8_t *cursorImage = NULL; 												// Cursor status
//
//		T-32c : core 0 (RNDCursorUpdate, in DSPSync) prepares what the line callback needs about
//		the mouse cursor, because those calls live in flash and the callback must not stall on XIP.
//		T-43 : it publishes a whole slot at once. Writing the seven fields one by one let core 1
//		read a mix of two states at the start of a frame (a new position with an old width, or
//		"enabled" with an image pointer not yet stored) ; the index below is a single byte, whose
//		store is atomic on the M0+, and core 0 always fills the slot core 1 is not reading.
//
//		T-44 : the image itself is copied here, in RAM. CURGetCurrent returns a pointer into
//		cursor_data, which is const, hence in flash : reading it from the callback was an XIP
//		access per cursor pixel, on the very core whose line must be encoded on time, while
//		core 0 hammers the flash — the late lines 0.10.3 set out to remove (T-38).
struct CursorState {
	uint16_t x,y,w,h;
	uint8_t skipX,skipY;
	bool on;
};
//		One shared image buffer rather than one per slot : it is only rewritten when the program
//		changes cursor (CURSetCurrent), and a change costs at worst one frame showing two halves
//		of two cursors. The 256 bytes saved matter (RAM_LIMIT, T-13).
static uint8_t cursorPixels[CURSOR_IMAGE_BYTES];
static const uint8_t *cursorPixelSource = NULL;  								// What cursorPixels holds
static struct CursorState cursorSlot[2] = {};  									// 2 x 272 bytes of RAM (T-44)
static volatile uint8_t cursorSlotIndex = 0;  									// Slot core 1 must read
uint16_t xCursor,yCursor,wCursor,hCursor;
static uint8_t skipXCursor = 0,skipYCursor = 0;
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
	if (dvi0.late_scanline_ctr) lateTotal = lateTotal + 1;  						// T-57 : sampled every line, on core 1
	while (queue_try_remove_u32(&dvi0.q_colour_free, &scanline));           	// Remove unused buffers from queue
	scanline = (uint32_t)SCANBUF(lineCounter); 									// Which buffer to send ?
	if (currentMode->bitsPerPixel == 1) scanline = (uint32_t)monoLine[lineCounter & monoLineMask];
	if (lineCounter < currentTiming->yOffset ||  									// Outside the framebuffer : black line.
			lineCounter >= currentTiming->yOffset + currentMode->yGSize) scanline = 0;
	if (!queue_try_add_u32(&dvi0.q_colour_valid, &scanline)) publishRejects = publishRejects + 1;  	// T-71 : NEVER block here (see above)

	lineCounter++;               												// Adjust line and frame.
	if (lineCounter == currentTiming->logicalLines) {
		frameCounter++;
		lineCounter = 0;
		if (pendingDisplayMemory != NULL) screenMemory = pendingDisplayMemory;	// Page flip at frame start (F-55)
		if (frameIrqOn) { irqAsserted = true;wdc65C02cpu_set_irq(true); }  		// T-14 : vsync IRQ (gpio_put is core safe, ~10 cycles on core 1)
		const struct CursorState *c = &cursorSlot[cursorSlotIndex];  			// T-43 : one consistent state, published
		cursorEnabled = c->on;cursorImage = cursorPixels;  						// as a whole by core 0 ; the image is in
		xCursor = c->x;yCursor = c->y;wCursor = c->w;hCursor = c->h;  			// RAM (T-44), never read from flash here
		skipXCursor = c->skipX;skipYCursor = c->skipY;
	}
	int y = (int)lineCounter - currentTiming->yOffset;  							// Framebuffer line to prepare (e.g. the other buffer)
	if (y < 0 || y >= currentMode->yGSize) return;  								// Border : nothing to prepare.

	if (currentMode->bitsPerPixel == 1) {  											// Mode 1 : word aligned copy of the packed line.
		uint32_t *mono = monoLine[lineCounter & monoLineMask];
		memcpy(mono,screenMemory + y * currentMode->stride,currentMode->stride);
		if (cursorEnabled && y >= yCursor && y < yCursor+hCursor) {   					// Mouse cursor in
			const uint8_t *cursorData = cursorImage + (y-yCursor+skipYCursor) * 16 + skipXCursor;   // monochrome (T-29) : colour 0 = off,
			uint8_t *bits = (uint8_t *)mono;  											// any other colour = on, $FF transparent
			for (uint16_t i = 0;i < wCursor;i++) {
				uint8_t pixel = *cursorData++;
				if (pixel == 0xFF) continue;
				uint16_t px = xCursor + i;
				if (pixel) bits[px >> 3] |= (0x80 >> (px & 7)); else bits[px >> 3] &= ~(0x80 >> (px & 7));
			}
		}
		return;
	}

	uint16_t *cursline,*scan;
	cursline = scan = SCANBUF(lineCounter);  									// The encoder is still on an older one
	uint8_t *screenPos = screenMemory + y * currentMode->stride;          			// Data to use in screen memory.
	if (currentMode->bitsPerPixel == 8) {
		//		T-58 : four pixels at a time. What starves this core during a disk transfer is
		//		memory bandwidth, not cycles — five TMDS buffers changed nothing, and giving the
		//		DMA priority over the cores made the streaks worse. Byte by byte, a 320 pixel
		//		line cost 320 reads and 320 halfword writes, all in an interrupt that preempts
		//		the encoder ; word at a time it costs 80 reads and 160 writes. The stride is a
		//		multiple of four and both buffers are word aligned, so this is safe.
		//		Alignment is guaranteed : both buffers are declared word aligned and the stride
		//		of every colour mode is a multiple of four, so no run time check is needed.
		int pixels = currentMode->xGSize;
		const uint32_t *src32 = (const uint32_t *)screenPos;
		uint32_t *dst32 = (uint32_t *)scan;
		for (int i = pixels >> 2;i > 0;i--) {
			uint32_t w = *src32++;  												// Four indices in one read
			uint32_t p0 = palette[w & 0xFF],p1 = palette[(w >> 8) & 0xFF];
			uint32_t p2 = palette[(w >> 16) & 0xFF],p3 = palette[w >> 24];
			*dst32++ = p0 | (p1 << 16);  											// Two pixels per write
			*dst32++ = p2 | (p3 << 16);
		}
		scan += pixels;screenPos += pixels;
	} else {  																		// 4 bpp : two pixels per byte, high nibble first.
		for (int i = 0;i < currentMode->stride;i++) {
			uint8_t p = *screenPos++;
			*scan++ = palette[p >> 4];
			*scan++ = palette[p & 0x0F];
		}
	}
	if (cursorEnabled && y >= yCursor && y < yCursor+hCursor) { 					// Cursor drawing on this line.
		const uint8_t *cursorData = cursorImage + (y-yCursor+skipYCursor) * 16 + skipXCursor;   // T-42 : clipped by RNDCursorUpdate
		cursline += xCursor;  													// Position on this line.
		for (uint16_t i = 0;i < wCursor;i++) { 									// Each pixel.
			uint8_t pixel = *cursorData++;
			if (pixel != 0xFF) *cursline = palette[pixel];  					// Check for transparency
			cursline++;
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
static volatile bool core1FlashPause = false,core1InFlashPause = false;  		// T-32b : cooperative pause for flash writes
static bool core1Launched = false;

static void __not_in_flash_func(_encode_loop)(void) {
	while (1) {
		uint32_t scanbuf,*tmdsbuf;
		if (core1StopRequest) {  													// Mode switch : leave with IRQs off, no lock held.
			irq_set_enabled(DMA_IRQ_1,false);
			return;
		}
		if (core1FlashPause) {  													// Flash write (banks, settings) : park here, in RAM,
			irq_set_enabled(DMA_IRQ_1,false);  										// with interrupts off — no SDK lockout handler, which
			core1InFlashPause = true;  												// lives in flash and stole cycles from the encoder
			while (core1FlashPause) tight_loop_contents();  						// (red late lines on the board, bmarty 2026-09-22).
			core1InFlashPause = false;
			irq_set_enabled(DMA_IRQ_1,true);
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
	DBGClockChanged();  														// T-74 : the debug port has its own baud rate
}

void DVIStart(void) {                                                             // Public and not inlined : Phosphoneo co-sim hooks this symbol (HLE).
	//		T-56, withdrawn : giving the DMA priority over the cores made the streaks MUCH worse
	//		on the board (bmarty, 2026-09-23). Useful all the same — it says the starved party is
	//		core 1, the encoder, not the display DMA : taking bandwidth from the cores hurts.
	//
	//		T-71, ANNULÉ : donner la priorité du bus à core 1 (BUSCTRL_BUS_PRIORITY_PROC1_BITS)
	//		était la conclusion logique de T-56 — jamais essayée — mais sur carte elle est pire
	//		que le mal (bmarty, 2026-09-25, 0.16.7) : **l'écran noircit dès `MODE 1`, sans le
	//		moindre accès disque**, et même les lectures SWD échouent. La priorité s'applique à
	//		tout ce qui partage le bus : en mode 1 core 1 tourne à plein et monopolise la SRAM,
	//		si bien que core 0 ne peut plus écrire la mémoire vidéo, ni le port de debug lire
	//		la sienne. Comme T-56, l'essai reste instructif : les deux cœurs se disputent bien
	//		la même bande passante, mais **aucun des deux ne peut être privilégié** — il faut
	//		diminuer la demande, pas arbitrer entre elles.
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
	scanline = (uint32_t)SCANBUF(0);queue_add_blocking_u32(&dvi0.q_colour_valid, &scanline);
	scanline = (uint32_t)SCANBUF(1);queue_add_blocking_u32(&dvi0.q_colour_valid, &scanline);
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

// T-17 : full teardown (unused : it left the screen black on the board, 0.9.9).
void RNDSuspend(void) { DVIStopMode(); }
void RNDResume(void) { if (!isInitialised) DVIStart(); }

// T-32b : park core 1 in RAM for a flash write. The DVI hardware is left alone : the picture freezes
// for the duration (the DMA is no longer rearmed) and resumes at once, without re-locking the monitor.
// T-32c : core 0 prepares what the line callback needs about the mouse cursor (these calls live in flash).
void RNDCursorUpdate(void) {
	uint16_t mx,my;uint8_t xHit,yHit;
	bool on = MSEGetCursorDrawInformation(&mx,&my);
	const uint8_t *image = CURGetCurrent(&xHit,&yHit);
	int x = (int)mx - xHit,y = (int)my - yHit;  								// T-42 : signed. The hot spot puts the top left
	int skipX = 0,skipY = 0,w = 16,h = 16;  									// corner off screen when the pointer nears an edge,
	if (x < 0) { skipX = -x;w -= skipX;x = 0; }  								// and an unsigned x wrapped to ~65530 : the cursor
	if (y < 0) { skipY = -y;h -= skipY;y = 0; }  								// then vanished instead of being clipped.
	if (currentMode != NULL) {
		if (x + w > currentMode->xGSize) w = currentMode->xGSize - x;
		if (y + h > currentMode->yGSize) h = currentMode->yGSize - y;
	}
	if (w <= 0 || h <= 0 || image == NULL) on = false;  							// Entirely off screen, or no image.
	uint8_t slot = cursorSlotIndex ^ 1;  										// T-43 : fill the slot core 1 is not reading,
	if (cursorPixelSource != image && image != NULL) {  							// then publish it with one byte store.
		memcpy(cursorPixels,image,CURSOR_IMAGE_BYTES);  						// T-44 : flash -> RAM, on core 0
		cursorPixelSource = image;
	}
	cursorSlot[slot].x = x;cursorSlot[slot].y = y;
	cursorSlot[slot].w = (w > 0) ? w : 0;cursorSlot[slot].h = (h > 0) ? h : 0;
	cursorSlot[slot].skipX = skipX;cursorSlot[slot].skipY = skipY;
	cursorSlot[slot].on = on;
	__dmb();  																	// Slot written before it is published
	cursorSlotIndex = slot;
}

//		T-57 : late_scanline_ctr of PicoDVI is a STATE, not a total : it is decremented as soon
//		as the pipeline catches up (dvi.c, ++ at one place, -- at another). Reading it from
//		DSPSync, 95 times a second, almost always caught it back at zero — which is why "DVI = 0"
//		wrongly cleared the display of any suspicion all day. The line callback runs on core 1
//		every 31 µs, so sampling there does catch the episodes ; lateTotal counts them.
uint32_t RNDLateScanlines(void) { return lateTotal; }

//		T-71 (0.16.10). Two numbers the debug port reports and the SWD probe can read without
//		stopping either core : lines dropped rather than blocking the callback, and how many
//		1 bpp buffers the mode is currently rotating through.
//		In RAM : the telemetry tick calls this from DSPSync, and nothing DSPSync reaches may
//		live in flash (T-46). Reading a counter must not cost an XIP access on core 0 while
//		core 1 is encoding — that is the very contention being measured.
uint32_t __not_in_flash_func(RNDPublishRejects)(void) { return publishRejects; }
//		T-71 lot 4 (0.16.11) : escapes from the bounded wait picodvi now does on the three data
//		channels. It must stay at zero. Anything else is the reload anomaly measured on the board
//		on 2026-09-25 -- two channels stopped, carrying another lane's control block, reload count
//		320 where mode 1 needs 360 -- which used to hang that wait, and core 1 with it, for good.
uint32_t __not_in_flash_func(RNDDmaWaitEscapes)(void) { return dvi_tcr_timeouts; }
//		T-77 (0.16.15) : reconfigurations of a lane control channel that landed while it was
//		still transferring its four words. The hypothesis says this is what shifts the list by
//		one rank ; zero here during a black screen would kill it.
uint32_t __not_in_flash_func(RNDCtrlBusyReconfig)(void) { return dvi_ctrl_busy_reconfig; }
//		T-71 lot 5 (0.16.12) : restarts of the three lane control channels, done from the
//		interrupt itself because it is the only one that still runs once the channels stop.
//
//		T-71 lot 6 (0.16.13) : the recovery net, and it runs on CORE 0 on purpose.
//
//		Everything tried inside the scanline interrupt failed, and the measurements say why :
//		when the lane channels stop, core 1 is gone -- the interrupt stops firing, the encoder
//		waits for TMDS buffers nobody returns, and a repair attempted from the one interrupt that
//		still runs cannot rebuild a coherent state (0.16.12 restarted the channels 65000 times a
//		second without the frame counter ever moving again). Core 0, meanwhile, is measurably
//		alive : during the freeze it still consumes the keyboard queue. And there is a repair
//		that is known to work, because bmarty has been doing it by hand since the first day :
//		MODE 0 then MODE 1, e.g. a full mode restart.
//
//		So core 0 watches the frame counter. If it stops moving while a mode is running, it does
//		the restart itself. The check is in RAM and costs a comparison per tick (T-46) ; the
//		restart path is in flash but is only ever reached when the display is already dead.
static volatile uint32_t displayRestarts = 0;  									// Recoveries performed

uint32_t __not_in_flash_func(RNDDisplayRestarts)(void) { return displayRestarts; }

void __not_in_flash_func(RNDDisplayWatchdog)(void) {
	static uint16_t lastFrame = 0;
	static uint8_t stuck = 0;
	if (!isInitialised || core1FlashPause || core1StopRequest) { stuck = 0;return; }   // Legitimate pauses, not a fault
	if (frameCounter != lastFrame) { lastFrame = frameCounter;stuck = 0;return; }
	if (++stuck < DISPLAY_STUCK_TICKS) return;  								// ~250 ms of a frozen counter at 95 Hz
	stuck = 0;
	displayRestarts = displayRestarts + 1;
	DVIStopMode();  															// The MODE 0 / MODE 1 that has always worked,
	DVIStart();  																// done by the core that is still standing
}
uint32_t __not_in_flash_func(RNDMonoBuffers)(void) { return MONO_LINE_COUNT; }

void RNDFlashPause(void) {
	if (!isInitialised) return;
	core1InFlashPause = false;core1FlashPause = true;
	uint32_t t0 = TMRRead();
	while (!core1InFlashPause && TMRRead() - t0 < 20) tight_loop_contents();  	// 200 ms at most
}

void RNDFlashResume(void) {
	core1FlashPause = false;
	uint32_t t0 = TMRRead();
	while (core1InFlashPause && TMRRead() - t0 < 20) tight_loop_contents();
}

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
