//Basic filtering
#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>

#define NO_SAMPLES 32

typedef int16_t filterData_t;

typedef struct
{
	filterData_t buf[NO_SAMPLES];
	filterData_t curSum;
	filterData_t curVal;
} filterHandle_t;


filterData_t movingAverageAddSample(filterHandle_t *handle, filterData_t valToAdd);

#endif
