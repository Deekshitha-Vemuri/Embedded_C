#ifndef BME680_COMPENSATION_H
#define BME680_COMPENSATION_H

#include "bme680.h"

esp_err_t bme680_compensate_temperature(
    uint32_t temp_adc,
    const bme680_calib_data_t *calib,
    int16_t *temperature_centi_c,
    int32_t *t_fine
);
esp_err_t bme680_compensate_pressure(
    uint32_t press_adc,
    const bme680_calib_data_t *calib,
    int32_t t_fine,
    uint32_t *pressure_pa
);
esp_err_t bme680_compensate_humidity(
    uint16_t hum_adc,
    const bme680_calib_data_t *calib,
    int16_t temperature_centi_c,
    uint32_t *humidity
);

#endif