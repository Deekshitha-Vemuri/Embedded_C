#ifndef BME680_H
#define BME680_H

#include <stdint.h>
#include "esp_err.h"

// Stores the factory calibration coefficients read from BME680.
// These coefficients are required to convert raw ADC values
// into actual temperature, pressure, and humidity measurements.
typedef struct
{
    // Temperature calibration
    uint16_t par_t1;	
    int16_t  par_t2;
    int8_t   par_t3;

    // Pressure calibration
    uint16_t par_p1;
    int16_t  par_p2;
    int8_t   par_p3;
    int16_t  par_p4;
    int16_t  par_p5;
    int8_t   par_p6;
    int8_t   par_p7;
    int16_t  par_p8;
    int16_t  par_p9;
    uint8_t  par_p10;

    // Humidity calibration
    uint16_t par_h1;
    uint16_t par_h2;
    int8_t   par_h3;
    int8_t   par_h4;
    int8_t   par_h5;
    uint8_t  par_h6;
    int8_t   par_h7;

} bme680_calib_data_t;


// Initialize I2C and register BME680
esp_err_t bme680_init(void);

// Verify BME680 chip ID
esp_err_t bme680_detect(void);

// Read calibration coefficients
esp_err_t bme680_read_calibration_data(void);

// Configure sensor and start forced measurement
esp_err_t bme680_start_forced_measurement(void);

// Wait until the measurement is completed
esp_err_t bme680_wait_for_measurement(void);

// Read compensated temperature in degrees Celsius
esp_err_t bme680_read_temperature(float *temperature);

// Read compensated pressure in Pascals
esp_err_t bme680_read_pressure(float *pressure);

// Read compensated relative humidity in percentage
esp_err_t bme680_read_humidity(float *humidity);

#endif