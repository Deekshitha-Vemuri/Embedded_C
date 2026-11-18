
/*
 * Task 31 - Common Sensor Management
 *
 * Build command (after all modules are created):
 * gcc -Wall -Wextra -std=c11 main.c sensor.c mock_temp.c mock_sensor2.c -o task31
 *
 * Functions used:
 * sensor_select()
 */

#include <stdio.h>
#include "mock_temp.h"
#include "mock_sensor2.h"
#include <stddef.h>
#include "sensor.h"



int sensor_select(int sensor_type, SensorDriver *selected_driver)
{
    if (selected_driver == NULL)
    {
        printf("[ERROR] Invalid driver pointer\n");
        return SENSOR_INVALID_CONFIG;
    }

    switch (sensor_type)
    {
        case SENSOR_TYPE_MOCK_TEMP:

            selected_driver->init = mock_temp_init;
            selected_driver->read = mock_temp_read;
            selected_driver->deinit = mock_temp_deinit;

            printf("[INFO] Selected sensor: mock_temp\n");
            break;

        case SENSOR_TYPE_MOCK_SENSOR2:

            selected_driver->init = mock_sensor2_init;
            selected_driver->read = mock_sensor2_read;
            selected_driver->deinit = mock_sensor2_deinit;

            printf("[INFO] Selected sensor: mock_sensor2\n");
            break;

        default:

            printf("[ERROR] Unsupported sensor type: %d\n", sensor_type);
            return SENSOR_INVALID_CONFIG;
    }

    return SENSOR_OK;
}

