#include "uart_handler.h"

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define UART_PORT       UART_NUM_0
#define UART_BAUD_RATE  115200
#define UART_RX_BUFFER  1024
#define UART_TX_BUFFER  1024

esp_err_t uart_handler_init(void)
{
    // UART communication configuration
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    // Apply UART configuration
    esp_err_t result = uart_param_config(UART_PORT, &uart_config);

    if (result != ESP_OK)
    {
        return result;
    }

    // Use the default UART0 TX and RX pins
    result = uart_set_pin(
        UART_PORT,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    );

    if (result != ESP_OK)
    {
        return result;
    }

    // Install UART driver and allocate buffers
    result = uart_driver_install(
        UART_PORT,
        UART_RX_BUFFER,
        UART_TX_BUFFER,
        0,
        NULL,
        0
    );

    return result;
}


esp_err_t uart_handler_receive_command(
    char *command,
    size_t buffer_size
)
{
    if (command == NULL || buffer_size < 2)
    {
        return ESP_ERR_INVALID_ARG;
    }

    size_t index = 0;
    char received_char;

    while (1)
    {
        int bytes_read = uart_read_bytes(
            UART_PORT,
            &received_char,
            1,
            pdMS_TO_TICKS(1000)
        );

        if (bytes_read <= 0)
        {
            return ESP_ERR_TIMEOUT;
        }

        // Ignore carriage return
        if (received_char == '\r')
        {
            continue;
        }

        // Newline indicates the end of the command
        if (received_char == '\n')
        {
            command[index] = '\0';
            return ESP_OK;
        }

        // Check whether the command buffer has space
        if (index >= buffer_size - 1)
        {
            command[index] = '\0';
            return ESP_ERR_INVALID_SIZE;
        }

        command[index++] = received_char;
    }
}
esp_err_t uart_handler_send_response(const char *response)
{
    if (response == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int bytes_written = uart_write_bytes(
        UART_PORT,
        response,
        strlen(response)
    );

    if (bytes_written != (int)strlen(response))
    {
        return ESP_FAIL;
    }

    // Send newline to mark the end of the response
    bytes_written = uart_write_bytes(UART_PORT, "\r\n", 2);

    if (bytes_written != 2)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}