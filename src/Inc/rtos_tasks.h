//Copyright (c) 2016 Steven Yan and Joshua Lewis Tyler
//Licensed under the MIT license
//See LICENSE.txt

//Heartbeat task to show that system is still alive
//Blink LEDs and send UART message every second
//Also send system up message on boot
//Hardcoded for green LED (PTD5) on KL-46Z dev board

#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include "FreeRTOS.h"
#include "task.h"


//Datatype to be sent via mouseDataQueue
typedef struct
{
	int8_t x;
	int8_t y;
	int8_t scroll;
	uint8_t btn;
} mouseData_t;

//Datatype to be sent via peripheralReportQueue
typedef struct
{
	enum {TOUCH, ACCEL} source;
	int8_t payload1;
	int8_t payload2;
} peripheralData_t;

void heartbeat(void *pvParameters);
void gather(void *pvParameters);
void send(void *pvParameters);
void touch(void *pvParameters);
void accel(void *pvParameters);
void vApplicationMallocFailedHook( void );
void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName );
void lcd(void *pvParameters);

#endif
