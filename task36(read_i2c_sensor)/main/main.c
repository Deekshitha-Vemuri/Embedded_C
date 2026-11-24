
#include <stdio.h>
#include <string.h>

#include "bme680.h"
#include "bme680_compensation.h"
#include "uart_handler.h"
#include "esp_log.h"

static const char *TAG = "MAIN";


static esp_err_t read_sensor_and_respond(void)
{
    esp_err_t result;

    // Trigger a new forced-mode measurement
    result = bme680_configure_sensor();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Sensor configuration failed");
        return result;
    }

    // Wait for measurement completion
    result = bme680_wait_for_measurement();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Measurement failed or timed out");
        return result;
    }

    // Read raw sensor data
    bme680_raw_data_t raw_data = {0};

    result = bme680_read_raw_data(&raw_data);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read raw sensor data");
        return result;
    }

    // Retrieve calibration data
    bme680_calib_data_t calibration = {0};

    result = bme680_get_calibration_data(&calibration);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get calibration data");
        return result;
    }

    // Temperature compensation
    int16_t temperature_centi_c = 0;
    int32_t t_fine = 0;

    result = bme680_compensate_temperature(
        raw_data.temperature,
        &calibration,
        &temperature_centi_c,
        &t_fine
    );

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Temperature compensation failed");
        return result;
    }

    // Pressure compensation
    uint32_t pressure_pa = 0;

    result = bme680_compensate_pressure(
        raw_data.pressure,
        &calibration,
        t_fine,
        &pressure_pa
    );

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Pressure compensation failed");
        return result;
    }

    // Humidity compensation
    uint32_t humidity_milli_percent = 0;

    result = bme680_compensate_humidity(
        raw_data.humidity,
        &calibration,
        temperature_centi_c,
        &humidity_milli_percent
    );

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Humidity compensation failed");
        return result;
    }

    // Format the sensor response
    char response[128];

    snprintf(
        response,
        sizeof(response),
        "DATA,T=%.2f,RH=%.3f,P=%.2f",
        temperature_centi_c / 100.0f,
        humidity_milli_percent / 1000.0f,
        pressure_pa / 100.0f
    );

    // Send readings to Windows PC
    result = uart_handler_send_response(response);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to send sensor response");
        return result;
    }

    ESP_LOGI(TAG, "Sensor readings sent successfully");

    return ESP_OK;
}


void app_main(void)
{
    ESP_LOGI(TAG, "BME680 firmware starting...");

    // Step 1: Initialize I2C
    esp_err_t result = bme680_init();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C initialization failed");
        return;
    }

    // Step 2: Detect BME680
    result = bme680_detect();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "BME680 detection failed");
        return;
    }

    // Step 3: Verify chip ID
    uint8_t chip_id = 0;

    result = bme680_read_chip_id(&chip_id);

    if (result != ESP_OK || chip_id != 0x61)
    {
        ESP_LOGE(TAG, "BME680 chip ID verification failed");
        return;
    }

    ESP_LOGI(TAG, "BME680 detected successfully");

    // Step 4: Read calibration data once
    result = bme680_read_calibration_data();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read calibration data");
        return;
    }

    ESP_LOGI(TAG, "Calibration data loaded successfully");

    // Step 5: Initialize UART
    result = uart_handler_init();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "UART initialization failed: %s",
                 esp_err_to_name(result));
        return;
    }

    ESP_LOGI(TAG, "UART command handler initialized");
    ESP_LOGI(TAG, "Waiting for READ_SENSOR command...");

    // Step 6: Wait for commands from Windows
    char command[32];

    while (1)
    {
        result = uart_handler_receive_command(
            command,
            sizeof(command)
        );

        if (result == ESP_ERR_TIMEOUT)
        {
            continue;
        }

        if (result != ESP_OK)
        {
            ESP_LOGE(TAG, "Command reception failed");
            continue;
        }

        ESP_LOGI(TAG, "Received command: %s", command);

        if (strcmp(command, "READ_SENSOR") == 0)
        {
            result = read_sensor_and_respond();

            if (result != ESP_OK)
            {
                uart_handler_send_response("ERROR,SENSOR_READ_FAILED");
            }
        }
        else
        {
            uart_handler_send_response("ERROR,UNKNOWN_COMMAND");
        }
    }
}
