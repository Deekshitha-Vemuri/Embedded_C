
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BAUD_RATE 115200
#define MIN_INTERVAL_SECONDS 1
#define MAX_INTERVAL_SECONDS 60

static HANDLE serial_handle = INVALID_HANDLE_VALUE;
static volatile LONG application_running = 1;

// --------------------------------------------------
// Read and validate interval entered by the user
// --------------------------------------------------

int get_sampling_interval(unsigned int *seconds)
{
    char input[64];
    char *end_pointer;

    printf("Enter sampling interval (1-60 seconds): ");

    if (fgets(input, sizeof(input), stdin) == NULL)
    {
        return 0;
    }

    unsigned long value = strtoul(input, &end_pointer, 10);

    // Ignore trailing spaces.
    while (isspace((unsigned char)*end_pointer))
    {
        end_pointer++;
    }

    // Validate the entered value.
    if (end_pointer == input ||
        *end_pointer != '\0' ||
        value < MIN_INTERVAL_SECONDS ||
        value > MAX_INTERVAL_SECONDS)
    {
        printf("Invalid interval! Please enter 1-60 seconds.\n");
        return 0;
    }

    *seconds = (unsigned int)value;

    return 1;
}

// --------------------------------------------------
// Open and configure ESP32 serial connection
// --------------------------------------------------

int serial_connect(const char *port_name)
{
    char device_name[32];

    snprintf(device_name, sizeof(device_name),
             "\\\\.\\%s", port_name);

    serial_handle = CreateFileA(
        device_name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (serial_handle == INVALID_HANDLE_VALUE)
    {
        printf("ERROR: Could not open %s.\n", port_name);
        printf("Close ESP-IDF Monitor and check the COM port.\n");
        return 0;
    }

    // Configure serial communication.
    DCB settings = {0};
    settings.DCBlength = sizeof(settings);

    if (!GetCommState(serial_handle, &settings))
    {
        printf("ERROR: Could not read serial settings.\n");
        CloseHandle(serial_handle);
        serial_handle = INVALID_HANDLE_VALUE;
        return 0;
    }

    settings.BaudRate = BAUD_RATE;
    settings.ByteSize = 8;
    settings.StopBits = ONESTOPBIT;
    settings.Parity = NOPARITY;
    settings.fBinary = TRUE;
    settings.fParity = FALSE;

    // Avoid asserting the ESP32 auto-reset control signals.
    settings.fDtrControl = DTR_CONTROL_DISABLE;
    settings.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(serial_handle, &settings))
    {
        printf("ERROR: Could not configure serial port.\n");
        CloseHandle(serial_handle);
        serial_handle = INVALID_HANDLE_VALUE;
        return 0;
    }

    // Configure serial read and write timeouts.
    COMMTIMEOUTS timeouts = {0};

    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.ReadTotalTimeoutMultiplier = 10;

    timeouts.WriteTotalTimeoutConstant = 1000;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(serial_handle, &timeouts))
    {
        printf("ERROR: Could not configure serial timeouts.\n");
        CloseHandle(serial_handle);
        serial_handle = INVALID_HANDLE_VALUE;
        return 0;
    }

    return 1;
}

// --------------------------------------------------
// Send interval and START commands to ESP32
// --------------------------------------------------

int send_sampling_command(unsigned int seconds)
{
    unsigned int milliseconds = seconds * 1000;

    char command[128];

    // Both commands are sent in one line.
    snprintf(command, sizeof(command),
             "SET_INTERVAL %u; START\n",
             milliseconds);

    DWORD bytes_written = 0;
    DWORD length = (DWORD)strlen(command);

    if (!WriteFile(
            serial_handle,
            command,
            length,
            &bytes_written,
            NULL))
    {
        printf("ERROR: Could not send command to ESP32.\n");
        return 0;
    }

    if (bytes_written != length)
    {
        printf("ERROR: Incomplete command transmission.\n");
        return 0;
    }

    printf("\nSampling interval sent: %u seconds\n", seconds);

    return 1;
}

// --------------------------------------------------
// Display only relevant ESP32 messages
// --------------------------------------------------

void display_esp32_message(const char *message)
{
    // Display protocol acknowledgements.
    if (strncmp(message, "ACK;", 4) == 0)
    {
        printf("[ESP32] %s\n", message);
    }

    // Display sensor readings.
    else if (strncmp(message, "DATA;", 5) == 0)
    {
        float temperature;
        float pressure;
        float humidity;
        char extra;

        if (sscanf(message,
                   "DATA;%f;%f;%f%c",
                   &temperature,
                   &pressure,
                   &humidity,
                   &extra) == 3)
        {
            printf("\nTemperature : %.2f C\n", temperature);
            printf("Pressure    : %.2f Pa\n", pressure);
            printf("Humidity    : %.2f %%\n", humidity);
        }
    }

    // Ignore boot messages and ESP-IDF debug logs.
}

// --------------------------------------------------
// Background thread for receiving ESP32 messages
// --------------------------------------------------

DWORD WINAPI serial_reader(LPVOID parameter)
{
    char buffer[256];
    char line[512];

    DWORD bytes_read;
    size_t line_length = 0;

    while (InterlockedCompareExchange(
               &application_running, 1, 1))
    {
        if (ReadFile(
                serial_handle,
                buffer,
                sizeof(buffer),
                &bytes_read,
                NULL))
        {
            for (DWORD i = 0; i < bytes_read; i++)
            {
                char ch = buffer[i];

                if (ch == '\n')
                {
                    line[line_length] = '\0';

                    if (line_length > 0)
                    {
                        display_esp32_message(line);
                        fflush(stdout);
                    }

                    line_length = 0;
                }
                else if (ch != '\r')
                {
                    if (line_length < sizeof(line) - 1)
                    {
                        line[line_length++] = ch;
                    }
                    else
                    {
                        line_length = 0;
                    }
                }
            }
        }
        else
        {
            printf("\nERROR: Serial communication failed.\n");
            break;
        }
    }

    return 0;
}

// --------------------------------------------------
// Main application
// --------------------------------------------------

int main(void)
{
    unsigned int interval_seconds;

    printf("========================================\n");
    printf("      TASK 37 DIAGNOSTIC APPLICATION\n");
    printf("========================================\n\n");

    // Ask for the interval before opening COM5.
    if (!get_sampling_interval(&interval_seconds))
    {
        return 1;
    }

    printf("\nConnecting to ESP32 on COM5...\n");

    // Open the ESP32 serial connection.
    if (!serial_connect("COM5"))
    {
        return 1;
    }

    printf("ESP32 connected successfully!\n");

    printf("Sampling interval: %u seconds\n",
           interval_seconds);

    // Start the background receiving thread.
    HANDLE reader_thread = CreateThread(
        NULL,
        0,
        serial_reader,
        NULL,
        0,
        NULL
    );

    if (reader_thread == NULL)
    {
        printf("ERROR: Could not create reader thread.\n");
        CloseHandle(serial_handle);
        return 1;
    }

    // Send interval and START commands.
    if (!send_sampling_command(interval_seconds))
    {
        InterlockedExchange(&application_running, 0);

        WaitForSingleObject(reader_thread, INFINITE);

        CloseHandle(reader_thread);
        CloseHandle(serial_handle);

        return 1;
    }

    printf("\nWaiting for sensor readings...\n");
    printf("Press ENTER to exit.\n");

    // Wait until the user presses Enter.
    getchar();

    // Stop the receiving thread.
    InterlockedExchange(&application_running, 0);

    // Wait for the reader thread to finish.
    WaitForSingleObject(reader_thread, 1000);

    CloseHandle(reader_thread);
    CloseHandle(serial_handle);

    printf("\nDisconnected from ESP32.\n");

    return 0;
}
