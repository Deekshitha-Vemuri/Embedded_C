
#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stddef.h>

// Read one complete command from the ESP32 USB serial console.
bool uart_read_line(char *buffer, size_t buffer_size);

// Send a response from ESP32 to the Windows application.
void uart_send_line(const char *message);

#endif
