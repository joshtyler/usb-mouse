//Device header
#include <MKL46Z4.H>

//FreeRTOS libraries
#include "FreeRTOS.h"
#include "task.h"

//User libraries
#include "uart.h" //UART Setup
#include "debug.h" //Debug print functions
#include "clock_config.h" //Clock configuration


//Initialise the Green LED on dev board
void initLED(void);


//Task to blink the green LED
void blinkLED(void *pvParameters);

//Test message processes
void msgProc1(void *p);
void msgProc2(void *p);

int main(void)
{
	//Setup clocking
	BOARD_BootClockRUN();
	SystemCoreClockUpdate();
	
	//Setup debug LED and UART
	initLED();
	uart_setup(115200);
	
	//Blink an LED as a heartbeat
	xTaskCreate(blinkLED, (const char *)"Blink LED", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);
	
	//Test message tasks
	xTaskCreate(msgProc1, (const char *)"Msg 1", configMINIMAL_STACK_SIZE, (void *)NULL, 1, NULL);
	xTaskCreate(msgProc2, (const char *)"Msg 2", configMINIMAL_STACK_SIZE, (void *)NULL, 2, NULL);
	
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
		dbg_putchar('.');
		dbg_puts("Heartbeat\r\n");
		vTaskDelay(500/portTICK_RATE_MS);
	}
}

void msgProc1(void *p)
{
	while(1)
	{
		dbg_puts("Message 1\r\n");
		vTaskDelay(230/portTICK_RATE_MS);
	}
}

void msgProc2(void *p)
{
	while(1)
	{
		dbg_puts("Important message 2\r\n");
		vTaskDelay(110/portTICK_RATE_MS);
	}
}
