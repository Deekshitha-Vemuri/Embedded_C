
/*
    Build using MSVC Developer Command Prompt:

    cl /W4 /TC main.c /Fe:main.exe
*/

#include <windows.h>
#include <stdio.h>
#include <string.h>

// Serial port configuration
#define SERIAL_PORT "\\\\.\\COM5"
#define BAUD_RATE 115200

// Configure an already-open serial port
int ConfigureSerialPort(HANDLE hSerial)
{
    DCB serialConfig = {0};
    COMMTIMEOUTS timeouts = {0};

    // Get current serial configuration
    serialConfig.DCBlength = sizeof(serialConfig);

    if (!GetCommState(hSerial, &serialConfig))
    {
        printf("Error: GetCommState failed.\n");
        return 0;
    }

    // Configure UART: 115200 baud, 8N1
    serialConfig.BaudRate = BAUD_RATE;
    serialConfig.ByteSize = 8;
    serialConfig.Parity = NOPARITY;
    serialConfig.StopBits = ONESTOPBIT;

    serialConfig.fOutxCtsFlow = FALSE;
    serialConfig.fRtsControl = RTS_CONTROL_DISABLE;
    serialConfig.fDtrControl = DTR_CONTROL_DISABLE;

    if (!SetCommState(hSerial, &serialConfig))
    {
        printf("Error: SetCommState failed.\n");
        return 0;
    }

    // Configure communication timeouts
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.ReadTotalTimeoutConstant = 1000;

    timeouts.WriteTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 1000;

    if (!SetCommTimeouts(hSerial, &timeouts))
    {
        printf("Error: SetCommTimeouts failed.\n");
        return 0;
    }

    // Clear old serial data
    if (!PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR))
    {
        printf("Error: PurgeComm failed.\n");
        return 0;
    }

    printf("UART configured: 115200 baud, 8N1.\n");

    return 1;
}

// Open and configure COM port
HANDLE OpenSerialPort(void)
{
    HANDLE hSerial;

    hSerial = CreateFileA(
        SERIAL_PORT,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hSerial == INVALID_HANDLE_VALUE)
    {
        return INVALID_HANDLE_VALUE;
    }

    if (!ConfigureSerialPort(hSerial))
    {
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    return hSerial;
}

// Keep retrying until ESP32 is available
HANDLE ConnectToESP32(void)
{
    HANDLE hSerial;

    while (1)
    {
        printf("Attempting to connect to ESP32...\n");

        hSerial = OpenSerialPort();

        if (hSerial != INVALID_HANDLE_VALUE)
        {
            printf("COM5 opened and configured successfully.\n");

            // Give ESP32 time to initialize
            Sleep(2000);

            return hSerial;
        }

        printf("ESP32 not available. Retrying in 2 seconds...\n");

        Sleep(2000);
    }
}

int main(void)
{
    HANDLE hSerial;

    char command[] = "STATUS\r\n";
    char response[256];

    DWORD bytesWritten;
    DWORD bytesRead;

    printf("ESP32 Continuous UART Communication\n");

    // 1. Establish initial connection
    hSerial = ConnectToESP32();

    // 2. Continuously communicate with ESP32
    while (1)
    {
        bytesWritten = 0;
        bytesRead = 0;

        memset(response, 0, sizeof(response));

        printf("\nSending command: STATUS\n");

        // Send STATUS command
        if (!WriteFile(
                hSerial,
                command,
                (DWORD)strlen(command),
                &bytesWritten,
                NULL))
        {
            printf("ESP32 communication failed during WriteFile.\n");

            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;

            Sleep(2000);

            hSerial = ConnectToESP32();

            continue;
        }

        if (bytesWritten != strlen(command))
        {
            printf("Incomplete command transmission.\n");

            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;

            hSerial = ConnectToESP32();

            continue;
        }

        printf("Command sent successfully.\n");
        printf("Waiting for ESP32 response...\n");

        int communication_failed = 0;

        // Read response until newline or timeout
        while (bytesRead < sizeof(response) - 1)
        {
            char ch;
            DWORD received = 0;

            if (!ReadFile(
                    hSerial,
                    &ch,
                    1,
                    &received,
                    NULL))
            {
                printf("ESP32 communication failed during ReadFile.\n");

                communication_failed = 1;
                break;
            }

            if (received == 0)
            {
                break;
            }

            response[bytesRead++] = ch;

            if (ch == '\n')
            {
                break;
            }
        }

        if (communication_failed)
        {
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;

            Sleep(2000);

            hSerial = ConnectToESP32();

            continue;
        }

        response[bytesRead] = '\0';

        // Display response
        if (bytesRead > 0)
        {
            printf("ESP32 Response: %s", response);
        }
        else
        {
            printf("Timeout: No response received.\n");
        }

        // Wait before sending the next command
        Sleep(2000);
    }

    // Normally unreachable because the loop is continuous
    CloseHandle(hSerial);

    printf("\nSerial port closed.\n");

    return 0;
}
