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
#include "heartbeat.h" //Heartbeat task
#include "uart.h" //UART Setup
#include "debug.h" //Debug print functions
#include "clock_config.h" //Clock configuration
#include "usb_dev.h" //USB Initialisation
#include "usb_mouse.h" //Mouse user interface
#include "gpio.h" //To read buttons
#include "lcd.h" //Drive LCD
#include "touch.h" //Read touch sensor
#include "iic.h" //Read accelerometer
#include "filter.h"

#define STACK_SIZE		( ( unsigned short ) 128 )

void gather(void *pvParameters);
void send(void *pvParameters);
void touch(void *pvParameters);
void accel(void *pvParameters);
void vApplicationMallocFailedHook( void );
void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName );
void lcd(void *pvParameters);

//Queue to send mouse data from gather task to send task
xQueueHandle mouseDataQueue = NULL;

//Signals to request a report from each peripheral
static xSemaphoreHandle scrollReportSignal = 0;
static xSemaphoreHandle accelReportSignal = 0;

xQueueHandle peripheralReportQueue = NULL;

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
		
		x*= .004;
		y*= .003;
		
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

