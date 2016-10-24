//Copyright (c) 2016 Steven Yan and Joshua Lewis Tyler
//Licensed under the MIT license
//See LICENSE.txt

//Capacitive touch strip driver for KL46Z development board

#include "touch.h"
#include "gpio.h"
#include <MKL46Z4.H>

static uint16_t calibrationValue =0;

void touch_init(void)
{
	//Init pins
	init_pin(ELEC1_PORT, ELEC1_PIN, 0, 0, 0);
	init_pin(ELEC2_PORT, ELEC2_PIN, 0, 0, 0);
	
	//Enable clock to module
	SIM->SCGC5 |= SIM_SCGC5_TSI_MASK;
	
	//Set threshold
	TSI0->TSHD =0; //0 - always wakeup
	
	//Set configuration register
	//Divide clock frequency by 2
	//Reference oscillator charge current 4uA
	//Electode oscillator charge current 500nA
	//4 scans per electrode
	//Capacitive sensing mode
	//Voltage rail config 0
	TSI0->GENCS = TSI_GENCS_PS(1) | TSI_GENCS_REFCHRG(3) | TSI_GENCS_EXTCHRG(0) | TSI_GENCS_NSCN(4) | TSI_GENCS_MODE(0) | TSI_GENCS_DVOLT(0);
	
	//Enable module
	TSI0->GENCS |= TSI_GENCS_TSIEN_MASK;
	
	//Set measured channel
	TSI0->DATA = TSI_DATA_TSICH(ELEC1_CHANNEL);
	
	//Calibrate module
	//(assume nobody is touching the moudle at init)
	calibrationValue = touch_read();
}

uint16_t touch_read(void)
{
	//Start software triggered measurement
	TSI0->DATA |= TSI_DATA_SWTS_MASK;
	
	//Wait for measurement to complete
	while (!(TSI0->GENCS & TSI_GENCS_EOSF_MASK));
	
	//Get result
	uint16_t data = TSI0->DATA & TSI_DATA_TSICNT_MASK;
	
	//Clear measurement complete flag
	TSI0->GENCS &= ~TSI_GENCS_EOSF_MASK;
	
	//Return calibrated data
	//Or 0 if we are less than the calibration for some reason
	 return (data > calibrationValue?  (data - calibrationValue) : 0);
}


