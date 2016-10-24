//Copyright (c) 2016 Steven Yan and Joshua Lewis Tyler
//Licensed under the MIT license
//See LICENSE.txt

//Device header
#include <MKL46Z4.H>

//Standard headers
#include <stdint.h>
#include <stdbool.h>

//FreeRTOS libraries
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


//User libraries
#include "rtos_tasks.h"
#include "uart.h" //UART Setup
#include "debug.h" //Debug print functions
#include "clock_config.h" //Clock configuration
#include "usb_dev.h" //USB Initialisation
#include "gpio.h" //To read buttons
#include "lcd.h" //Drive LCD
#include "touch.h" //Read touch sensor
#include "iic.h" //Read accelerometer
#include "filter.h" //Filter data

#define STACK_SIZE		( ( unsigned short ) 128 )
	

//Queues. Defined in rtos_tasks.c
//Queue to send mouse data from gather task to send task
extern xQueueHandle mouseDataQueue;
//Queue to send peripheral reports to gather task
extern xQueueHandle peripheralReportQueue;

int main(void)
{
	//Setup clocking
	BOARD_BootClockRUN();
	SystemCoreClockUpdate();
	
	//Setup peripherals
	gpio_init();
	uart_init(115200);
	usb_init();
	lcd_init();
	touch_init();
	
	BOARD_I2C_ReleaseBus();
	BOARD_I2C_ConfigurePins();
	
	//Queue to transfer data from gather to send
	//Note queue only holds 1 item to ensure that data is up to date
	mouseDataQueue = xQueueCreate(1, sizeof(mouseData_t));
	
	//Queue to hold reports from peripherals before being assembled by gather
	//Queue is large enough to hold one report from each peripheral
	peripheralReportQueue = xQueueCreate(2, sizeof(peripheralData_t));

	//Heartbeat task
	//Blink LED and send UART message *at lowest priority* to indicate that we're still alive
	xTaskCreate(heartbeat, (const char *)"Heartbeat", STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);
	
	//LCD task
	//Display strings on LCD
	xTaskCreate(lcd, (const char *)"LCD", STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);
	
	//Touch task
	//Read touch sensor
	xTaskCreate(touch, (const char *)"Touch", STACK_SIZE, (void *)NULL, configMAX_PRIORITIES-2, NULL);
	
	//Accel task
	//Read accelerometer
	xTaskCreate(accel, (const char *)"Accel", STACK_SIZE, (void *)NULL, configMAX_PRIORITIES-2, NULL);
	
	//Gather task
	//Get sensor data and send to send task
	xTaskCreate(gather, (const char *)"Gather", STACK_SIZE, (void *)NULL, configMAX_PRIORITIES-1, NULL);
	
	//Send task
	//Send mouse data via USB
	xTaskCreate(send, (const char *)"Send", STACK_SIZE, (void *)NULL, configMAX_PRIORITIES, NULL);

	vTaskStartScheduler();

	return 0;
}


