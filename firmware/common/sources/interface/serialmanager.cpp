// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      serialmanager.cpp
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      25th January 2024
//      Reviewed :  No
//      Purpose :   Serial interface communication.
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"

static bool SERCommand(uint8_t command,uint8_t *data,uint8_t size);

static bool isInitialised = false;

// ***************************************************************************************
//
//								Attempt to initialise Serial I/O
//
// ***************************************************************************************

bool SERSetup(void) {
	if (!isInitialised) {  															// Try to initialise it.
	    isInitialised = SERInitialise();                                         	// Initialise serial port.
	    if (!isInitialised) {
	    	CONWriteString("Serial not functioning.\r");  							// Failed to initialise.
	    	return false; 												
	    }
	    while (SERIsByteAvailable()) SERReadByte();  								// Clear anything already incoming.
	}
	SERSetSerialFormat(SERIAL_TRANSFER_BAUD_RATE,SERIAL_PROTOCOL_8N1); 				// Set transceive format.
	return true;
}

// ***************************************************************************************
//
//									Input and process buffer.
//
// ***************************************************************************************

static uint8_t sBuffer[256];  														// Input buffer.

void SERCheckDataAvailable(void) {

	if (!SERSetup()) return; 														// Initialise serial I/O if not done
	bool completed = false;
	CONWriteString("Serial link enabled.\r");
	while (!completed) {
		KBDSync();  																// Update USB stuff 
		if (cpuMemory[controlPort+3] & 0x80) {  									// ESC pressed ?
			completed = true;
		}
		if (SERIsByteAvailable()) {   												// Something waiting.
			sBuffer[0] = SERReadByte();  											// Read length of data
			sBuffer[1] = SERReadByte();  											// Read checksum
			uint8_t checksum = 0;
			//		T-60 : sBuffer holds 256 bytes and the length byte goes up to 255, so
			//		sBuffer[i+2] reached sBuffer[256] — one past the end. Everything here comes
			//		from whatever is sending on the serial line.
			uint8_t toRead = sBuffer[0];
			if (toRead > sizeof(sBuffer)-2) toRead = sizeof(sBuffer)-2;
			for (int16_t i = 0;i < toRead;i++) {  									// Read in data and calculate checksum
				sBuffer[i+2] = SERReadByte();
				checksum += sBuffer[i+2];			
			}
			if (sBuffer[1] == checksum) {  											// Checksum okay.
				completed = SERCommand(sBuffer[2],sBuffer+3,sBuffer[0]-1); 			// Do command.
			} else {
				CONWriteString("Serial checksum error.\r");  
				completed = true;
			}
		}
	}
	CONWriteString("Serial link disabled.\r");
}

// ***************************************************************************************
//
//									Handle serial command
//
// ***************************************************************************************

static uint8_t *dataPtr = NULL;  												// Where we write stuff
static uint8_t *dataEnd = NULL;  												// T-60 : and where the area ends
static uint16_t startAddress,currentAddress;  									// Start and current address accessing.
static uint16_t saveAddress,saveSize;  											// For writing.
static char fileName[32]; 														// File name

static bool SERCommand(uint8_t command,uint8_t *data,uint8_t size) {
	uint16_t a;
	switch (command) {
		case 0:  																// 0 detach
			break;
		case 1:  																// 1 data transmit
			if (dataPtr != NULL) {  											// Address assigned ?
				for (uint8_t i = 0;i < size;i++) { 								// Copy data if so
					if (dataPtr >= dataEnd) break;  							// T-60 : stop at the end of the area
					*dataPtr++ = data[i];
					currentAddress++;
				}
			}
			break;
		case 2: 																// 2 set 6502 memory address
		case 3: 																// 3 set gfxmemory address
			currentAddress = startAddress = data[0] + (data[1] << 8);
			//		T-60 : the graphics area is half the 16 bit range, so an address above
			//		32 Ko pointed outside it, and the writes below followed.
			if (command == 3 && currentAddress >= GFX_MEMORY_SIZE) { dataPtr = NULL;break; }
			dataPtr = (command == 2) ? cpuMemory+currentAddress:gfxObjectMemory+currentAddress;
			dataEnd = (command == 2) ? cpuMemory+MEMORY_SIZE : gfxObjectMemory+GFX_MEMORY_SIZE;
			break;
		case 4:  																// 4 set 6502 memory address indirect
			a = data[0] + (data[1] << 8);  										// Get address from here.
			currentAddress = startAddress = cpuMemory[a] + (cpuMemory[a+1]<<8); // Get the actual address
			dataPtr = cpuMemory + currentAddress;   							// Initialise the pointer
			dataEnd = cpuMemory + MEMORY_SIZE;  								// T-60
			break;
		case 5:
			KBDInsertQueue(data[0]);  											// 5 Insert key in keyboard queue
			break;
		case 6:  																// 6 Set save address
			saveAddress = data[0]+(data[1] << 8);
			saveSize = data[2]+(data[3] << 8);
			//CONWriteString("%d %d\r",saveAddress,saveSize);
			break;
		case 7:  																// 7 Save file.			
			//		T-60 : fileName holds 32 bytes and the length byte goes up to 255.
			{
				uint8_t nameLen = data[0];
				if (nameLen > sizeof(fileName)-1) nameLen = sizeof(fileName)-1;
				for (int i = 0;i < nameLen;i++) fileName[i] = data[i+1];  		// Make C String
				fileName[nameLen] = '\0';
			}
			FIOWriteFile(fileName,saveAddress,saveSize);   						// Save it
			CONWriteString("Saved '%s' (%d bytes)\r",fileName,saveSize); 		// Announce it.
			break;
	}
	return (command == 0);
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//
// ***************************************************************************************
