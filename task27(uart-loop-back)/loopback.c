
/*
    Task 27: UART loopback using virtual COM ports or hardware

    Build using:
    cl /nologo /W4 /TC /std:c11 loopback.c

    VIRTUAL COM PORT SETUP:
    - Set USE_VIRTUAL_PORTS to 1.
    - Create a connected virtual COM port pair: COM6 and COM7.
    - COM6 sends data and COM7 receives data.
    - No physical USB-to-UART adapter is required.

    HARDWARE LOOPBACK SETUP:
    - Set USE_VIRTUAL_PORTS to 0.
    - Use the actual COM port assigned to the USB-to-UART adapter.
    - Connect the adapter's TXD pin to its RXD pin.
    - The same COM port handle is used for WriteFile() and ReadFile().
    - No second COM port is required.
*/

#include <windows.h>  // CreateFileA, GetCommState, SetCommState, SetCommTimeouts, WriteFile, ReadFile, PurgeComm, CloseHandle, GetLastError
#include <stdio.h>    // printf
#include <string.h>   // strlen, memcmp

// Select the testing mode.
// 1 = Virtual COM ports (COM6 and COM7).
// 0 = Physical USB-to-UART hardware loopback.
#define USE_VIRTUAL_PORTS 0

#if USE_VIRTUAL_PORTS

// Virtual mode: COM6 sends and COM7 receives.
// Both ports must be connected as a virtual pair.
const char *sendPortName = "\\\\.\\COM6";
const char *receivePortName = "\\\\.\\COM7";

#else

// Hardware mode: Use the actual USB-to-UART adapter COM port.
// COM5 is an example. The actual port may be COM3, COM4, COM6, etc.
// For COM10 and above, use the format \\.\COM10.
// Connect TXD to RXD on the same adapter.
const char *sendPortName = "\\\\.\\COM5";
const char *receivePortName = "\\\\.\\COM5";

#endif

// Configure one COM port to use 115200 8N1.
int ConfigureComPort(HANDLE hSerial)
{
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(hSerial, &dcb))
    {
        printf("Failed to get COM port settings. Error: %lu\n",
               GetLastError());
        return 0;
    }

    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    // Disable hardware and software flow control.
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = TRUE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(hSerial, &dcb))
    {
        printf("Failed to configure COM port. Error: %lu\n",
               GetLastError());
        return 0;
    }

    // Configure read and write timeouts.
    COMMTIMEOUTS timeouts = { 0 };

    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 500;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 500;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts))
    {
        printf("Failed to configure COM port timeouts. Error: %lu\n",
               GetLastError());
        return 0;
    }

    return 1;
}

// Close the handles without closing the same handle twice.
void ClosePorts(HANDLE hSend, HANDLE hReceive)
{
#if USE_VIRTUAL_PORTS

    if (hReceive != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hReceive);
    }

#else

    // hReceive and hSend refer to the same handle.
    (void)hReceive;

#endif

    if (hSend != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hSend);
    }
}

int main(void)
{
    // Data to transmit.
    const char sendData[] = "HELLO";
    DWORD bytesToSend = (DWORD)strlen(sendData);

    // Buffer to receive the transmitted data.
    char receiveData[sizeof(sendData)] = { 0 };

    DWORD bytesWritten = 0;
    DWORD bytesRead = 0;

    // Open the selected COM port for transmitting data.
    HANDLE hSend = CreateFileA(
        sendPortName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hSend == INVALID_HANDLE_VALUE)
    {
        printf("Failed to open %s. Error: %lu\n",
               sendPortName, GetLastError());
        return 1;
    }

    printf("Successfully opened %s.\n", sendPortName);

#if USE_VIRTUAL_PORTS

    // Virtual mode: Open the second COM port for receiving data.
    HANDLE hReceive = CreateFileA(
        receivePortName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hReceive == INVALID_HANDLE_VALUE)
    {
        printf("Failed to open %s. Error: %lu\n",
               receivePortName, GetLastError());

        ClosePorts(hSend, INVALID_HANDLE_VALUE);
        return 1;
    }

    printf("Successfully opened %s.\n", receivePortName);

#else

    // Hardware mode: Use the same COM port for sending and receiving.
    HANDLE hReceive = hSend;

#endif

    // Configure the sending COM port.
    if (!ConfigureComPort(hSend))
    {
        ClosePorts(hSend, hReceive);
        return 1;
    }

#if USE_VIRTUAL_PORTS

    // Virtual mode: Configure the receiving COM port too.
    if (!ConfigureComPort(hReceive))
    {
        ClosePorts(hSend, hReceive);
        return 1;
    }

    printf("Both ports configured to 115200 8N1.\n");

#else

    printf("COM port configured to 115200 8N1.\n");

#endif

    // Clear old data from the COM port buffers.
#if USE_VIRTUAL_PORTS

    if (!PurgeComm(hReceive, PURGE_RXCLEAR | PURGE_TXCLEAR) ||
        !PurgeComm(hSend, PURGE_RXCLEAR | PURGE_TXCLEAR))

#else

    if (!PurgeComm(hSend, PURGE_RXCLEAR | PURGE_TXCLEAR))

#endif
    {
        printf("Failed to clear COM port buffers. Error: %lu\n",
               GetLastError());

        ClosePorts(hSend, hReceive);
        return 1;
    }

    // Send HELLO through the selected sending COM port.
    printf("Sending through %s: %s\n",
           sendPortName, sendData);

    if (!WriteFile(
        hSend,
        sendData,
        bytesToSend,
        &bytesWritten,
        NULL))
    {
        printf("Failed to write data. Error: %lu\n",
               GetLastError());

        ClosePorts(hSend, hReceive);
        return 1;
    }

    if (bytesWritten != bytesToSend)
    {
        printf("Incomplete transmission. Sent %lu of %lu bytes.\n",
               bytesWritten, bytesToSend);

        ClosePorts(hSend, hReceive);
        return 1;
    }

    printf("Bytes transmitted: %lu\n", bytesWritten);

    // Read the transmitted data from the receiving COM port.
    if (!ReadFile(
        hReceive,
        receiveData,
        bytesToSend,
        &bytesRead,
        NULL))
    {
        printf("Failed to read data. Error: %lu\n",
               GetLastError());

        ClosePorts(hSend, hReceive);
        return 1;
    }

    printf("Bytes received: %lu\n", bytesRead);

    // Compare the transmitted and received data.
    if (bytesRead == bytesToSend &&
        memcmp(sendData, receiveData, bytesToSend) == 0)
    {
        receiveData[bytesRead] = '\0';

        printf("Received through %s: %s\n",
               receivePortName, receiveData);

#if USE_VIRTUAL_PORTS
        printf("Virtual UART loopback test successful!\n");
#else
        printf("Hardware UART loopback test successful!\n");
#endif
    }
    else
    {
        printf("UART loopback test failed.\n");

        printf("Expected %lu bytes but received %lu bytes.\n",
               bytesToSend, bytesRead);
    }

    // Close the COM port handles.
    ClosePorts(hSend, hReceive);

    printf("COM port handles closed successfully.\n");

    return 0;
}
