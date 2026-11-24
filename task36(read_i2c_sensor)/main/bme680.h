#ifndef BME680_H
#define BME680_H
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
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
typedef struct
{
    uint32_t pressure;
    uint32_t temperature;
    uint16_t humidity;

} bme680_raw_data_t;
esp_err_t bme680_init(void);
esp_err_t bme680_detect(void);
esp_err_t bme680_read_chip_id(uint8_t *chip_id);
esp_err_t bme680_read_registers(
    uint8_t start_register,
    uint8_t *buffer,
    size_t length
);
esp_err_t bme680_read_calibration_data(void);
esp_err_t bme680_write_register(uint8_t register_address, uint8_t value);
esp_err_t bme680_configure_sensor(void);
esp_err_t bme680_wait_for_measurement(void);
esp_err_t bme680_read_raw_data(bme680_raw_data_t *raw_data);
esp_err_t bme680_get_calibration_data(
    bme680_calib_data_t *calib
);
#endif