#ifndef _LIGHT_SENSOR_H_
#define _LIGHT_SENSOR_H_

#include <stdint.h>
#include <stdbool.h>

// based on provided example header file for sampler.


void lightSensor_init(void);
void lightSensor_cleanup(void);

// values seem to go between 0 to maybe 1650 as well.
// It kinda hovers around 300~500 due to light polution though
// until I shine a flashlight on it or use the LED right across from it
uint16_t lightSensor_readVal(void);

// Must be called once every 1s.
// Moves the samples that it has been collecting this second into
// the history, which makes the samples available for reads (below).
void lightSensor_moveCurrentDataToHistory(void);

// Get the number of samples collected during the previous complete second.
int lightSensor_getHistorySize(void);

// Get a copy of the samples in the sample history.
// Returns a newly allocated array and sets `size` to be the
// number of elements in the returned array (output-only parameter).
// The calling code must call free() on the returned pointer.
// Note: It provides both data and size to ensure consistency.
double* lightSensor_getHistory(int *size);

// Get the average light level (not tied to the history).
double lightSensor_getAverageReading(void);

// Get the total number of light level samples taken so far.
long long lightSensor_getNumSamplesTaken(void);


#endif

