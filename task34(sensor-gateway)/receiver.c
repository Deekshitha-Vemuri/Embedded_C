
/*
 * Task 34 - Windows Simulated Sensor Gateway
 * Receiver Application
 *
 * Build command (MSVC Developer Command Prompt):
 * cl /nologo /W4 /TC /std:c11 receiver.c /Fe:receiver.exe
 *
 * Windows APIs used:
 * CreateNamedPipeA()
 * ConnectNamedPipe()
 * ReadFile()
 * DisconnectNamedPipe()
 * CloseHandle()
 * GetLastError()
 *
 * C functions used:
 * printf()
 * memmove()
 *
 * Packet format:
 * AA 55 ID TEMP_L TEMP_H HUM_L HUM_H CRC8
 *
 * CRC-8: Polynomial 0x07, initial value 0x00
 */

#include <stdio.h>
#include <windows.h>
#include <string.h>

#define PIPE_NAME "\\\\.\\pipe\\SensorGateway"

#define PACKET_SIZE 8
#define BUFFER_SIZE 128

#define HEADER_1 0xAA
#define HEADER_2 0x55

// Calculate CRC-8 for the first seven packet bytes
unsigned char calculate_crc8(const unsigned char *data,
                             unsigned int length)
{
    unsigned char crc = 0;
    unsigned int i;
    unsigned int bit;

    for (i = 0; i < length; i++)
    {
        crc ^= data[i];

        for (bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
            {
                crc = (unsigned char)((crc << 1) ^ 0x07);
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

// Remove processed bytes from the beginning of the buffer
void remove_bytes(unsigned char buffer[],
                  unsigned int *count,
                  unsigned int bytes)
{
    memmove(buffer, buffer + bytes, *count - bytes);
    *count -= bytes;
}

// Search for complete packets and validate them
void parse_packets(unsigned char buffer[],
                   unsigned int *count)
{
    unsigned char received_crc;
    unsigned char calculated_crc;

    unsigned int temperature_raw;
    unsigned int humidity_raw;

    unsigned char sensor_id;

    while (*count >= 2)
    {
        // Search for packet header
        if (buffer[0] != HEADER_1 || buffer[1] != HEADER_2)
        {
            remove_bytes(buffer, count, 1);
            continue;
        }

        // Wait until all eight bytes are available
        if (*count < PACKET_SIZE)
        {
            break;
        }

        // Calculate and verify CRC
        received_crc = buffer[7];

        calculated_crc = calculate_crc8(buffer, 7);

        if (received_crc != calculated_crc)
        {
            printf("[ERROR] CRC validation failed.\n");

            // Discard one byte and search for the next header
            remove_bytes(buffer, count, 1);
            continue;
        }

        // Decode sensor information
        sensor_id = buffer[2];

        temperature_raw =
            (unsigned int)buffer[3] |
            ((unsigned int)buffer[4] << 8);

        humidity_raw =
            (unsigned int)buffer[5] |
            ((unsigned int)buffer[6] << 8);

        printf("[RX] ID=%u T=%.2f H=%.2f CRC_OK\n",
               sensor_id,
               temperature_raw / 100.0,
               humidity_raw / 100.0);

        // Remove the successfully processed packet
        remove_bytes(buffer, count, PACKET_SIZE);
    }
}

int main(void)
{
    HANDLE pipe;

    unsigned char received_data[32];
    unsigned char packet_buffer[BUFFER_SIZE];

    DWORD bytes_read;
    BOOL result;

    unsigned int packet_buffer_count = 0;

    printf("Windows Sensor Gateway - Receiver\n");
    printf("---------------------------------\n");

    // Create the named pipe
    pipe = CreateNamedPipeA(
        PIPE_NAME,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        0,
        0,
        0,
        NULL
    );

    if (pipe == INVALID_HANDLE_VALUE)
    {
        printf("[ERROR] CreateNamedPipe failed: %lu\n",
               GetLastError());
        return 1;
    }

    printf("[INFO] Named pipe created.\n");
    printf("[INFO] Waiting for sender connection...\n");

    // Wait for the sender to connect
    result = ConnectNamedPipe(pipe, NULL);

    if (!result && GetLastError() != ERROR_PIPE_CONNECTED)
    {
        printf("[ERROR] ConnectNamedPipe failed: %lu\n",
               GetLastError());

        CloseHandle(pipe);
        return 1;
    }

    printf("[INFO] Sender connected successfully.\n");

    // Continuously receive data
    while (1)
    {
        bytes_read = 0;

        result = ReadFile(
            pipe,
            received_data,
            sizeof(received_data),
            &bytes_read,
            NULL
        );

        if (!result)
        {
            DWORD error = GetLastError();

            if (error == ERROR_BROKEN_PIPE ||
                error == ERROR_NO_DATA)
            {
                printf("[INFO] Sender disconnected.\n");
            }
            else
            {
                printf("[ERROR] ReadFile failed: %lu\n", error);
            }

            break;
        }

        if (bytes_read == 0)
        {
            printf("[INFO] No more data received.\n");
            break;
        }

        // Append received bytes to the packet buffer
        if (packet_buffer_count + bytes_read > BUFFER_SIZE)
        {
            printf("[ERROR] Packet buffer overflow.\n");
            packet_buffer_count = 0;
        }

        memcpy(
            packet_buffer + packet_buffer_count,
            received_data,
            bytes_read
        );

        packet_buffer_count += bytes_read;

        // Parse all available complete packets
        parse_packets(packet_buffer, &packet_buffer_count);
    }

    // Close the connection
    DisconnectNamedPipe(pipe);
    CloseHandle(pipe);

    printf("[INFO] Receiver closed.\n");

    return 0;
}
