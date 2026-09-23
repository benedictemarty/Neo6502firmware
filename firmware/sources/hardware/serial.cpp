// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      serial.cpp
//      Authors :   Paul Robson (paul@robsons.org.uk)
//      Date :      24th January 2024
//      Reviewed :  No
//      Purpose :   Serial interface
//
// ***************************************************************************************
// ***************************************************************************************

#include "common.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

#define UART_ID uart0

#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE

#define UART_TX_PIN 28
#define UART_RX_PIN 29

#define UART_RX_BUFFER_SIZE 	(2048)

static uint8_t rxBuffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t writeAddress;
static volatile uint16_t readAddress;

// ***************************************************************************************
//
//								Serial Port Receive
//
// ***************************************************************************************

void SERReceiveInterrupt() {
	while (uart_is_readable(UART_ID)) {
		rxBuffer[writeAddress] = uart_getc(UART_ID);
		writeAddress = (writeAddress + 1) & (UART_RX_BUFFER_SIZE-1);
		if (writeAddress == readAddress) {
			CONWriteString("Warning: Serial RX Buffer Overflow\r");
		}
	}
}

// ***************************************************************************************
//
//								Initialise the serial port
//
// ***************************************************************************************

bool SERInitialise(void) {
	SERSetSerialFormat(9600,0);
	return true;
}

// ***************************************************************************************
//
//					Set the Serial Protocol. Only 8N1 currently supported.
//
// ***************************************************************************************

static uint32_t currentBaudRate = 0;

//
//		The UART divisors are derived from clk_peri, which follows clk_sys : after a
//		display mode switch (252 <-> 270 MHz) the baud rate must be programmed again.
//
void SERClockChanged(void) {
	if (currentBaudRate != 0) uart_set_baudrate(UART_ID,currentBaudRate);
}

void SERSetSerialFormat(uint32_t baudRate,uint32_t protocol) {
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;
    currentBaudRate = baudRate;
    irq_set_enabled(UART_IRQ, false);

	uart_init(UART_ID, baudRate);
	gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
	gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
	uart_set_hw_flow(UART_ID, false, false);
	uart_set_fifo_enabled(UART_ID, false);
	uart_set_baudrate(UART_ID,baudRate);
	uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

	readAddress = 0;
	writeAddress = 0;

    irq_set_exclusive_handler(UART_IRQ, SERReceiveInterrupt);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);
}

// ***************************************************************************************
//
//						Check if byte is available from UART 
//
// ***************************************************************************************

bool SERIsByteAvailable(void) {
	//if (readAddress != writeAddress) CONWriteString("Avail: %d %d\r",readAddress,writeAddress);
	return readAddress != writeAddress;
}

// ***************************************************************************************
//
//								Read Byte from UART
//
// ***************************************************************************************

//		T-59 : this waited for ever. A program that reads the serial port with nothing coming
//		froze the whole machine — the bus loop is blocked, so even the reset key combination
//		stops answering. The file's own history claims "29-01-24 Added timeout to readbyte",
//		but no such timeout was in the code. Bounded to 100 ms, as ADR-0001 (T-34) requires of
//		every wait ; returns 0 on timeout, and 10,1 tells whether a byte is actually there.
uint8_t SERReadByte(void) {
	uint32_t t0 = TMRRead();  													// 100 Hz timer
	while (readAddress == writeAddress) {
		if (TMRRead() - t0 >= 10) return 0;
	}
	//CONWriteString("Read2: %d %d\r",readAddress,writeAddress);
	uint8_t b = rxBuffer[readAddress];
	readAddress = (readAddress+1) & (UART_RX_BUFFER_SIZE-1);
	return b;
}

// ***************************************************************************************
//
//								Write Byte to UART
//
// ***************************************************************************************

void SERWriteByte(uint8_t b) {
	uart_putc_raw(UART_ID,b);
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//		29-01-24 	Added timeout to readbyte.
//		22-03-24 	Interrupt driven Receive buffer.
//
// ***************************************************************************************
