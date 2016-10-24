//Copyright (c) 2016 Steven Yan and Joshua Lewis Tyler
//Licensed under the MIT license
//See LICENSE.txt

//Capacitive touch strip driver for KL46Z development board

#ifndef TOUCH_H
#define TOUCH_H

#include <stdint.h>

//Electrode 1 is at PTA1
#define ELEC1_PORT PORTA
#define ELEC1_GPIO PTA
#define ELEC1_PIN 1
#define ELEC1_CHANNEL 9

//Electrode 2 is at PTA2
#define ELEC2_PORT PORTA
#define ELEC2_GPIO PTA
#define ELEC2_PIN 1
#define ELEC2_CHANNEL 10

void touch_init(void);
uint16_t touch_read(void);

#endif
