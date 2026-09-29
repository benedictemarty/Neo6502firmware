// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      processor_pio.cpp
//      Authors :   Sascha Schneider
//                  Oliver Schmidt
//                  Rien Matthijsse
//                  Angel Sancho
//      Date :      8th December 2023
//      Reviewed :  No
//      Purpose :   Drive the 65C02 processor via PIO
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "bus_serve.h"

#define PICO_NEO6502
#define CHIPS_IMPL

#include "system/wdc65C02cpu.h"
#include "sm0_memory_emulation_with_clock.pio.h"

extern volatile bool irqAsserted;  											// tick.cpp (F-60)

// ***************************************************************************************
//
//		T-49 : bus stall counters. The PIO state machine generates the 6502 clock (side-set
//		on GPIO 21), so when the firmware is late the machine stalls and the clock simply
//		stops : the 6502 is stretched, never fed a wrong byte. Those stalls are what a
//		starved firmware looks like, and the hardware records them for free in PIO_FDEBUG,
//		whose bits are sticky until written back. TXSTALL = the data of a read was not ready
//		in time, RXSTALL = the address FIFO was not drained in time.
//
//		Sampled from DSPSync (~95 Hz), so a count is "how many sampling windows contained at
//		least one stall", not the exact number of stalls — enough to tell a healthy binary
//		(0) from a starved one, and it costs the bus loop nothing (T-46 : this must not add
//		flash code to the critical path, hence __not_in_flash_func).
//
// ***************************************************************************************

#define BUS_FDEBUG_TXSTALL (1u << 24)  										// SM0 : PIO_FDEBUG TXSTALL[27:24]
#define BUS_FDEBUG_TXOVER  (1u << 16)  										// TXOVER[19:16] : data pushed into a full FIFO, lost
#define BUS_FDEBUG_RXUNDER (1u << 8)   										// RXUNDER[11:8] : an EMPTY FIFO was read
#define BUS_FDEBUG_RXSTALL (1u << 0)   										// RXSTALL[3:0]
#define BUS_FDEBUG_ALL (BUS_FDEBUG_TXSTALL|BUS_FDEBUG_TXOVER|BUS_FDEBUG_RXUNDER|BUS_FDEBUG_RXSTALL)

//		T-52 : the two that matter. TXSTALL and RXSTALL are normal — serving an API command
//		stalls the machine. RXUNDER is not : the bus loop calls pio_sm_get(), which reads
//		pio->rxf[sm] without checking the FIFO holds anything, so an early firmware reads a
//		stale word and then writes a phantom byte at a phantom address into cpuMemory. The
//		6502 program is corrupted while the firmware carries on — which is why Ctrl+Alt+AltGr
//		still reboots the board. TXOVER means a read reply was pushed into a full FIFO and lost.
static uint32_t busStallTx = 0,busStallRx = 0,busUnder = 0,busOver = 0;

void __not_in_flash_func(HWBusProbe)(void) {
	uint32_t flags = pio1->fdebug & BUS_FDEBUG_ALL;
	if (flags == 0) return;
	if (flags & BUS_FDEBUG_TXSTALL) busStallTx++;
	if (flags & BUS_FDEBUG_RXSTALL) busStallRx++;
	if (flags & BUS_FDEBUG_RXUNDER) busUnder++;
	if (flags & BUS_FDEBUG_TXOVER) busOver++;
	pio1->fdebug = flags;  													// Sticky : write 1 to clear
}

//		T-50 : stall counts alone are too coarse — servicing an API command already stalls the
//		PIO, because the 6502 spins on the control port while the firmware works. What tells a
//		healthy firmware from a starved one is **how long** it stays away from the bus. These
//		measure, in microseconds, the longest and the total time spent outside the bus loop in
//		DSPSync (keyboard, USB task, blink) and in DSPHandler (one API command).
//		Only the worst case is kept : it is what separates a healthy firmware from a starved
//		one, and RAM is down to a few hundred spare bytes (T-13).
static uint32_t syncMaxUs = 0,cmdMaxUs = 0;

//		T-82 (ADR-0002) : a background read (3,28) serves the bus from wait_for_disk_io. A write
//		of the control port seen there cannot be handled at once (we are inside FatFs and
//		TinyUSB) : it is noted, the 65C02 keeps waiting on it, and the main loop runs it when
//		the read is over — in a loop, not recursively, so chained reads cannot pile up.
static volatile bool busServeInWait = false;
static volatile bool busPendingCommand = false;
static uint8_t burstWrites = 0;

void HWBackgroundServe(bool on) {
	busServeInWait = on;
}

bool __not_in_flash_func(HWBusServeInWait)(void) {
	return busServeInWait;
}

void __time_critical_func(HWBusServeBurst)(void) {  								// Called between two tuh_task()
	union u32 value;
	uint8_t consecutive_writes = burstWrites;
	const uint16_t cp = CONTROLPORT;
	for (int i = 0;i < 64;i++) {
		BUS_SERVE_ONE({ busPendingCommand = true; })
	}
	burstWrites = consecutive_writes;
}

uint32_t HWBusTiming(uint8_t which) {
	switch (which) {
		case 0: return syncMaxUs;
		case 1: return cmdMaxUs;
		case 2: return RNDCallbackMax();  										// T-90 : display timings of core 1 (T-77)
		case 3: return RNDIrqGapMax();
		case 4: return RNDIrqGapLong();
	}
	return 0;
}

uint32_t HWBusStalls(uint8_t which) {  										// 0 TXSTALL, 1 RXSTALL, 2 RXUNDER, 3 TXOVER
	switch(which) {
		case 0: return busStallTx;
		case 1: return busStallRx;
		case 2: return busUnder;
		case 3: return busOver;
	}
	return 0;
}

void HWBusStallsReset(void) {
	busStallTx = busStallRx = busUnder = busOver = 0;
	syncMaxUs = cmdMaxUs = 0;
	RNDTimingReset();  															// T-90
	pio1->fdebug = BUS_FDEBUG_ALL;
}

void initPio() {
    uint offset = 0;

    offset = pio_add_program(pio1, &memory_emulation_with_clock_program);
    memory_emulation_with_clock_program_init(pio1, 0, offset);
    pio_sm_set_enabled(pio1, 0, true);
}

// ***************************************************************************************
//
//                      Start and run the CPU. Does not have to return.
//
// ***************************************************************************************

void __time_critical_func(CPUExecute)(void) {
    wdc65C02cpu_init();
    initPio();
    wdc65C02cpu_reset();

    union u32 value;                                                            // bus_serve.h (ADR-0002)
    
    uint16_t count = 0;
    uint8_t consecutive_writes = 0;
    const uint16_t cp = CONTROLPORT;
    
    while (1) {
        BUS_SERVE_ONE({
                    uint32_t t0 = timer_hw->timerawl;                           // T-50 : how long one API command
                    DSPHandler(cpuMemory + controlPort, cpuMemory);             // keeps us away from the bus
                    while (busPendingCommand) {                                 // T-82 : posted during a background read
                        busPendingCommand = false;
                        DSPHandler(cpuMemory + controlPort, cpuMemory);
                    }
                    t0 = timer_hw->timerawl - t0;
                    if (t0 > cmdMaxUs) cmdMaxUs = t0;
        })

        if (!count++) {
            uint32_t t0 = timer_hw->timerawl;                                   // T-50 : and how long the periodic
            DSPSync();                                                          // work does (keyboard, USB, blink)
            t0 = timer_hw->timerawl - t0;
            if (t0 > syncMaxUs) syncMaxUs = t0;
        }
    }
}

// ***************************************************************************************
//
//      Date        Revision
//      ====        ========
//		 13-03-26     Optimized main CPU loop and PIO state machine code
//
// ***************************************************************************************
