// Basic polling driver for Freescale KL46 family microcontroller

// We want the UART which is routed through the SDA MCU on the FRDM-KL46Z
// This is PTA1 (UART0_RX) and PTA2 (UART0_TX) (FRDM-KL46Z user manual p.10)

// This driver is TX only for now

#include <MKL46Z4.H>
#include "uart.h"


// Arguments are:
// UART Baud rate
void uartSetup(int baud)
{
	uint16_t sbr; //Baud rate module divisor
	
	
	//Setup SIM (System Integration Module)
	SIM->SOPT2 |=  SIM_SOPT2_UART0SRC(1); //Set UART to use PLLFLLCLK (System clock)
	
	SIM->SCGC4 |= SIM_SCGC4_UART0_MASK; //Enable clock to UART0
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK; //Enable clock to PortA
	
	//Setup PORT (Port Control and Interrupts)
	PORTA->PCR[1] = PORT_PCR_MUX(1u<<1); //Set Mux control to 010. Alt 2 (UART0_RX)
	PORTA->PCR[2] = PORT_PCR_MUX(1u<<1); //Set Mux control to 010. Alt 2 (UART0_TX)
	PTA->PDDR |= (1u<<2); //Set PTA2 UART0_TX as output
	
	
	//Divisor = (module_clock)/(baud_rate * (OSR+1))
	//From reference manual p.762
	//Default OSR (Oversampling rate) is 16
	sbr = (SystemCoreClock/(baud * 17));
	
	// Setup UART Peripheral
	// Lower nibble of BDH is upper 4 bits of BMD
	// We want other settings to be 0
	UART0->BDH = (uint8_t) ((sbr >> 8) & 0xF);
	
	// BDL is lower 8 bits of bmd
	UART0->BDL = (uint8_t) (sbr & 0xFF);
	
	//Enable transmitter and receiver
	UART0->C2 |= (UART0_C2_TE_MASK | UART0_C2_RE_MASK);
	
}


void uart_putChar(char c)
{
	//Wait until there is room in the transmit buffer
	while(!(UART0->S1 & UART0_S1_TDRE_MASK));
	
	//Put data into UART data register for transmission
	UART0->D = (uint8_t) c;
	
}
