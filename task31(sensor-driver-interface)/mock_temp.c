
/*
 * Task 31 - Mock Temperature Sensor
 *
 * Build command:
 cl /nologo /W4 /TC /std:c11 main.c sensor.c mock_temp.c mock_sensor2.c /Fe:task31.exe
 *
 * Functions used:
 * mock_temp_init()
 * mock_temp_read()
 * mock_temp_deinit()
 */

#include <stdio.h>
#include "mock_temp.h"

int mock_temp_init(void)
{
    printf("[INFO] Mock temperature sensor initialized\n");

    return SENSOR_OK;
}

int mock_temp_read(SensorData *data)
{
    if (data == NULL)
    {
        printf("[ERROR] Invalid sensor data pointer\n");
        return SENSOR_READ_FAILED;
    }

    data->temperature = 24.8f;
data->humidity_available = 0;

    return SENSOR_OK;
}

void mock_temp_deinit(void)
{
    printf("[INFO] Mock temperature sensor deinitialized\n");
}
