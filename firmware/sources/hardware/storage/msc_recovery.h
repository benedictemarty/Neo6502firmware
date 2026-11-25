//		T-168 : Reset Recovery of a USB key (msc_recovery.cpp)

#pragma once
#include <stdint.h>

bool MSCResetRecovery(uint8_t devAddr);  										// False : the key did not recover
void MSCRecoveryWait(void);  													// Provided by the caller : run the USB stack once
