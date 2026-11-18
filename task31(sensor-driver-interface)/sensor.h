
#ifndef SENSOR_H
#define SENSOR_H


// Sensor operation status codes
#define SENSOR_OK                0
#define SENSOR_ERROR            -1
#define SENSOR_INIT_FAILED      -2
#define SENSOR_READ_FAILED      -3
#define SENSOR_INVALID_CONFIG   -4

// Common sensor reading format
typedef struct
{
    float temperature;
    float humidity;
    int humidity_available;
} SensorData;

// Common sensor driver interface
typedef struct
{
    int (*init)(void);
    int (*read)(SensorData *data);
    void (*deinit)(void);

} SensorDriver;

// Supported sensor types
#define SENSOR_TYPE_MOCK_TEMP   1
#define SENSOR_TYPE_MOCK_SENSOR2 2

// Select a sensor driver based on configuration
int sensor_select(int sensor_type, SensorDriver *selected_driver);

#endif
