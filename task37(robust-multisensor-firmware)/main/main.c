#include "bme680.h"
#include "esp_log.h"

// Logging tag used to identify Task 37 messages
static const char *TAG = "TASK37";

void app_main(void)
{
    ESP_LOGI(TAG, "Task 37 started");

    // Initialize I2C and register the BME680 device
    esp_err_t result = bme680_init();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "BME680 initialization failed");
        return;
    }

    // Verify that the connected sensor is a BME680
    result = bme680_detect();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "BME680 detection failed");
        return;
    }

    ESP_LOGI(TAG, "BME680 initialization and detection completed");

    // Read the factory calibration coefficients stored in the sensor
    result = bme680_read_calibration_data();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Calibration reading failed");
        return;
    }

    ESP_LOGI(TAG, "Calibration data read successful");

    // Configure oversampling and start a forced measurement
    result = bme680_start_forced_measurement();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start sensor measurement");
        return;
    }

    // Wait until the sensor completes its measurement
    result = bme680_wait_for_measurement();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Measurement did not complete");
        return;
    }

    // Read and compensate temperature
    float temperature = 0.0f;

    result = bme680_read_temperature(&temperature);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Temperature reading failed");
        return;
    }

    ESP_LOGI(TAG, "Temperature: %.2f C", temperature);

    // Read and compensate pressure
    float pressure = 0.0f;

    result = bme680_read_pressure(&pressure);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Pressure reading failed");
        return;
    }

    ESP_LOGI(TAG, "Pressure: %.2f Pa", pressure);

    // Read and compensate relative humidity
    float humidity = 0.0f;

    result = bme680_read_humidity(&humidity);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Humidity reading failed");
        return;
    }

    ESP_LOGI(TAG, "Humidity: %.2f %%", humidity);
}