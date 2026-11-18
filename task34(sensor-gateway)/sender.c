
/*
Build:
cl /nologo /W4 /TC /std:c11 sender.c /Fe:sender.exe

APIs used:
CreateFileA(), WriteFile(), CloseHandle(), Sleep()
*/

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define PIPE_NAME "\\\\.\\pipe\\SensorGateway"
#define PACKET_SIZE 8
#define TOTAL_PACKETS 10

/* CRC-8 calculation: polynomial 0x07, initial value 0x00 */
uint8_t calculate_crc8(const uint8_t data[], int length)
{
    uint8_t crc = 0x00;

    for (int i = 0; i < length; i++)
    {
        crc ^= data[i];

        for (int bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
                crc = (uint8_t)((crc << 1) ^ 0x07);
            else
                crc = (uint8_t)(crc << 1);
        }
    }

    return crc;
}

/* Create an 8-byte sensor packet */
void create_packet(uint8_t packet[], uint8_t sensor_id,
                   float temperature, float humidity)
{
    uint16_t temp_value = (uint16_t)(temperature * 100);
    uint16_t humidity_value = (uint16_t)(humidity * 100);

    packet[0] = 0xAA;
    packet[1] = 0x55;
    packet[2] = sensor_id;

    packet[3] = (uint8_t)(temp_value & 0xFF);
    packet[4] = (uint8_t)(temp_value >> 8);

    packet[5] = (uint8_t)(humidity_value & 0xFF);
    packet[6] = (uint8_t)(humidity_value >> 8);

    packet[7] = calculate_crc8(packet, 7);
}

int main(void)
{
    unsigned int sensor_id;
    unsigned int interval_ms;

    printf("Enter Sensor ID (0-255): ");
    if (scanf_s("%u", &sensor_id) != 1 || sensor_id > 255)
    {
        printf("[ERROR] Invalid Sensor ID.\n");
        return 1;
    }

    printf("Enter sampling interval in milliseconds: ");
    if (scanf_s("%u", &interval_ms) != 1 || interval_ms == 0)
    {
        printf("[ERROR] Invalid sampling interval.\n");
        return 1;
    }

    printf("\n[TX] Connecting to named pipe...\n");

    HANDLE pipe = CreateFileA(
        PIPE_NAME,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (pipe == INVALID_HANDLE_VALUE)
    {
        printf("[ERROR] Cannot connect to receiver. Error: %lu\n",
               GetLastError());
        return 1;
    }

    printf("[TX] Connected successfully.\n\n");

    float temperature = 25.0f;
    float humidity = 50.0f;

    for (int i = 1; i <= TOTAL_PACKETS; i++)
    {
        uint8_t packet[PACKET_SIZE];

        create_packet(packet, (uint8_t)sensor_id,
                      temperature, humidity);

        /* Corrupt CRC intentionally on every fourth packet */
        if (i % 4 == 0)
        {
            packet[7] ^= 0x01;
            printf("[TX] Sending corrupted packet %d\n", i);
        }
        else
        {
            printf("[TX] Sending valid packet %d\n", i);
        }

        DWORD bytes_written = 0;

        BOOL success = WriteFile(
            pipe,
            packet,
            PACKET_SIZE,
            &bytes_written,
            NULL
        );

        if (!success || bytes_written != PACKET_SIZE)
        {
            printf("[ERROR] WriteFile failed. Error: %lu\n",
                   GetLastError());
            break;
        }

        printf("[TX] Packet sent (%lu bytes).\n", bytes_written);

        temperature += 0.5f;
        humidity += 0.5f;

        Sleep(interval_ms);
    }

    CloseHandle(pipe);

    printf("\n[TX] Sender finished.\n");

    return 0;
}
