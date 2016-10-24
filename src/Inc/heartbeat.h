//Copyright (c) 2016 Steven Yan and Joshua Lewis Tyler
//Licensed under the MIT license
//See LICENSE.txt

//Heartbeat task to show that system is still alive
//Blink LEDs and send UART message every second
//Also send system up message on boot
//Hardcoded for green LED (PTD5) on KL-46Z dev board

#ifndef HEARTBEAT_H
#define HEARTBEAT_H

void led_init(void);
void heartbeat(void *pvParameters);

#endif
