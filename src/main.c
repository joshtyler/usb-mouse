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

void gather(void *pvParameters);
void send(void *pvParameters);
void touch(void *pvParameters);
void vApplicationMallocFailedHook( void );
void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName );

//Queue to send mouse data from gather task to send task
xQueueHandle mouseDataQueue = NULL;

//Signals to request a report from each
static xSemaphoreHandle scrollReportSignal = 0;

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
	int8_t payload;
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
	iic_init();
	
	//Queue to transfer data from gather to send
	//Note queue only holds 1 item to ensure that data is up to date
	mouseDataQueue = xQueueCreate(1, sizeof(mouseData_t));
	
	//Queue to hold reports from peripherals before being assembled by gather
	//Queue is large enough to hold one report from each peripheral
	peripheralReportQueue = xQueueCreate(2, sizeof(peripheralData_t));

	//Heartbeat task
	//Blink LED and send UART message *at lowest priority* to indicate that we're still alive
	xTaskCreate(heartbeat, (const char *)"Heartbeat", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);
	
	//Touch task
	//Read touch sensor
	xTaskCreate(touch, (const char *)"Touch", configMINIMAL_STACK_SIZE, (void *)NULL, configMAX_PRIORITIES-2, NULL);
	
	//Gather task
	//Get sensor data and send to send task
	xTaskCreate(gather, (const char *)"Gather", configMINIMAL_STACK_SIZE, (void *)NULL, configMAX_PRIORITIES-1, NULL);
	
	//Send task
	//Send mouse data via USB
	xTaskCreate(send, (const char *)"Send", configMINIMAL_STACK_SIZE, (void *)NULL, configMAX_PRIORITIES, NULL);

	
	vTaskStartScheduler();

	
	return 0;
}

void gather(void *pvParameters)
{
	//Create semaphores which request data, and take them
	dbg_puts("Gather. Creating semaphore.\r\n");
	scrollReportSignal = xSemaphoreCreateBinary();
	
	peripheralData_t periphData;
	mouseData_t data = {0,0,0,0};
	dbg_puts("Gather. About to enter main loop");
	while(1)
	{
		dbg_puts("Gather. In main loop.\r\n");
		xSemaphoreGive(scrollReportSignal);
		dbg_puts("Gather. Given semaphore.\r\n");
		xQueueReceive(peripheralReportQueue, &periphData, portMAX_DELAY);
		dbg_puts("Gather. Got peripheral data.\r\n");
		data.scroll = periphData.payload;
		lcd_setNum(periphData.payload >= 0? periphData.payload : -periphData.payload);
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
	const uint16_t touchThreshold = 100; //This is emperically a good minimum value for te user touching the strip
	const uint16_t maxDist = 75; //When we remove our finger, we will get a sharp transition. Ignore this!
	
	//The strip tends to produce values in the range 100-1000. %8 scales this nicely into ~128 for a full swipe. This is the full scale of 
	const uint8_t scalingFactor = 0x8; 
	
	const TickType_t delay = 2/portTICK_RATE_MS; //Measure every 2ms
	
	struct
	{
		bool touched; //True if the button was pressed at the last measurement
		uint16_t val; //Last measurement
	} meas = {false, 0}, prevMeas = {false, 0};
	
	int32_t distance;
	
	while(1)
	{
//		dbg_puts("Touch. In main loop.\r\n");
		//Get measurement
		meas.val = touch_read();
		
		//Check if touched
		meas.touched = (meas.val > touchThreshold? true: false);
		
		if(prevMeas.touched)
		{
			int32_t diff = ( (int32_t)meas.val - (int32_t)prevMeas.val);
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
			dbg_puts("Touch. Taken Semaphore.\r\n");
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
			
			peripheralData_t tx_data = {TOUCH, (int8_t)distance};
			distance = 0;
			
			xQueueSend(peripheralReportQueue, &tx_data, portMAX_DELAY);
			dbg_puts("Touch. Sent to queue.\r\n");
		}
	}
}

void vApplicationMallocFailedHook( void )
{
	
}


void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
	
}

