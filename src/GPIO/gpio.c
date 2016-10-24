//Copyright (c) 2016 Steven Yan and Joshua Lewis Tyler
//Licensed under the MIT license
//See LICENSE.txt

//Routines to use the buttons and LEDs on the KL46Z dev board

#include <stdbool.h>
#include <MKL46Z4.H>
#include "gpio.h"

void gpio_init(void)
{
	//LEDs
	init_pin(LED1_PORT, LED1_PIN, true, false, 1);
	init_pin(LED2_PORT, LED2_PIN, true, false, 1);
	
	//Switches
	//Enable internal pullups as there are none on PCB
	init_pin(SW1_PORT, SW1_PIN, false, true, 1);
	init_pin(SW2_PORT, SW2_PIN, false, true, 1);
	
}

void init_pin(PORT_Type * port, uint8_t pin, bool output, bool pullup, uint8_t alt)
{
	unsigned int mask;
	GPIO_Type *pt;
	
	switch((uint32_t) port)
	{
		case PORTA_BASE:
			mask = SIM_SCGC5_PORTA_MASK;
			pt = PTA;
			break;
		case PORTB_BASE:
			mask = SIM_SCGC5_PORTB_MASK;
			pt = PTB;
		break;
		case PORTC_BASE:
			mask = SIM_SCGC5_PORTC_MASK;
			pt = PTC;
		break;
		case PORTD_BASE:
			mask = SIM_SCGC5_PORTD_MASK;
			pt = PTD;
		break;
		case PORTE_BASE:
			mask = SIM_SCGC5_PORTE_MASK;
			pt = PTE;
		break;
		
		default:
			//Invalid port
			while(1);	
	}
	
	SIM->SCGC5 |= mask; //Enable clock to port
	port->PCR[pin] = PORT_PCR_MUX(alt); //Set mux control

	
	//Set PDDR bit if output, or clear if input
	if(output)
	{
		pt->PDDR |= (1u<<pin);
	} else {
		pt->PDDR &= ~(1u<<pin);
	}
	
	//Condtionally set pullup
	if(pullup)
	{
		port->PCR[pin] |= (PORT_PCR_PE_MASK | PORT_PCR_PS_MASK);
	} else {
		port->PCR[pin] &= ~PORT_PCR_PE_MASK;
	}
	
}
