
idf_component_register(
    SRCS
        "main.c"
        "bme680.c"
        "bme680_compensation.c"
        "uart_handler.c"

    INCLUDE_DIRS "."
    PRIV_REQUIRES
        esp_driver_i2c
        esp_driver_gpio
        esp_driver_uart
        log
)
