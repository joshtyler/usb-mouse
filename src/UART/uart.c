// Basic polling driver for Freescale KL46 family microcontroller
// This driver is TX only for now

// We want the UART which is routed through the SDA MCU on the FRDM-KL46Z
// This is PTA1 (UART0_RX) and PTA2 (UART0_TX) (FRDM-KL46Z user manual p.10)

#include <MKL46Z4.H> //For device registers
#include <stdlib.h> //For abs()
#include <stdbool.h> //For boolean type
#include "uart.h"

//Include FreeRTOS type for semaphopre handlig
#include "FreeRTOS.h"
#include "semphr.h"

//Sempahore to only allow one person to use the UART at once
static xSemaphoreHandle uartGatekeeper =0;

//Flag to signal if internal function has gatekeeper
//This means that uart_puts can take the gatekeeper, and uart_putchar will still write
static bool internalHasGatekeeper = false;

//Setup UART 0
//Argument is baud rate
void uart_init(int baud)
{
	uint16_t sbr; //Baud rate module divisor
	
	//Setup SIM (System Integration Module)
	SIM->SOPT2 |=  SIM_SOPT2_UART0SRC(1); //Set UART to use MCGPLLCLK/2 (Equal to system clock in our system)
	SIM->SCGC4 |= SIM_SCGC4_UART0_MASK; //Enable clock to UART0
	SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK; //Enable clock to PortA
	
	//Setup PORT (Port Control and Interrupts)
	PORTA->PCR[1] = PORT_PCR_MUX(1u<<1); //Set Mux control to 010. Alt 2 (UART0_RX)
	PORTA->PCR[2] = PORT_PCR_MUX(1u<<1); //Set Mux control to 010. Alt 2 (UART0_TX)
	PTA->PDDR |= (1u<<2); //Set PTA2 (UART0_TX) as output
	
	
	//Divisor = (module_clock)/(baud_rate * (OSR+1))
	//From reference manual p.762
	//However this gives wrong result!
	//Example code uses formula:
	//Divisor = (module_clock)/(baud_rate * (OSR))
	//This works...
	//Default OSR (Oversampling rate) is 16
	sbr = (SystemCoreClock/(baud * 16));
	
	//Due to integer division errors, pick the best of sbr and sbr+1
	if (calcBaudError(SystemCoreClock,sbr+1, 16, 115200) < calcBaudError(SystemCoreClock,sbr, 16, 115200))
	{
		sbr = sbr+1;
	}
	
	// Setup UART Peripheral
	// Lower nibble of BDH is upper 4 bits of BMD
	// We want other settings to be 0
	UART0->BDH = (uint8_t) ((sbr >> 8) & 0x1F);
	
	// BDL is lower 8 bits of bmd
	UART0->BDL = (uint8_t) (sbr & 0xFF);
	
	//Enable transmitter and receiver
	UART0->C2 |= (UART0_C2_TE_MASK | UART0_C2_RE_MASK);
	
	//Create gatekeepter
	uartGatekeeper = xSemaphoreCreateMutex();
	
}

//Send a character via the UART
void uart_putchar(char c)
{
	//Take and return the gatekeeper if not called internally
	if(!internalHasGatekeeper)
	{
		//No need to wrap this in an if, as we will wait forever to get the gatekeeper
		xSemaphoreTake(uartGatekeeper, portMAX_DELAY);
	}
	
	//Wait until there is room in the transmit buffer
	while(!(UART0->S1 & UART0_S1_TDRE_MASK));
	
	//Put data into UART data register for transmission
	UART0->D = (uint8_t) c;
	
	if(!internalHasGatekeeper)
	{
		xSemaphoreGive(uartGatekeeper);
	}

}

//Send a string via the UART
void uart_puts(const char *str)
{
	if(xSemaphoreTake(uartGatekeeper, portMAX_DELAY))
	{
		//Signal that an internal function has the gatekeeper, sot that uart_putchar won't block us
		internalHasGatekeeper = true;
		while(*str)
		{
			uart_putchar(*str);
			
			str++;
		}
		internalHasGatekeeper = false;
		xSemaphoreGive(uartGatekeeper);
	}
}

//Calculate the baud rate the chosen sbr will produce
static int calcBaudError(int clk, int sbr, int osr, int baud)
{
	int calcBaud = clk/(sbr*osr);
	return abs(calcBaud - baud);
}
