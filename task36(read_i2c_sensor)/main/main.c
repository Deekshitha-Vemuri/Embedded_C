
#include <stdio.h>
#include "bme680.h"
#include "esp_log.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "BME680 firmware starting...");

    esp_err_t result = bme680_init();

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed");
        return;
    }

    result = bme680_detect();

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "BME680 detection failed");
        return;
    }

    ESP_LOGI(TAG, "BME680 sensor is ready!");
}
