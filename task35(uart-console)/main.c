/*

    Build using MSVC Developer Command Prompt:

    cl /W4 /TC main.c /Fe:task35_windows.exe
*/

#include <windows.h>
#include <stdio.h>
#include <string.h>

// Serial port configuration
#define SERIAL_PORT "\\\\.\\COM5"
#define BAUD_RATE 115200

int main(void)
{
    HANDLE hSerial;
    DCB serialConfig = {0};
    COMMTIMEOUTS timeouts = {0};

    char command[] = "STATUS\r\n";
    char response[256] = {0};

    DWORD bytesWritten = 0;
    DWORD bytesRead = 0;

    printf("ESP32 UART Communication\n");

    // ------------------------------------------------
    // 1. Open the COM port
    // ------------------------------------------------

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
        printf("Error: Cannot open COM5.\n");
        printf("Check the USB connection and COM port.\n");
        return 1;
    }

    printf("COM5 opened successfully.\n");

    // ------------------------------------------------
    // 2. Get the current serial port configuration
    // ------------------------------------------------

    if (!GetCommState(hSerial, &serialConfig))
    {
        printf("Error: GetCommState failed.\n");
        CloseHandle(hSerial);
        return 1;
    }

    // ------------------------------------------------
    // 3. Configure UART communication
    // ------------------------------------------------

    serialConfig.BaudRate = BAUD_RATE;
    serialConfig.ByteSize = 8;
    serialConfig.Parity = NOPARITY;
    serialConfig.StopBits = ONESTOPBIT;

    // Disable hardware flow control
    serialConfig.fOutxCtsFlow = FALSE;
    serialConfig.fRtsControl = RTS_CONTROL_DISABLE;

    // Avoid changing ESP32 reset-control lines
    serialConfig.fDtrControl = DTR_CONTROL_DISABLE;

    if (!SetCommState(hSerial, &serialConfig))
    {
        printf("Error: SetCommState failed.\n");
        CloseHandle(hSerial);
        return 1;
    }

    printf("UART configured: 115200 baud, 8N1.\n");

    // ------------------------------------------------
    // 4. Configure read and write timeouts
    // ------------------------------------------------

    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.ReadTotalTimeoutConstant = 1000;

    timeouts.WriteTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 1000;

    if (!SetCommTimeouts(hSerial, &timeouts))
    {
        printf("Error: SetCommTimeouts failed.\n");
        CloseHandle(hSerial);
        return 1;
    }

    // ------------------------------------------------
    // 5. Clear old serial data
    // ------------------------------------------------

    PurgeComm(
        hSerial,
        PURGE_RXCLEAR | PURGE_TXCLEAR
    );

    // Give ESP32 time to finish booting
    Sleep(2000);

    // ------------------------------------------------
    // 6. Send STATUS command to ESP32
    // ------------------------------------------------

    printf("\nSending command: STATUS\n");

    if (!WriteFile(
            hSerial,
            command,
            (DWORD)strlen(command),
            &bytesWritten,
            NULL))
    {
        printf("Error: WriteFile failed.\n");
        CloseHandle(hSerial);
        return 1;
    }

    if (bytesWritten != strlen(command))
    {
        printf("Error: Incomplete command transmission.\n");
        CloseHandle(hSerial);
        return 1;
    }

    printf("Command sent successfully.\n");

    // ------------------------------------------------
    // 7. Read response from ESP32
    // ------------------------------------------------

    printf("Waiting for ESP32 response...\n");

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
            printf("Error: ReadFile failed. Device may have disconnected.\n");
            CloseHandle(hSerial);
            return 1;
        }

        // Stop when no more data arrives
        if (received == 0)
        {
            break;
        }

        response[bytesRead++] = ch;

        // Stop after receiving a complete line
        if (ch == '\n')
        {
            break;
        }
    }

    response[bytesRead] = '\0';

    // ------------------------------------------------
    // 8. Display ESP32 response
    // ------------------------------------------------

    if (bytesRead > 0)
    {
        printf("\nESP32 Response: %s", response);
    }
    else
    {
        printf("\nTimeout: No response received from ESP32.\n");
    }

    // ------------------------------------------------
    // 9. Close the serial port
    // ------------------------------------------------

    CloseHandle(hSerial);

    printf("\nSerial port closed.\n");

    return 0;
}
