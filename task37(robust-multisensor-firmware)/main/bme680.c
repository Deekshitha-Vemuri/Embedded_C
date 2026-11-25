#include "bme680.h"

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// --------------------------------------------------
// Hardware and register configuration
// --------------------------------------------------

// I2C pins connected to the BME680
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_SCL_IO GPIO_NUM_22

// BME680 I2C address and chip identification
#define BME680_I2C_ADDRESS 0x77
#define BME680_CHIP_ID_REG 0xD0
#define BME680_CHIP_ID 0x61

// Sensor status and measurement data registers
#define BME680_STATUS_REG 0x1D
#define BME680_PRESS_MSB_REG 0x1F
#define BME680_TEMP_MSB_REG 0x22
#define BME680_HUM_MSB_REG 0x25

// Sensor control registers
#define BME680_CTRL_HUM_REG 0x72
#define BME680_CTRL_MEAS_REG 0x74

// Status register bit masks
#define BME680_NEW_DATA_MASK (1 << 7)
#define BME680_MEASURING_MASK (1 << 5)


// --------------------------------------------------
// Global sensor data and I2C handles
// --------------------------------------------------

// Stores factory calibration coefficients
static bme680_calib_data_t calib_data;

// Intermediate temperature value used by pressure compensation
static float t_fine;

// Final compensated temperature used by humidity compensation
static float compensated_temperature;

// Logging tag for BME680 messages
static const char *TAG = "BME680";

// I2C handles used to communicate with the sensor
static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t device_handle = NULL;


// --------------------------------------------------
// Initialize I2C master bus
// --------------------------------------------------

static esp_err_t i2c_master_init(void)
{
    // Configure the ESP32 I2C master bus
    i2c_master_bus_config_t i2c_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,

        // External pull-up resistors are already present
        // on the BME680 breakout board.
        .flags.enable_internal_pullup = false
    };

    // Create and initialize the I2C master bus
    esp_err_t result = i2c_new_master_bus(
        &i2c_config,
        &bus_handle
    );

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C initialization failed: %s",
                 esp_err_to_name(result));

        return result;
    }

    ESP_LOGI(TAG, "I2C master bus initialized successfully");

    return ESP_OK;
}


// --------------------------------------------------
// Add BME680 to I2C bus
// --------------------------------------------------

static esp_err_t bme680_add_device(void)
{
    // Configure BME680 as an I2C slave device
    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BME680_I2C_ADDRESS,
        .scl_speed_hz = 100000
    };

    // Register the sensor with the I2C master bus
    esp_err_t result = i2c_master_bus_add_device(
        bus_handle,
        &device_config,
        &device_handle
    );

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add BME680 device: %s",
                 esp_err_to_name(result));

        return result;
    }

    ESP_LOGI(TAG, "BME680 device added to I2C bus");

    return ESP_OK;
}


// --------------------------------------------------
// Read one register
// --------------------------------------------------

static esp_err_t bme680_read_register(
    uint8_t register_address,
    uint8_t *register_value
)
{
    // Check whether the output pointer is valid
    if (register_value == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Send register address and receive one byte
    return i2c_master_transmit_receive(
        device_handle,
        &register_address,
        1,
        register_value,
        1,
        1000
    );
}


// --------------------------------------------------
// Write one register
// --------------------------------------------------

static esp_err_t bme680_write_register(
    uint8_t register_address,
    uint8_t value
)
{
    // First byte is the register address.
    // Second byte is the value to write.
    uint8_t data[2] = {
        register_address,
        value
    };

    return i2c_master_transmit(
        device_handle,
        data,
        sizeof(data),
        1000
    );
}


// --------------------------------------------------
// Read multiple consecutive registers
// --------------------------------------------------

static esp_err_t bme680_read_registers(
    uint8_t start_register,
    uint8_t *buffer,
    size_t length
)
{
    // Validate the destination buffer and requested length
    if (buffer == NULL || length == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Send the starting register address and read
    // the requested number of consecutive bytes.
    return i2c_master_transmit_receive(
        device_handle,
        &start_register,
        1,
        buffer,
        length,
        1000
    );
}


// --------------------------------------------------
// Start forced measurement
// --------------------------------------------------

esp_err_t bme680_start_forced_measurement(void)
{
    esp_err_t result;

    // Configure humidity oversampling to x1
    // CTRL_HUM register: 0x72
    result = bme680_write_register(
        BME680_CTRL_HUM_REG,
        0x01
    );

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure humidity");
        return result;
    }

    // Configure temperature oversampling x1,
    // pressure oversampling x1, and forced mode.
    //
    // 0x25 = 00100101 in binary:
    // Temperature oversampling = x1
    // Pressure oversampling    = x1
    // Measurement mode         = forced mode
    result = bme680_write_register(
        BME680_CTRL_MEAS_REG,
        0x25
    );

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start forced measurement");
        return result;
    }

    ESP_LOGI(TAG, "Forced measurement started successfully");

    return ESP_OK;
}


// --------------------------------------------------
// Wait for measurement completion
// --------------------------------------------------

esp_err_t bme680_wait_for_measurement(void)
{
    uint8_t status = 0;

    // Poll sensor status every 10 milliseconds.
    // Stop waiting after a maximum of 1000 milliseconds.
    for (int elapsed = 0; elapsed < 1000; elapsed += 10)
    {
        esp_err_t result = bme680_read_register(
            BME680_STATUS_REG,
            &status
        );

        if (result != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to read sensor status");
            return result;
        }

        // Bit 5 indicates whether the sensor is measuring
        bool measuring =
            (status & BME680_MEASURING_MASK) != 0;

        // Bit 7 indicates whether new measurement data is available
        bool new_data =
            (status & BME680_NEW_DATA_MASK) != 0;

        // Measurement is complete when the sensor is not measuring
        // and the new-data flag is set.
        if (!measuring && new_data)
        {
            ESP_LOGI(TAG, "Measurement completed successfully");
            return ESP_OK;
        }

        // Delay for 10 milliseconds before checking again
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGE(TAG, "Measurement timeout");

    return ESP_ERR_TIMEOUT;
}


// --------------------------------------------------
// Detect BME680 using chip ID
// --------------------------------------------------

esp_err_t bme680_detect(void)
{
    uint8_t chip_id = 0;

    // Read the chip ID register (0xD0)
    esp_err_t result = bme680_read_register(
        BME680_CHIP_ID_REG,
        &chip_id
    );

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read chip ID: %s",
                 esp_err_to_name(result));

        return result;
    }

    ESP_LOGI(TAG, "BME680 Chip ID: 0x%02X", chip_id);

    // Expected BME680 chip ID is 0x61
    if (chip_id != BME680_CHIP_ID)
    {
        ESP_LOGE(TAG, "Unexpected chip ID!");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "BME680 detected successfully!");

    return ESP_OK;
}


// --------------------------------------------------
// Public initialization function
// --------------------------------------------------

esp_err_t bme680_init(void)
{
    // First initialize the I2C master bus
    esp_err_t result = i2c_master_init();

    if (result != ESP_OK)
    {
        return result;
    }

    // Then register the BME680 device on the I2C bus
    result = bme680_add_device();

    if (result != ESP_OK)
    {
        return result;
    }

    return ESP_OK;
}


// --------------------------------------------------
// Combine two bytes
// --------------------------------------------------

static uint16_t combine_bytes(uint8_t lsb, uint8_t msb)
{
    // Shift MSB eight positions left and combine it with LSB
    return ((uint16_t)msb << 8) | lsb;
}


// --------------------------------------------------
// Read calibration data
// --------------------------------------------------

esp_err_t bme680_read_calibration_data(void)
{
    uint8_t calib1[25];
    uint8_t calib2[10];

    esp_err_t result;

    // Read calibration block 1: registers 0x89 to 0xA1
    result = bme680_read_registers(0x89, calib1, 25);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read calibration block 1");
        return result;
    }

    // Read calibration block 2: registers 0xE1 to 0xEA
    result = bme680_read_registers(0xE1, calib2, 10);

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read calibration block 2");
        return result;
    }

    // Temperature calibration coefficients
    calib_data.par_t1 = combine_bytes(calib2[8], calib2[9]);
    calib_data.par_t2 = (int16_t)combine_bytes(calib1[1], calib1[2]);
    calib_data.par_t3 = (int8_t)calib1[3];

    // Pressure calibration coefficients
    calib_data.par_p1 = combine_bytes(calib1[5], calib1[6]);
    calib_data.par_p2 = (int16_t)combine_bytes(calib1[7], calib1[8]);
    calib_data.par_p3 = (int8_t)calib1[9];
    calib_data.par_p4 = (int16_t)combine_bytes(calib1[11], calib1[12]);
    calib_data.par_p5 = (int16_t)combine_bytes(calib1[13], calib1[14]);
    calib_data.par_p6 = (int8_t)calib1[16];
    calib_data.par_p7 = (int8_t)calib1[15];
    calib_data.par_p8 = (int16_t)combine_bytes(calib1[19], calib1[20]);
    calib_data.par_p9 = (int16_t)combine_bytes(calib1[21], calib1[22]);
    calib_data.par_p10 = calib1[23];

    // Humidity calibration coefficients
    //
    // H1 uses the byte at E2 and the lower nibble at E3.
    // H2 uses the byte at E1 and the upper nibble at E3.
    //
    // calib2[0] = E1
    // calib2[1] = E2
    // calib2[2] = E3

// Humidity calibration coefficients
//
// calib2[0] = Register E1
// calib2[1] = Register E2
// calib2[2] = Register E3
//
// H1: E3 is MSB and E2 lower nibble is LSB.
// H2: E1 is MSB and E2 upper nibble is LSB.

calib_data.par_h1 =
    ((uint16_t)calib2[2] << 4) |
    (calib2[1] & 0x0F);

calib_data.par_h2 =
    ((uint16_t)calib2[0] << 4) |
    (calib2[1] >> 4);

    calib_data.par_h3 = (int8_t)calib2[3];
    calib_data.par_h4 = (int8_t)calib2[4];
    calib_data.par_h5 = (int8_t)calib2[5];
    calib_data.par_h6 = calib2[6];
    calib_data.par_h7 = (int8_t)calib2[7];

    ESP_LOGI(TAG, "BME680 calibration data read successfully");

    // Print selected coefficients for debugging
    ESP_LOGI(TAG, "T1=%u T2=%d T3=%d",
             calib_data.par_t1,
             calib_data.par_t2,
             calib_data.par_t3);

    ESP_LOGI(TAG, "P1=%u P2=%d P3=%d",
             calib_data.par_p1,
             calib_data.par_p2,
             calib_data.par_p3);

    ESP_LOGI(TAG, "H1=%u H2=%u H3=%d",
             calib_data.par_h1,
             calib_data.par_h2,
             calib_data.par_h3);

    return ESP_OK;
}


// --------------------------------------------------
// Read raw temperature ADC
// --------------------------------------------------

static esp_err_t bme680_read_raw_temperature(uint32_t *raw_temp)
{
    if (raw_temp == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Temperature ADC occupies three consecutive registers
    uint8_t data[3];

    esp_err_t result = bme680_read_registers(
        BME680_TEMP_MSB_REG,
        data,
        3
    );

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read raw temperature");
        return result;
    }

    // Combine the three bytes into a 20-bit ADC value
    // The lower four bits of the third byte are not part of the ADC.
    *raw_temp =
        ((uint32_t)data[0] << 12) |
        ((uint32_t)data[1] << 4) |
        ((uint32_t)data[2] >> 4);

    ESP_LOGI(TAG, "Raw temperature ADC: %" PRIu32, *raw_temp);

    return ESP_OK;
}


// --------------------------------------------------
// Compensate temperature
// --------------------------------------------------

static float bme680_compensate_temperature(uint32_t raw_temp)
{
    float var1;
    float var2;
    float temperature;

    // Calculate the first intermediate value using T1 and T2
    var1 = (((float)raw_temp / 16384.0f) -
            ((float)calib_data.par_t1 / 1024.0f))
            * (float)calib_data.par_t2;

    // Calculate the temperature difference using T1
    float temp_difference =
        ((float)raw_temp / 131072.0f) -
        ((float)calib_data.par_t1 / 8192.0f);

    // Calculate the second intermediate value using T3
    var2 = temp_difference * temp_difference *
           ((float)calib_data.par_t3 * 16.0f);

    // t_fine is required by pressure compensation
    t_fine = var1 + var2;

    // Convert t_fine into degrees Celsius
    temperature = t_fine / 5120.0f;

    // Preserve temperature for humidity compensation
    compensated_temperature = temperature;

    return temperature;
}


// --------------------------------------------------
// Public temperature reading function
// --------------------------------------------------

esp_err_t bme680_read_temperature(float *temperature)
{
    if (temperature == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t raw_temp = 0;

    // Read the raw temperature ADC value
    esp_err_t result = bme680_read_raw_temperature(&raw_temp);

    if (result != ESP_OK)
    {
        return result;
    }

    // Convert raw ADC into degrees Celsius
    *temperature = bme680_compensate_temperature(raw_temp);

    ESP_LOGI(TAG, "Temperature compensation completed");

    return ESP_OK;
}


// --------------------------------------------------
// Read raw pressure ADC
// --------------------------------------------------

static esp_err_t bme680_read_raw_pressure(uint32_t *raw_pressure)
{
    if (raw_pressure == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Pressure ADC occupies three consecutive registers
    uint8_t data[3];

    esp_err_t result = bme680_read_registers(
        BME680_PRESS_MSB_REG,
        data,
        3
    );

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read raw pressure");
        return result;
    }

    // Combine the three bytes into a 20-bit pressure ADC value
    *raw_pressure =
        ((uint32_t)data[0] << 12) |
        ((uint32_t)data[1] << 4) |
        ((uint32_t)data[2] >> 4);

    ESP_LOGI(TAG, "Raw pressure ADC: %" PRIu32, *raw_pressure);

    return ESP_OK;
}


// --------------------------------------------------
// Compensate pressure
// --------------------------------------------------

static float bme680_compensate_pressure(uint32_t raw_pressure)
{
    double var1;
    double var2;
    double var3;
    double pressure;

    // Calculate the first intermediate pressure value using t_fine
    var1 = ((double)t_fine / 2.0) - 64000.0;

    // Apply pressure calibration coefficients P6 and P5
    var2 = var1 * var1 *
           ((double)calib_data.par_p6 / 131072.0);

    var2 = var2 +
           (var1 * ((double)calib_data.par_p5 * 2.0));

    // Apply pressure calibration coefficient P4
    var2 = (var2 / 4.0) +
           ((double)calib_data.par_p4 * 65536.0);

    // Apply pressure calibration coefficients P3 and P2
    var1 = ((((double)calib_data.par_p3 * var1 * var1) / 16384.0) +
            ((double)calib_data.par_p2 * var1)) / 524288.0;

    // Apply pressure calibration coefficient P1
    var1 = (1.0 + (var1 / 32768.0)) *
           (double)calib_data.par_p1;

    // Prevent division by zero
    if (var1 == 0.0)
    {
        return -1.0f;
    }

    // Calculate pressure using the raw ADC and intermediate values
    pressure = 1048576.0 - (double)raw_pressure;

    pressure = ((pressure - (var2 / 4096.0)) * 6250.0) / var1;

    // Apply final pressure calibration corrections P9 and P8
    var1 = ((double)calib_data.par_p9 * pressure * pressure) /
           2147483648.0;

    var2 = pressure * ((double)calib_data.par_p8 / 32768.0);

    // Apply final pressure calibration correction P10
    var3 = (pressure / 256.0) *
           (pressure / 256.0) *
           (pressure / 256.0) *
           ((double)calib_data.par_p10 / 131072.0);

    // Apply P7 and combine the final correction values
    pressure = pressure +
               (var1 + var2 + var3 +
                ((double)calib_data.par_p7 * 128.0)) / 16.0;

    // Pressure result is in Pascals
    return (float)pressure;
}


// --------------------------------------------------
// Read raw humidity ADC
// --------------------------------------------------

static esp_err_t bme680_read_raw_humidity(uint16_t *raw_humidity)
{
    if (raw_humidity == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Humidity ADC occupies two consecutive registers
    uint8_t data[2];

    esp_err_t result = bme680_read_registers(
        BME680_HUM_MSB_REG,
        data,
        2
    );

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read raw humidity");
        return result;
    }

    // Combine the two bytes into a 16-bit ADC value
    *raw_humidity =
        ((uint16_t)data[0] << 8) |
        (uint16_t)data[1];

    ESP_LOGI(TAG, "Raw humidity ADC: %u",
             (unsigned int)*raw_humidity);

    return ESP_OK;
}


// --------------------------------------------------
// Compensate humidity
// --------------------------------------------------

static float bme680_compensate_humidity(uint16_t raw_humidity)
{
    double var1;
    double var2;
    double var3;
    double var4;
    double humidity;

    // Humidity compensation uses the compensated temperature in Celsius
    double temperature = (double)compensated_temperature;

    // Calculate the first intermediate humidity value using H1 and H3
    var1 = (double)raw_humidity -
           (((double)calib_data.par_h1 * 16.0) +
            (((double)calib_data.par_h3 / 2.0) * temperature));

    // Apply humidity calibration coefficients H2, H4 and H5
    var2 = var1 *
           (((double)calib_data.par_h2 / 262144.0) *
            (1.0 +
             (((double)calib_data.par_h4 / 16384.0) * temperature) +
             (((double)calib_data.par_h5 / 1048576.0) *
              temperature * temperature)));

    // Calculate correction values using H6 and H7
    var3 = (double)calib_data.par_h6 / 16384.0;

    var4 = (double)calib_data.par_h7 / 2097152.0;

    // Calculate final compensated relative humidity
    humidity = var2 +
               ((var3 + (var4 * temperature)) * var2 * var2);

    // Relative humidity cannot be greater than 100%
    if (humidity > 100.0)
    {
        humidity = 100.0;
    }

    // Relative humidity cannot be less than 0%
    if (humidity < 0.0)
    {
        humidity = 0.0;
    }

    return (float)humidity;
}


// --------------------------------------------------
// Public pressure reading function
// --------------------------------------------------

esp_err_t bme680_read_pressure(float *pressure)
{
    if (pressure == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t raw_pressure = 0;

    // Read the raw pressure ADC value
    esp_err_t result = bme680_read_raw_pressure(&raw_pressure);

    if (result != ESP_OK)
    {
        return result;
    }

    // Compensate raw pressure using calibration coefficients
    // and t_fine calculated during temperature compensation
    *pressure = bme680_compensate_pressure(raw_pressure);

    if (*pressure < 0.0f)
    {
        ESP_LOGE(TAG, "Pressure compensation failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Pressure compensation completed");

    return ESP_OK;
}


// --------------------------------------------------
// Public humidity reading function
// --------------------------------------------------

esp_err_t bme680_read_humidity(float *humidity)
{
    if (humidity == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t raw_humidity = 0;

    // Read the raw humidity ADC value
    esp_err_t result = bme680_read_raw_humidity(&raw_humidity);

    if (result != ESP_OK)
    {
        return result;
    }

    // Compensate raw humidity using calibration coefficients
    // and the temperature calculated earlier
    *humidity = bme680_compensate_humidity(raw_humidity);

    ESP_LOGI(TAG, "Humidity compensation completed");

    return ESP_OK;
}