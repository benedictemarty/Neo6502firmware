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
#define BUS_FDEBUG_RXSTALL (1u << 0)   										// SM0 : PIO_FDEBUG RXSTALL[3:0]

static uint32_t busStallTx = 0,busStallRx = 0;

void __not_in_flash_func(HWBusProbe)(void) {
	uint32_t flags = pio1->fdebug & (BUS_FDEBUG_TXSTALL | BUS_FDEBUG_RXSTALL);
	if (flags == 0) return;
	if (flags & BUS_FDEBUG_TXSTALL) busStallTx++;
	if (flags & BUS_FDEBUG_RXSTALL) busStallRx++;
	pio1->fdebug = flags;  													// Sticky : write 1 to clear
}

uint32_t HWBusStalls(uint8_t which) {  										// 0 = TX (read not ready), 1 = RX (address FIFO)
	return (which == 0) ? busStallTx : busStallRx;
}

void HWBusStallsReset(void) {
	busStallTx = busStallRx = 0;
	pio1->fdebug = BUS_FDEBUG_TXSTALL | BUS_FDEBUG_RXSTALL;
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

    union u32
    {
        uint32_t value;
        struct {
            uint16_t address;
            uint8_t flags;
        } data;
    } value;
    
    uint16_t count = 0;
    uint8_t consecutive_writes = 0;
    const uint16_t cp = CONTROLPORT;
    
    while (1) {
        // Ensures synchronization with the PIO during W65C02 interrupts
        if(consecutive_writes < 3) {
            value.value = pio_sm_get(pio1, 0);
        } else {
            consecutive_writes = 0;
            value.value = pio_sm_get_blocking(pio1, 0);
        }
      
        if (value.data.flags & 0x8) { // 65C02 Read
            pio_sm_put(pio1, 0, cpuMemory[value.data.address]);
            
            consecutive_writes = 0;

            // F-60 : vector fetch ($FFFF) of an interrupt sequence releases IRQB (bmarty).
            // Cost ~3 cycles on every read : the nop padding below was 14, now 11 (R9 in F-60 notes).
            if (value.data.address == 0xFFFF && irqAsserted) {
                irqAsserted = false;
                wdc65C02cpu_set_irq(false);
            }
            
            __asm volatile (
              "nop; nop; nop; nop\n\t"
              "nop; nop; nop; nop\n\t"
              "nop; nop; nop\n\t"
              ::: "memory"
            );
          
        } else { // 65C02 Write
            consecutive_writes++;
            
            // Safe to do without blocking since the PIO state machine always finishes first
            cpuMemory[value.data.address] = pio_sm_get(pio1, 0);
            
            if ((uint8_t)value.value == 0x00) {
                if (value.data.address == cp) {
                    DSPHandler(cpuMemory + controlPort, cpuMemory);
                }
            }
            
            __asm volatile (
              "nop\n"
              ::: "memory"
            );
        }
        
        if (!count++) {
            DSPSync();
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
