//Copyright (c) 2016 Steven Yan and Joshua Lewis Tyler
//Licensed under the MIT license
//See LICENSE.txt

//Tasks to run as FreeRTOS tasks

#include "rtos_tasks.h"


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
#include "uart.h" //UART Setup
#include "debug.h" //Debug print functions
#include "clock_config.h" //Clock configuration
#include "usb_dev.h" //USB Initialisation
#include "usb_mouse.h" //Mouse user interface
#include "gpio.h" //To read buttons
#include "lcd.h" //Drive LCD
#include "touch.h" //Read touch sensor
#include "iic.h" //Read accelerometer
#include "filter.h" //Filter data


//Queue to send mouse data from gather task to send task
xQueueHandle mouseDataQueue = NULL;

//Queue to send peripheral reports to gather task
xQueueHandle peripheralReportQueue = NULL;

//Signals to request a report from each peripheral
static xSemaphoreHandle scrollReportSignal = 0;
static xSemaphoreHandle accelReportSignal = 0;




//Heartbeat task to show that system is still alive
//Blink LEDs and send UART message every second
//Also send system up message on boot
//Hardcoded for green LED (PTD5) on KL-46Z dev board
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


void gather(void *pvParameters)
{
	//Create semaphores which request data
	scrollReportSignal = xSemaphoreCreateBinary();
	accelReportSignal = xSemaphoreCreateBinary();
	
	peripheralData_t periphData;
	mouseData_t data = {0,0,0,0};
	while(1)
	{
		xSemaphoreGive(scrollReportSignal);
		xSemaphoreGive(accelReportSignal);
		for(int i=0; i<2; i++)
		{
			xQueueReceive(peripheralReportQueue, &periphData, portMAX_DELAY);
			if(periphData.source == TOUCH)
			{
				data.scroll = periphData.payload1;
			} else if(periphData.source == ACCEL) {
				data.x = periphData.payload1;
				data.y = periphData.payload2;
			} else {
				//Invalid
				dbg_puts("Invalid peripheral.\r\n");
			}
		}
		data.btn = usb_mouse_buttons(readSW1(), 0, readSW2(), 0, 0);
		xQueueSend(mouseDataQueue, &data, portMAX_DELAY); //Send data to queue, wait forever for it to be accepted
		
		vTaskDelay(10/portTICK_RATE_MS);
	}
}

void send(void *pvParameters)
{
	mouseData_t data;
	while(1)
	{
		xQueueReceive(mouseDataQueue, &data, portMAX_DELAY); //Send data to queue, wait forever
		usb_mouse_send_data(data.x, data.y, data.scroll, 0, data.btn);
	}
}


//Constantly take measurements of position on capacitive touch sensor
//Keep a record of distance scrolled since last report
void touch(void *pvParameters)
{
	const uint16_t minTouchThreshold = 150; //This is emperically a good minimum value for the user touching the strip
	const uint16_t maxTouchThreshold = 1500; //This is emperically a good maximum value for the user touching the strip
	const uint16_t maxDist = 50; //When we remove our finger, we will get a sharp transition. Ignore this!
	
	//The strip tends to produce values in the range 100-1000. %8 scales this nicely into ~128 for a full swipe. This is the full scale of 
	const uint8_t scalingFactor = 16;
	
	//Minimum number of touches before adding to diff
	//This stops touches being counted as movement
	const uint8_t minTouches = 100;
	uint8_t noTouches =0;
	
	const TickType_t delay = 2/portTICK_RATE_MS; //Measure every 2ms
	
	struct
	{
		bool touched; //True if the button was pressed at the last measurement
		filterHandle_t filter; //Filter to filter and hold
	} meas = {false, { {0}, 0, 0}}, prevMeas = {false, { {0}, 0, 0}};

	int32_t distance;
	
	while(1)
	{
		//Get measurement
		uint16_t val = touch_read();
		
		//Check if touched
		meas.touched = (val > minTouchThreshold && val < maxTouchThreshold ? true: false);
		
		if(!meas.touched)
		{
			val =0;
			noTouches =0;
		} else if(noTouches < minTouches) {
			noTouches++;
		}
		
		//Filter values
		movingAverageAddSample(&meas.filter, val);
		
		if(noTouches >= minTouches)
		{
			int32_t diff = ( (int32_t)meas.filter.curVal - (int32_t)prevMeas.filter.curVal);
			//Add up differences
			if(diff < maxDist && -diff < maxDist )
			{
				distance += diff;
			}
		}
		
		prevMeas = meas;
		
		//Wait to be asked to report distance.
		//If we're not asked to, go back around the loop
		if(xSemaphoreTake(scrollReportSignal, delay))
		{
			prevMeas.touched = false;
			
			distance = distance/scalingFactor;
			//Clamp to limits of int8_t
			if(distance > INT8_MAX)
			{
				distance = INT8_MAX;
			}
			if(distance < INT8_MIN)
			{
				distance = INT8_MIN;
			}
			
			peripheralData_t tx_data = {TOUCH, (int8_t)distance,0};
			distance = 0;
			
			xQueueSend(peripheralReportQueue, &tx_data, portMAX_DELAY);
		}
	}
}

void accel(void *pvParameters)
{
	const TickType_t delay = 2/portTICK_RATE_MS; //Measure every 2ms
	
	while(1)
	{
		int16_t x,y;
		
		readAccel(&x, &y);
		
		//Scale data
		x >>= 8;
		y >>= 8; 
		
		peripheralData_t tx_data = {ACCEL, (int8_t)x, (int8_t)y};
		
		if(xSemaphoreTake(accelReportSignal, delay))
		{

			xQueueSend(peripheralReportQueue, &tx_data, portMAX_DELAY);
		}
	}
}

void lcd(void *pvParameters)
{
	const char* strings[] = {"USB Mouse", "University of Southampton","Real-Time Computing and Embedded Systems","2016"};
	const TickType_t wordDelay = 1500/portTICK_RATE_MS;
	const TickType_t scrollDelay = 500/portTICK_RATE_MS;
	const unsigned int numStrings = sizeof(strings)/sizeof(strings[1]);
		
	while(1)
	{
		for(int i=0; i< numStrings; i++)
		{
			for(int strPos=0; ; strPos++)
			{
				if(lcd_setStr( (const char *) (strings[i] + strPos) ))
				{
					break;
				}
				vTaskDelay(scrollDelay);
			}
			vTaskDelay(wordDelay);
		}
	}
}

void vApplicationMallocFailedHook( void )
{
	dbg_puts("Malloc failed\r\n");
	while(1);
}


void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
	dbg_puts("Stack overflow\r\n");
	while(1);
}
