//Heartbeat task to show that system is still alive
//Blink LED and send UART message every second
//Also send system up message on boot
//Hardcoded for green LED (PTD5) on KL-46Z dev board

#include "heartbeat.h"
#include "debug.h"
#include <MKL46Z4.H>

#include "FreeRTOS.h"
#include "task.h"


void led_init(void)
{
	//Debug LED is at PTD5 on KL46z dev board
	SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK; //Enable clock to PortD
	PORTD->PCR[5] = PORT_PCR_MUX(1); //Set Mux control of PTD5  or GPIO
	PTD->PDDR |= (1u<<5); //Set PTD5 as output
}

void heartbeat(void *pvParameters)
{
	dbg_puts("USB Mouse begin.\r\n");
	while(1)
	{
		PTD->PTOR = (1u<<5);
		dbg_puts("Heartbeat\r\n");
		vTaskDelay(1000/portTICK_RATE_MS);
	}
}
