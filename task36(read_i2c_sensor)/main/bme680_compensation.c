#include "bme680_compensation.h"
#include <stdint.h>
esp_err_t bme680_compensate_temperature(
    uint32_t temp_adc,
    const bme680_calib_data_t *calib,
    int16_t *temperature_centi_c,
    int32_t *t_fine
)
{
    if (calib == NULL ||
        temperature_centi_c == NULL ||
        t_fine == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int32_t var1;
    int32_t var2;
    int32_t var3;

    // Calculate var1
    var1 = ((int32_t)temp_adc >> 3)
           - ((int32_t)calib->par_t1 << 1);

    // Calculate var2
    var2 = (var1 * (int32_t)calib->par_t2) >> 11;

    // Calculate var3
    var3 = (((var1 >> 1) * (var1 >> 1)) >> 12);

    var3 = (var3 * ((int32_t)calib->par_t3 << 4)) >> 14;

    // Calculate t_fine
    *t_fine = var2 + var3;

    // Calculate compensated temperature
    *temperature_centi_c =
        (int16_t)(((*t_fine * 5) + 128) >> 8);

    return ESP_OK;
}
esp_err_t bme680_compensate_pressure(
    uint32_t press_adc,
    const bme680_calib_data_t *calib,
    int32_t t_fine,
    uint32_t *pressure_pa
)
{
    if (calib == NULL || pressure_pa == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int32_t var1;
    int32_t var2;
    int32_t var3;

    uint32_t press_comp;

    // Step 1: Calculate var1
    var1 = ((int32_t)t_fine >> 1) - 64000;

    // Step 2: Calculate var2
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11)
           * (int32_t)calib->par_p6 >> 2;

    var2 = var2 + ((var1 * (int32_t)calib->par_p5) << 1);

    var2 = (var2 >> 2) + ((int32_t)calib->par_p4 << 16);

    // Step 3: Calculate var1 again
    var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13)
             * ((int32_t)calib->par_p3 << 5)) >> 3)
           + (((int32_t)calib->par_p2 * var1) >> 1);

    var1 = var1 >> 18;

    var1 = ((32768 + var1) * (int32_t)calib->par_p1) >> 15;

    // Prevent division by zero
    if (var1 == 0)
    {
        return ESP_ERR_INVALID_STATE;
    }

    // Step 4: Calculate initial pressure
    press_comp = 1048576 - press_adc;

    press_comp = (uint32_t)(
        ((int32_t)press_comp - (var2 >> 12)) * 3125
    );

    // Step 5: Calculate pressure compensation
    if (press_comp < (1U << 30))
    {
        press_comp = (press_comp / (uint32_t)var1) << 1;
    }
    else
    {
        press_comp = (press_comp << 1) / (uint32_t)var1;
    }

    // Step 6: Calculate final correction values
    var1 = ((int32_t)calib->par_p9 *
            (int32_t)(((press_comp >> 3) * (press_comp >> 3)) >> 13)) >> 12;

    var2 = ((int32_t)(press_comp >> 2) *
            (int32_t)calib->par_p8) >> 13;

    var3 = ((int32_t)(press_comp >> 8) *
            (int32_t)(press_comp >> 8) *
            (int32_t)(press_comp >> 8) *
            (int32_t)calib->par_p10) >> 17;

    // Step 7: Calculate final compensated pressure
    press_comp = (int32_t)press_comp +
                 ((var1 + var2 + var3 +
                   ((int32_t)calib->par_p7 << 7)) >> 4);

    *pressure_pa = press_comp;

    return ESP_OK;
}
esp_err_t bme680_compensate_humidity(
    uint16_t hum_adc,
    const bme680_calib_data_t *calib,
    int16_t temperature_centi_c,
    uint32_t *humidity
)
{
    if (calib == NULL || humidity == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int32_t temp_scaled;
    int32_t var1;
    int32_t var2;
    int32_t var3;
    int32_t var4;
    int32_t var5;
    int32_t var6;

    int32_t hum_comp;

    // Step 1: Scale compensated temperature
    temp_scaled = (int32_t)temperature_centi_c;

    // Step 2: Calculate var1
    var1 = ((int32_t)hum_adc -
            (int32_t)(((int32_t)calib->par_h1 << 4) -
            (((temp_scaled * (int32_t)calib->par_h3) /
            100) >> 1)));

    // Step 3: Calculate var2
    var2 = ((int32_t)calib->par_h2 *
            (((temp_scaled * (int32_t)calib->par_h4) / 100) +
            (((temp_scaled *
            ((temp_scaled * (int32_t)calib->par_h5) / 100)) >> 6) / 100) +
            (int32_t)(1 << 14))) >> 10;

    // Step 4: Calculate var3
    var3 = var1 * var2;

    // Step 5: Calculate var4
    var4 = (((int32_t)calib->par_h6 << 7) +
            ((temp_scaled * (int32_t)calib->par_h7) / 100)) >> 4;

    // Step 6: Calculate var5
    var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;

    // Step 7: Calculate var6
    var6 = (var4 * var5) >> 1;

    // Step 8: Calculate final humidity
    hum_comp = (((var3 + var6) >> 10) * 1000) >> 12;

    // Limit humidity to the valid 0–100% RH range
    if (hum_comp < 0)
    {
        hum_comp = 0;
    }
    else if (hum_comp > 100000)
    {
        hum_comp = 100000;
    }

    *humidity = (uint32_t)hum_comp;

    return ESP_OK;
}