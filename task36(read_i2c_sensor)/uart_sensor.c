// build:cl /W4 /TC uart_sensor.c /Fe:uart_sensor.exe
#include <stdio.h>
#include <windows.h>
#include <string.h>
#define PORT_NAME       "\\\\.\\COM5"
#define BAUD_RATE       CBR_115200
#define BUFFER_SIZE     256

// Open and configure the serial port
HANDLE open_serial_port(void)
{
    HANDLE serial_port = CreateFileA(
        PORT_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (serial_port == INVALID_HANDLE_VALUE)
    {
        printf("Error: Could not open COM5. Error code: %lu\n",
               GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    DCB serial_config = {0};
    serial_config.DCBlength = sizeof(serial_config);

    if (!GetCommState(serial_port, &serial_config))
    {
        printf("Error: Could not get serial configuration.\n");
        CloseHandle(serial_port);
        return INVALID_HANDLE_VALUE;
    }

    serial_config.BaudRate = BAUD_RATE;
    serial_config.ByteSize = 8;
    serial_config.StopBits = ONESTOPBIT;
    serial_config.Parity = NOPARITY;

    // Avoid changing ESP32 auto-reset control lines
    serial_config.fDtrControl = DTR_CONTROL_DISABLE;
    serial_config.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(serial_port, &serial_config))
    {
        printf("Error: Could not configure COM5.\n");
        CloseHandle(serial_port);
        return INVALID_HANDLE_VALUE;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 1000;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    if (!SetCommTimeouts(serial_port, &timeouts))
    {
        printf("Error: Could not set serial timeouts.\n");
        CloseHandle(serial_port);
        return INVALID_HANDLE_VALUE;
    }

    return serial_port;
}

// Send command to ESP32
int send_sensor_command(HANDLE serial_port)
{
    const char command[] = "READ_SENSOR\n";
    DWORD bytes_written = 0;

    if (!WriteFile(
            serial_port,
            command,
            (DWORD)strlen(command),
            &bytes_written,
            NULL))
    {
        printf("Error: Failed to send command.\n");
        return 0;
    }

    if (bytes_written != strlen(command))
    {
        printf("Error: Command was not completely sent.\n");
        return 0;
    }

    printf("Command sent: READ_SENSOR\n");
    return 1;
}

// Read one complete line from ESP32
int receive_line(
    HANDLE serial_port,
    char *buffer,
    DWORD buffer_size
)
{
    DWORD bytes_read;
    DWORD start_time = GetTickCount();
    DWORD index = 0;
    char received_char;

    while ((DWORD)(GetTickCount() - start_time) < 10000)
    {
        if (!ReadFile(
                serial_port,
                &received_char,
                1,
                &bytes_read,
                NULL))
        {
            printf("Error: Failed to read from COM5.\n");
            return 0;
        }

        if (bytes_read == 0)
            continue;

        if (received_char == '\r')
            continue;

        if (received_char == '\n')
        {
            buffer[index] = '\0';
            return 1;
        }

        if (index < buffer_size - 1)
        {
            buffer[index++] = received_char;
        }
        else
        {
            index = 0;
        }
    }

    return 0;
}

// Receive and display the three sensor readings
void receive_sensor_readings(HANDLE serial_port)
{
    char response[BUFFER_SIZE];

    printf("\nWaiting for sensor response...\n");

    while (receive_line(serial_port, response, sizeof(response)))
    {
        // Ignore ESP-IDF log messages
        if (strncmp(response, "DATA,", 5) == 0)
        {
            float temperature;
            float humidity;
            float pressure;

            if (sscanf(
                    response,
                    "DATA,T=%f,RH=%f,P=%f",
                    &temperature,
                    &humidity,
                    &pressure) == 3)
            {
                printf("\n========== BME680 SENSOR DATA ==========\n");
                printf("Temperature : %.2f C\n", temperature);
                printf("Humidity    : %.3f %%\n", humidity);
                printf("Pressure    : %.2f hPa\n", pressure);
                printf("========================================\n");
                return;
            }

            printf("Invalid sensor response: %s\n", response);
        }
        else if (strncmp(response, "ERROR,", 6) == 0)
        {
            printf("ESP32 reported: %s\n", response);
            return;
        }
    }

    printf("Timeout: No sensor response received within 10 seconds.\n");
}

int main(void)
{
    printf("========================================\n");
    printf("       BME680 UART SENSOR READER\n");
    printf("========================================\n");

    HANDLE serial_port = open_serial_port();

    if (serial_port == INVALID_HANDLE_VALUE)
        return 1;

    printf("COM5 opened successfully.\n");

    // Clear old received data before sending a new command
    PurgeComm(serial_port, PURGE_RXCLEAR | PURGE_TXCLEAR);

    if (send_sensor_command(serial_port))
    {
        receive_sensor_readings(serial_port);
    }

    CloseHandle(serial_port);

    printf("\nSerial port closed.\n");

    return 0;
}
