
#ifndef MOCK_TEMP_H
#define MOCK_TEMP_H

#include "sensor.h"

// Mock temperature sensor functions
int mock_temp_init(void);
int mock_temp_read(SensorData *data);
void mock_temp_deinit(void);

#endif
