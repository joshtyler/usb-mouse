#include <MKL46Z4.H>
#include "FreeRTOS.h"
#include "task.h"

//Initialise the Green LED on dev board
void initLED(void);

//Task to blink the green LED
void blinkLED(void *pvParameters);

int main(void)
{
	//Blink an LED as a heartbeat
	initLED();
	xTaskCreate(blinkLED, (const char *)"Blink LED", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);
	
	
	//Start scheduler
	vTaskStartScheduler();

	
	return 0;
}

void initLED(void)
{
	// Inspired from example at https://eewiki.net/display/microcontroller/Getting+started+with+Freescale%27s+Freedom+KL46Z+Development+Board
	SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK; //Enable clock to PortD
	PORTD->PCR[5] = PORT_PCR_MUX(1u); //This sets the Mux control of PTD5 to 001, or GPIO
	PTD->PDDR |= (1u<<5); //Set PTD5 as output
}

void blinkLED(void *pvParameters)
{
	while(1)
	{
		PTD->PTOR = (1u<<5);
		vTaskDelay(500/portTICK_RATE_MS);
	}
}
