#ifndef BME680_H
#define BME680_H
#include <stdint.h>
#include "esp_err.h"

esp_err_t bme680_init(void);
esp_err_t bme680_detect(void);
esp_err_t bme680_read_chip_id(uint8_t *chip_id);
#endif