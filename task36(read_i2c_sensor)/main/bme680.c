#include "driver/i2c_master.h"
#include "bme680.h"
#include "esp_log.h"
#define I2C_PORT       I2C_NUM_0
#define I2C_SDA_GPIO   GPIO_NUM_21
#define I2C_SCL_GPIO   GPIO_NUM_22
#define I2C_FREQUENCY  100000
static const char *TAG = "BME680";
static i2c_master_bus_handle_t bus_handle;
esp_err_t bme680_init(void)  
/* esp_err_t is the return type if initialization succeded or not has 3 return values .ESP_OK and ESP_FAIL and ESP_ERR_INVALID_ARG*/
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
