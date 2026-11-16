
/*
    Build command:
    cl /nologo /W4 /TC /std:c11 serial_config.c
*/

#include <stdio.h>
#include <windows.h>

int main(void)
{
    printf("Opening COM port: \\\\.\\COM5\n");
    // Open the existing COM5 port
    HANDLE hSerial = CreateFileW(
        L"\\\\.\\COM5",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hSerial == INVALID_HANDLE_VALUE)
    {
        printf("Error opening COM5: %lu\n", GetLastError());
        return 1;
    }

    // Get the current COM port settings
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(hSerial, &dcb))
    {
        printf("Error getting COM port settings: %lu\n", GetLastError());
        CloseHandle(hSerial);
        return 1;
    }

    // Configure the COM port to 115200 8N1
    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;

    // Apply the new settings
    if (!SetCommState(hSerial, &dcb))
    {
        printf("Error configuring COM port: %lu\n", GetLastError());
        CloseHandle(hSerial);
        return 1;
    }

    // Configure read and write timeouts
    COMMTIMEOUTS timeouts = {0};

    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;

    if (!SetCommTimeouts(hSerial, &timeouts))
    {
        printf("Error setting COM port timeouts: %lu\n", GetLastError());
        CloseHandle(hSerial);
        return 1;
    }

    // Retrieve the actual settings after configuration
    if (!GetCommState(hSerial, &dcb))
    {
        printf("Error retrieving COM port settings: %lu\n", GetLastError());
        CloseHandle(hSerial);
        return 1;
    }

    // Print the actual COM port settings
    printf("\nCOM Port Configuration\n");
    printf("----------------------\n");
    printf("COM Port : COM5\n");
    printf("Baud Rate: %lu\n", dcb.BaudRate);
    printf("Data Bits: %u\n", dcb.ByteSize);

    printf("Parity   : ");
    switch (dcb.Parity)
    {
        case NOPARITY:
            printf("None\n");
            break;
        case ODDPARITY:
            printf("Odd\n");
            break;
        case EVENPARITY:
            printf("Even\n");
            break;
        case MARKPARITY:
            printf("Mark\n");
            break;
        case SPACEPARITY:
            printf("Space\n");
            break;
        default:
            printf("Unknown\n");
    }

    printf("Stop Bits: ");
    switch (dcb.StopBits)
    {
        case ONESTOPBIT:
            printf("1\n");
            break;
        case ONE5STOPBITS:
            printf("1.5\n");
            break;
        case TWOSTOPBITS:
            printf("2\n");
            break;
        default:
            printf("Unknown\n");
    }

    // Close the COM port
    CloseHandle(hSerial);

    return 0;
}
