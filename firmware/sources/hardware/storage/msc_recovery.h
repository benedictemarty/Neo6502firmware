//		T-168 : recovery of a USB key that stopped answering (msc_recovery.cpp)

#pragma once
#include <stdint.h>

bool MSCRecoverKey(uint8_t devAddr);  											// False : the key did not come back
bool MSCRecoveryHoldsMount(uint8_t devAddr);  									// Unmount callback : true, keep the volume
bool MSCRecoveryReturned(uint8_t devAddr);  									// Mount callback : true, the volume is still mounted

void MSCRecoveryWait(void);  													// Provided by the caller : run the USB stack once
void MSCRecoveryLost(uint8_t devAddr);  										// Provided by the caller : unmount a held volume
