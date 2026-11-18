
/*
 * Task 31 - Mock Sensor 2
 *
 * Build command (MSVC Developer Command Prompt):
 * cl /nologo /W4 /TC /std:c11 main.c sensor.c mock_temp.c mock_sensor2.c /Fe:task31.exe
 *
 * Functions used:
 * mock_sensor2_init()
 * mock_sensor2_read()
 * mock_sensor2_deinit()
 */

#include <stdio.h>
#include <stddef.h>
#include "mock_sensor2.h"

int mock_sensor2_init(void)
{
    printf("[INFO] Mock sensor 2 initialized\n");

    return SENSOR_OK;
}

int mock_sensor2_read(SensorData *data)
{
    if (data == NULL)
    {
        printf("[ERROR] Invalid sensor data pointer\n");
        return SENSOR_READ_FAILED;
    }

   data->temperature = 25.0f;
data->humidity = 50.0f;
data->humidity_available = 1;

    return SENSOR_OK;
}

void mock_sensor2_deinit(void)
{
    printf("[INFO] Mock sensor 2 deinitialized\n");
}
