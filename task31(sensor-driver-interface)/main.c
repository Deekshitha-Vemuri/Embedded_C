
/*
 * Task 31 - Sensor Driver Application
 *
 * Build command (MSVC Developer Command Prompt):
 * cl /nologo /W4 /TC /std:c11 main.c sensor.c mock_temp.c mock_sensor2.c /Fe:task31.exe
 *
 * Functions used:
 * scanf(), printf(), sensor_select()
 * SensorDriver.init(), SensorDriver.read(), SensorDriver.deinit()
 */
#include <stdio.h>
#include "sensor.h"

int main(void)
{
    int sensor_type;
    SensorDriver driver;
    SensorData reading;

    // Ask the user to select a sensor
    printf("Select Sensor:\n");
    printf("1. Mock Temperature Sensor\n");
    printf("2. Mock Sensor 2\n");
    printf("Enter your choice: ");

    if (scanf("%d", &sensor_type) != 1)
    {
        printf("[ERROR] Invalid input. Please enter a number.\n");
        return 1;
    }

    // Select the sensor driver
    if (sensor_select(sensor_type, &driver) != SENSOR_OK)
    {
        printf("[ERROR] Sensor selection failed\n");
        return 1;
    }

    // Initialize the selected sensor
    if (driver.init() != SENSOR_OK)
    {
        printf("[ERROR] Sensor initialization failed\n");
        return 1;
    }

    // Read sensor data
    if (driver.read(&reading) != SENSOR_OK)
    {
        printf("[ERROR] Sensor reading failed\n");
        driver.deinit();
        return 1;
    }

    // Display sensor readings
printf("\n[DATA] Temperature: %.1f C\n", reading.temperature);

if (reading.humidity_available)
{
    printf("[DATA] Humidity: %.1f %%\n", reading.humidity);
}

    // Deinitialize the sensor
    driver.deinit();

    return 0;
}
