
#include "uart.h"

#include <stdio.h>
#include <string.h>

// Read a complete line sent by the Windows application.
bool uart_read_line(char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0)
    {
        return false;
    }

    // Read characters from the UART0 console until Enter is received.
    if (fgets(buffer, buffer_size, stdin) == NULL)
    {
        return false;
    }

    // Remove newline and carriage return characters.
    buffer[strcspn(buffer, "\r\n")] = '\0';

    return true;
}

// Send a response through the ESP32 USB serial connection.
void uart_send_line(const char *message)
{
    if (message == NULL)
    {
        return;
    }

    printf("%s\n", message);
    fflush(stdout);
}
