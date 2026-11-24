#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "bme680.h"
#include "esp_log.h"
#include <stddef.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define I2C_PORT       I2C_NUM_0
#define I2C_SDA_GPIO   GPIO_NUM_21
#define I2C_SCL_GPIO   GPIO_NUM_22
#define I2C_FREQUENCY  100000
static const char *TAG = "BME680";
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t bme680_handle;
static bme680_calib_data_t calib_data;
esp_err_t bme680_init(void)  
/*
 * esp_err_t is the ESP-IDF error/status type.
 * ESP_OK indicates success.
 * Other error codes indicate different failure conditions.
 */
{
   i2c_master_bus_config_t i2c_config = {
       .i2c_port = I2C_PORT,
       .sda_io_num = I2C_SDA_GPIO,
       .scl_io_num = I2C_SCL_GPIO,
       .clk_source = I2C_CLK_SRC_DEFAULT,
       .glitch_ignore_cnt = 7,
       .flags.enable_internal_pullup = true
   };
   esp_err_t result = i2c_new_master_bus(&i2c_config, &bus_handle);
   if (result != ESP_OK) {
       ESP_LOGE(TAG, "I2C bus initialization failed");
       return result;
   }
   ESP_LOGI(TAG, "I2C bus initialized successfully");
   i2c_device_config_t dev_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = 0x77,
    .scl_speed_hz = I2C_FREQUENCY,
};
result = i2c_master_bus_add_device(
    bus_handle,
    &dev_config,
    &bme680_handle
);

if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to add BME680 to I2C bus");
    return result;
}

ESP_LOGI(TAG, "BME680 device added to I2C bus successfully");
   return ESP_OK;
}
esp_err_t bme680_detect(void)
{
   esp_err_t result = i2c_master_probe(bus_handle, 0x77, 1000);
   if (result == ESP_OK) {
       ESP_LOGI(TAG, "BME680 detected at address 0x77");
   }
   else {
       ESP_LOGE(TAG, "BME680 not detected");
   }
   return result;
}


esp_err_t bme680_read_chip_id(uint8_t *chip_id)
{
    // Check whether the caller provided a valid pointer
    if (chip_id == NULL) {
        ESP_LOGE(TAG, "Invalid chip ID pointer");
        return ESP_ERR_INVALID_ARG;
    }

    // BME680 chip ID register address
    uint8_t register_address = 0xD0;

    // Read one byte from the chip ID register
    esp_err_t result = i2c_master_transmit_receive(
        bme680_handle,
        &register_address,
        1,
        chip_id,
        1,
        1000
    );

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read BME680 chip ID");
        return result;
    }

    return ESP_OK;
}
esp_err_t bme680_read_registers(
    uint8_t start_register,
    uint8_t *buffer,
    size_t length
)
{
    if (buffer == NULL || length == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t result = i2c_master_transmit_receive(
        bme680_handle,
        &start_register,
        1,
        buffer,
        length,
        1000
    );

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read registers starting at 0x%02X",
                 start_register);
        return result;
    }

    return ESP_OK;
}
static void bme680_decode_calibration(
    const uint8_t coeff_arr1[25],
    const uint8_t coeff_arr2[16]
)
{
    // Temperature calibration
    calib_data.par_t1 =
        (uint16_t)(((uint16_t)coeff_arr2[9] << 8) |
                    coeff_arr2[8]);

    calib_data.par_t2 =
        (int16_t)(((uint16_t)coeff_arr1[2] << 8) |
                   coeff_arr1[1]);

    calib_data.par_t3 = (int8_t)coeff_arr1[3];


    // Pressure calibration
    calib_data.par_p1 =
        (uint16_t)(((uint16_t)coeff_arr1[6] << 8) |
                    coeff_arr1[5]);

    calib_data.par_p2 =
        (int16_t)(((uint16_t)coeff_arr1[8] << 8) |
                   coeff_arr1[7]);

    calib_data.par_p3 = (int8_t)coeff_arr1[9];

    calib_data.par_p4 =
        (int16_t)(((uint16_t)coeff_arr1[12] << 8) |
                   coeff_arr1[11]);

    calib_data.par_p5 =
        (int16_t)(((uint16_t)coeff_arr1[14] << 8) |
                   coeff_arr1[13]);

    calib_data.par_p6 = (int8_t)coeff_arr1[16];
    calib_data.par_p7 = (int8_t)coeff_arr1[15];

    calib_data.par_p8 =
        (int16_t)(((uint16_t)coeff_arr1[20] << 8) |
                   coeff_arr1[19]);

    calib_data.par_p9 =
        (int16_t)(((uint16_t)coeff_arr1[22] << 8) |
                   coeff_arr1[21]);

    calib_data.par_p10 = coeff_arr1[23];


    // Humidity calibration
    calib_data.par_h2 =
        (uint16_t)(((uint16_t)coeff_arr2[0] << 4) |
                    (coeff_arr2[1] >> 4));

    calib_data.par_h1 =
        (uint16_t)(((uint16_t)coeff_arr2[2] << 4) |
                    (coeff_arr2[1] & 0x0F));

    calib_data.par_h3 = (int8_t)coeff_arr2[3];
    calib_data.par_h4 = (int8_t)coeff_arr2[4];
    calib_data.par_h5 = (int8_t)coeff_arr2[5];
    calib_data.par_h6 = coeff_arr2[6];
    calib_data.par_h7 = (int8_t)coeff_arr2[7];
}
esp_err_t bme680_read_calibration_data(void)
{
    // Temporary arrays to store calibration register bytes
    uint8_t coeff_arr1[25];
    uint8_t coeff_arr2[16];

    // Read the first calibration block: 0x89 to 0xA1
    esp_err_t result = bme680_read_registers(
        0x89,
        coeff_arr1,
        sizeof(coeff_arr1)
    );

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read first calibration block");
        return result;
    }

    // Read the second calibration block: 0xE1 to 0xF0
    result = bme680_read_registers(
        0xE1,
        coeff_arr2,
        sizeof(coeff_arr2)
    );

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read second calibration block");
        return result;
    }

    // Convert raw register bytes into calibration coefficients
    bme680_decode_calibration(coeff_arr1, coeff_arr2);

    ESP_LOGI(TAG, "Calibration data loaded successfully");

    return ESP_OK;
}
esp_err_t bme680_get_calibration_data(
    bme680_calib_data_t *calib
)
{
    if (calib == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *calib = calib_data;

    return ESP_OK;
}
//Sends a register address and a value to the BME680 through I2C.
esp_err_t bme680_write_register(
    uint8_t register_address,
    uint8_t value
)
{
    uint8_t write_buffer[2];

    write_buffer[0] = register_address;
    write_buffer[1] = value;

    esp_err_t result = i2c_master_transmit(
        bme680_handle,
        write_buffer,
        sizeof(write_buffer),
        1000
    );

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write register 0x%02X",
                 register_address);
        return result;
    }

    return ESP_OK;
}
esp_err_t bme680_configure_sensor(void)
{
    // Configure humidity oversampling: x1
    esp_err_t result = bme680_write_register(0x72, 0x01);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Humidity configuration failed");
        return result;
    }

    // Configure temperature x1, pressure x1, forced mode
    result = bme680_write_register(0x74, 0x25);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Measurement configuration failed");
        return result;
    }

    ESP_LOGI(TAG, "BME680 sensor configured successfully");

    return ESP_OK;
}
esp_err_t bme680_wait_for_measurement(void)
{
    uint8_t status = 0;

    for (int i = 0; i < 100; i++)
    {
        esp_err_t result = bme680_read_registers(
            0x1D,
            &status,
            1
        );

        if (result != ESP_OK) {
            return result;
        }

        // Check whether measurement is complete
        bool measuring = (status & (1U << 5)) != 0;

        // Check whether new data is available
        bool new_data = (status & (1U << 7)) != 0;

        if (!measuring && new_data) {
            ESP_LOGI(TAG, "Measurement completed");
            return ESP_OK;
        }

        // Wait one FreeRTOS tick before checking again
        vTaskDelay(1);
    }

    ESP_LOGE(TAG, "Measurement timeout");

    return ESP_ERR_TIMEOUT;
}
esp_err_t bme680_read_raw_data(bme680_raw_data_t *raw_data)
{
    // Validate the structure pointer
    if (raw_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Buffer to store eight consecutive register bytes
    uint8_t data[8];

    // Read registers 0x1F through 0x26
    esp_err_t result = bme680_read_registers(
        0x1F,
        data,
        sizeof(data)
    );

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read raw measurement data");
        return result;
    }

    // Extract 20-bit pressure value
    raw_data->pressure =
        ((uint32_t)data[0] << 12) |
        ((uint32_t)data[1] << 4)  |
        ((uint32_t)data[2] >> 4);

    // Extract 20-bit temperature value
    raw_data->temperature =
        ((uint32_t)data[3] << 12) |
        ((uint32_t)data[4] << 4)  |
        ((uint32_t)data[5] >> 4);

    // Extract 16-bit humidity value
    raw_data->humidity =
        ((uint16_t)data[6] << 8) |
        data[7];

    return ESP_OK;
}

