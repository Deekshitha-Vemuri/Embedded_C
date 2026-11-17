
/*
 * Task 29: Serial Timeout and Reconnect Controller
 *
 * Description:
 * Send a packet to ESP32, receive its ACK, detect communication
 * timeout, close the COM port and reconnect automatically.
 *
 * Build command:
 * cl /nologo /W4 /TC /std:c11 reconnect_controller.c
 */

#include <stdio.h>
#include <stdint.h>
#include <windows.h>

/* Configuration */

#define COM_PORT           "COM5"
#define BAUD_RATE          115200

#define PACKET_SIZE        8
#define ACK_BYTE           0x06

#define PACKET_INTERVAL    1000
#define CONNECTION_TIMEOUT 3000

#define INITIAL_RETRY_MS   1000
#define MAX_RETRY_MS       5000

/* Connection states */

typedef enum
{
    CONNECTED,
    DISCONNECTED,
    RETRYING

} ConnectionState;

/* Global connection state */

ConnectionState current_state = DISCONNECTED;

/* Packet sent to ESP32 */

const uint8_t packet[PACKET_SIZE] =
{
    0xAA, 0x55, 0x01, 0xC4,
    0x09, 0x88, 0x13, 0xA8
};

/* Print error details */

void log_error(const char *operation)
{
    DWORD error_code = GetLastError();

    printf("[ERROR] %s failed. Windows error code: %lu\n",
           operation, error_code);
}

/* Display connection state changes */

void set_state(ConnectionState new_state)
{
    if (current_state == new_state)
        return;

    current_state = new_state;

    switch (new_state)
    {
        case CONNECTED:
            printf("[STATE] CONNECTED\n");
            break;

        case DISCONNECTED:
            printf("[STATE] DISCONNECTED\n");
            break;

        case RETRYING:
            printf("[STATE] RETRYING\n");
            break;
    }
}

/* Open and configure the COM port */

HANDLE open_serial_port(void)
{
    HANDLE hSerial;
    DCB dcb = {0};
    COMMTIMEOUTS timeouts = {0};

    char port_path[] = "\\\\.\\COM5";

    printf("[INFO] Opening %s...\n", COM_PORT);

    hSerial = CreateFileA(
        port_path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hSerial == INVALID_HANDLE_VALUE)
    {
        log_error("CreateFileA");
        return INVALID_HANDLE_VALUE;
    }

    /* Get existing COM port settings */

    dcb.DCBlength = sizeof(dcb);

    if (!GetCommState(hSerial, &dcb))
    {
        log_error("GetCommState");
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    /* Configure 115200 baud, 8N1 */

    dcb.BaudRate = BAUD_RATE;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;

    if (!SetCommState(hSerial, &dcb))
    {
        log_error("SetCommState");
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    /* Configure read and write timeouts */

    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 100;

    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 500;

    if (!SetCommTimeouts(hSerial, &timeouts))
    {
        log_error("SetCommTimeouts");
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    /* Clear old data from serial buffers */

    if (!PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR))
    {
        log_error("PurgeComm");
        CloseHandle(hSerial);
        return INVALID_HANDLE_VALUE;
    }

    printf("[INFO] %s opened successfully.\n", COM_PORT);
    printf("[INFO] Serial configuration: 115200 8N1\n");

    return hSerial;
}

/* Send packet to ESP32 */

int send_packet(HANDLE hSerial)
{
    DWORD bytes_written = 0;

    BOOL result = WriteFile(
        hSerial,
        packet,
        PACKET_SIZE,
        &bytes_written,
        NULL
    );

    if (!result)
    {
        log_error("WriteFile");
        return 0;
    }

    if (bytes_written != PACKET_SIZE)
    {
        printf("[ERROR] Incomplete packet transmission.\n");
        printf("[ERROR] Expected %d bytes, sent %lu bytes.\n",
               PACKET_SIZE, bytes_written);

        return 0;
    }

    printf("[TX] Packet sent successfully (%lu bytes).\n",
           bytes_written);

    return 1;
}

/* Receive and validate ESP32 ACK */

int receive_ack(HANDLE hSerial)
{
    uint8_t received_byte = 0;
    DWORD bytes_read = 0;

    BOOL result = ReadFile(
        hSerial,
        &received_byte,
        1,
        &bytes_read,
        NULL
    );

    if (!result)
    {
        log_error("ReadFile");
        return -1;
    }

    /* No data received before the read timeout */

    if (bytes_read == 0)
        return 0;

    /* Check whether ESP32 returned the correct ACK */

    if (received_byte == ACK_BYTE)
    {
        printf("[RX] Valid ACK received: 0x%02X\n",
               received_byte);

        return 1;
    }

    printf("[WARN] Unexpected response: 0x%02X\n",
           received_byte);

    return 0;
}

/* Close COM port and prepare for reconnection */

void disconnect_port(HANDLE *hSerial)
{
    set_state(DISCONNECTED);

    if (*hSerial != INVALID_HANDLE_VALUE)
    {
        printf("[INFO] Closing COM port handle.\n");

        if (!CloseHandle(*hSerial))
        {
            log_error("CloseHandle");
        }

        *hSerial = INVALID_HANDLE_VALUE;
    }

    set_state(RETRYING);
}

/* Main reconnect controller */

int main(void)
{
    HANDLE hSerial = INVALID_HANDLE_VALUE;

    ULONGLONG last_valid_packet_time = 0;
    ULONGLONG next_packet_time = 0;
    ULONGLONG next_retry_time = 0;

    DWORD retry_delay = INITIAL_RETRY_MS;

    printf("====================================\n");
    printf(" Task 29: Serial Reconnect Controller\n");
    printf("====================================\n");

    printf("[INFO] Monitoring %s\n", COM_PORT);

    while (1)
    {
        ULONGLONG current_time = GetTickCount64();

        /*
         * STEP 1:
         * Open COM port when disconnected.
         */

        if (hSerial == INVALID_HANDLE_VALUE)
        {
            if (current_time < next_retry_time)
            {
                Sleep(50);
                continue;
            }

            printf("\n[INFO] Attempting COM port connection...\n");

            hSerial = open_serial_port();

            if (hSerial == INVALID_HANDLE_VALUE)
            {
                set_state(DISCONNECTED);

                printf("[WARN] COM port connection failed.\n");

                set_state(RETRYING);

                printf("[INFO] Next retry in %lu ms.\n",
                       retry_delay);

                next_retry_time =
                    GetTickCount64() + retry_delay;

                retry_delay *= 2;

                if (retry_delay > MAX_RETRY_MS)
                    retry_delay = MAX_RETRY_MS;

                continue;
            }

            /*
             * Port opened, but the ESP32 has not
             * confirmed communication yet.
             */

            last_valid_packet_time = GetTickCount64();

            next_packet_time = GetTickCount64();

            printf("[INFO] Waiting for ESP32 ACK.\n");
        }

        current_time = GetTickCount64();

        /*
         * STEP 2:
         * Send a packet every second.
         */

        if (current_time >= next_packet_time)
        {
            if (!send_packet(hSerial))
            {
                disconnect_port(&hSerial);

                next_retry_time =
                    GetTickCount64() + retry_delay;

                retry_delay *= 2;

                if (retry_delay > MAX_RETRY_MS)
                    retry_delay = MAX_RETRY_MS;

                continue;
            }

            next_packet_time =
                GetTickCount64() + PACKET_INTERVAL;
        }

        /*
         * STEP 3:
         * Read ACK from ESP32.
         */

        int ack_status = receive_ack(hSerial);

        if (ack_status == -1)
        {
            disconnect_port(&hSerial);

            next_retry_time =
                GetTickCount64() + retry_delay;

            retry_delay *= 2;

            if (retry_delay > MAX_RETRY_MS)
                retry_delay = MAX_RETRY_MS;

            continue;
        }

        /*
         * STEP 4:
         * Update time only after a valid ACK.
         */

        if (ack_status == 1)
        {
            last_valid_packet_time = GetTickCount64();

            set_state(CONNECTED);

            /* Reset retry delay after successful communication */

            retry_delay = INITIAL_RETRY_MS;
        }

        /*
         * STEP 5:
         * Check whether 3 seconds have passed
         * without receiving a valid ACK.
         */

        current_time = GetTickCount64();

        if (current_time - last_valid_packet_time >=
            CONNECTION_TIMEOUT)
        {
            printf("\n[WARN] No valid ACK received for 3 seconds.\n");

            printf("[WARN] Communication timeout detected.\n");

            disconnect_port(&hSerial);

            printf("[INFO] Next retry in %lu ms.\n",
                   retry_delay);

            next_retry_time =
                GetTickCount64() + retry_delay;

            retry_delay *= 2;

            if (retry_delay > MAX_RETRY_MS)
                retry_delay = MAX_RETRY_MS;

            continue;
        }

        /*
         * STEP 6:
         * Small delay to avoid continuously using the CPU.
         */

        Sleep(10);
    }

    return 0;
}
