//Device header
#include <MKL46Z4.H>

//FreeRTOS libraries
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

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

void gather(void *pvParameters);
void send(void *pvParameters);

//Queue to send mouse data from gather task to send task
xQueueHandle mouseDataQueue = NULL;

//Datatype to be send via queue
typedef struct
{
	int8_t x;
	int8_t y;
	int8_t scroll;
	uint8_t btn;
} mouseData_t;

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
	
	while(1)
	{
		lcd_setNum(touch_read());
		int i;
		for(i=0;i<999999;i++);
	}
	
	//Queue to transfer data from gather to send
	//Note queue only holds 1 item to ensure that data is up to date
	mouseDataQueue = xQueueCreate(1, sizeof(mouseData_t));
	
	//Heartbeat task
	//Blink LED and send UART message *at lowest priority* to indicate that we're still alive
	xTaskCreate(heartbeat, (const char *)"Heartbeat", configMINIMAL_STACK_SIZE, (void *)NULL, tskIDLE_PRIORITY, NULL);
	
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
	mouseData_t data = {0,0,0,0};
	while(1)
	{
		data.btn = usb_mouse_buttons(readSW1(), 0, readSW2(), 0, 0);
		xQueueSend(mouseDataQueue, &data, portMAX_DELAY); //Send data to queue, wait forever
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

