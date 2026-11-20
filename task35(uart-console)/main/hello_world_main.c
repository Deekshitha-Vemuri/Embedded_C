
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_err.h"

// UART configuration
#define UART_PORT UART_NUM_0
#define UART_BAUD_RATE 115200
#define UART_BUFFER_SIZE 1024

// UART communication settings
static const uart_config_t uart_config = {
    .baud_rate = UART_BAUD_RATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT
};

void app_main(void)
{
    // Apply UART configuration
    ESP_ERROR_CHECK(
        uart_param_config(UART_PORT, &uart_config)
    );

    // Configure UART pins
    ESP_ERROR_CHECK(
        uart_set_pin(
            UART_PORT,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE
        )
    );

    // Install UART driver
    ESP_ERROR_CHECK(
        uart_driver_install(
            UART_PORT,
            UART_BUFFER_SIZE,
            UART_BUFFER_SIZE,
            0,
            NULL,
            0
        )
    );

    char command[128];
    int command_len = 0;
    int discard_command = 0;

    // Continuously listen for commands
    while (1)
    {
        uint8_t rx_data[128];

        int len = uart_read_bytes(
            UART_PORT,
            rx_data,
            sizeof(rx_data),
            pdMS_TO_TICKS(100)
        );

        for (int i = 0; i < len; i++)
        {
            if (rx_data[i] == '\n')
            {
                if (!discard_command)
                {
                    command[command_len] = '\0';

                    if (strcmp(command, "STATUS") == 0)
                    {
						//printf("STATUS command received!\n");
                        int64_t time_us = esp_timer_get_time();
                        int64_t uptime_ms = time_us / 1000;

                        char response[64];

                        int response_len = snprintf(
                            response,
                            sizeof(response),
                            "OK uptime_ms=%" PRId64 "\r\n",
                            uptime_ms
                        );

                        if (response_len > 0 &&
                            response_len < sizeof(response))
                        {
                            uart_write_bytes(
                                UART_PORT,
                                response,
                                response_len
                            );
                        }
                    }
                }

                command_len = 0;
                discard_command = 0;
            }
            else if (rx_data[i] != '\r' && !discard_command)
            {
                if (command_len < sizeof(command) - 1)
                {
                    command[command_len++] = (char)rx_data[i];
                }
                else
                {
                    command_len = 0;
                    discard_command = 1;
                }
            }
        }

        if (len == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
