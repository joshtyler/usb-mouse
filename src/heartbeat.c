//Copyright (c) 2016 Steven Yan and Joshua Lewis Tyler
//Licensed under the MIT license
//See LICENSE.txt

//Heartbeat task to show that system is still alive
//Blink LEDs and send UART message every second
//Also send system up message on boot
//Hardcoded for green LED (PTD5) on KL-46Z dev board

#include "heartbeat.h"
#include "debug.h"
#include "gpio.h"

#include "FreeRTOS.h"
#include "task.h"


void heartbeat(void *pvParameters)
{
	dbg_puts("USB Mouse begin.\r\n");
	setLED1();
	clearLED2();
	while(1)
	{
		toggleLED1();
		toggleLED2();
		dbg_puts("Heartbeat\r\n");
		vTaskDelay(1000/portTICK_RATE_MS);
	}
}
