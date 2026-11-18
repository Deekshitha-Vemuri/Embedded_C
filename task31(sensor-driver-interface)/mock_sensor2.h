
#ifndef MOCK_SENSOR2_H
#define MOCK_SENSOR2_H

#include "sensor.h"

// Mock sensor 2 functions
int mock_sensor2_init(void);
int mock_sensor2_read(SensorData *data);
void mock_sensor2_deinit(void);

#endif
