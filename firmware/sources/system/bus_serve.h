// ***************************************************************************************
//
//      Name :      bus_serve.h
//      Author :    bmarty (bmarty@mailo.com)
//      Purpose :   ADR-0002 (T-82) : the body of the 65C02 bus loop, as a macro, so that the main
//                  loop (CPUExecute) and the wait for a USB transfer during a background read
//                  serve the bus with the SAME code — same PIO timing, nop padding included, and
//                  the IRQB release on the $FFFF vector fetch. A macro rather than an inline
//                  function : CPUExecute expands to exactly the text it had, so its machine code
//                  does not change (checked by disassembly, T-46/T-48 lesson).
//
//                  Needs in scope : union u32 value, uint8_t consecutive_writes, const uint16_t cp.
//                  the macro argument is the statement run when the 65C02 writes the control port.
//
// ***************************************************************************************

#pragma once

union u32 {
    uint32_t value;
    struct {
        uint16_t address;
        uint8_t flags;
    } data;
};

#define BUS_SERVE_ONE(...) \
        /* Ensures synchronization with the PIO during W65C02 interrupts */                                \
        if(consecutive_writes < 3) {                                                                       \
            value.value = pio_sm_get(pio1, 0);                                                             \
        } else {                                                                                           \
            consecutive_writes = 0;                                                                        \
            value.value = pio_sm_get_blocking(pio1, 0);                                                    \
        }                                                                                                  \
                                                                                                           \
        if (value.data.flags & 0x8) { /* 65C02 Read */                                                     \
            pio_sm_put(pio1, 0, cpuMemory[value.data.address]);                                            \
                                                                                                           \
            consecutive_writes = 0;                                                                        \
                                                                                                           \
            /* F-60 : vector fetch ($FFFF) of an interrupt sequence releases IRQB (bmarty). */             \
            /* Cost ~3 cycles on every read : the nop padding below was 14, now 11 (R9 in F-60 notes). */  \
            if (value.data.address == 0xFFFF && irqAsserted) {                                             \
                irqAsserted = false;                                                                       \
                wdc65C02cpu_set_irq(false);                                                                \
            }                                                                                              \
                                                                                                           \
            __asm volatile (                                                                               \
              "nop; nop; nop; nop\n\t"                                                                     \
              "nop; nop; nop; nop\n\t"                                                                     \
              "nop; nop; nop\n\t"                                                                          \
              ::: "memory"                                                                                 \
            );                                                                                             \
                                                                                                           \
        } else { /* 65C02 Write */                                                                         \
            consecutive_writes++;                                                                          \
                                                                                                           \
            /* Safe to do without blocking since the PIO state machine always finishes first */            \
            cpuMemory[value.data.address] = pio_sm_get(pio1, 0);                                           \
                                                                                                           \
            if ((uint8_t)value.value == 0x00) {                                                            \
                if (value.data.address == cp) {                                                            \
                    __VA_ARGS__                                                                        \
                }                                                                                          \
            }                                                                                              \
                                                                                                           \
            __asm volatile (                                                                               \
              "nop\n"                                                                                      \
              ::: "memory"                                                                                 \
            );                                                                                             \
        }
