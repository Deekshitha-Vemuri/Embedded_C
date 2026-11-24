#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include <stddef.h>
#include "esp_err.h"

esp_err_t uart_handler_init(void);

esp_err_t uart_handler_receive_command(
    char *command,
    size_t buffer_size
);

esp_err_t uart_handler_send_response(
    const char *response
);

#endif