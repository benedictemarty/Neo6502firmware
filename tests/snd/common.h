// Stub de common.h pour compiler sndcreator.cpp sur PC (T-80, T-79).
#include <stdint.h>
#include <stdbool.h>
#define __time_critical_func(x) x
#define __not_in_flash(group)
extern uint8_t cpuMemory[];
extern uint8_t gfxObjectMemory[];                                               // T-119 : source de 8,17
#define GFX_MEMORY_SIZE 0x8000
#include "interface/sound.h"
