//Copyright (c) 2016 Steven Yan and Joshua Lewis Tyler
//Licensed under the MIT license
//See LICENSE.txt

#include <stdint.h>
#include "filter.h"

filterData_t movingAverageAddSample(filterHandle_t *handle, filterData_t valToAdd)
{
	static int i=0;
	
	//Subtract oldest value from sum
	handle->curSum -= handle->buf[i];
	
	//Replace oldest value with new value
	handle->buf[i] = valToAdd;
	
	//Add new value to sum
	handle->curSum += handle->buf[i];
	
	//Maintain index
	i++;
	i%=NO_SAMPLES;
	
	//Update current value
	handle->curVal = handle->curSum / NO_SAMPLES;
	
	return handle->curVal;
}
