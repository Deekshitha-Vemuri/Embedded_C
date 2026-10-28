/*
 * Packet layout:
 * Byte 0-1: Header (AA 55)
 * Byte 2:   Sensor ID (uint8_t, 0-255)
 * Byte 3-4: Temperature (int16_t, scaled by 100)
 * Byte 5-6: Humidity (uint16_t, scaled by 100)
 * Byte 7:   XOR checksum of bytes 0-6
 *
 * Multi-byte values use little-endian encoding.
 * Temperature range: -327.68 to 327.67 C
 * Humidity range: 0.00 to 100.00 %
 */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
int main(void)
{
    uint8_t packet[8];
    int sensorID;
    double temperature, humidity;

    printf("Enter Sensor ID (0-255): ");
    if (scanf("%d", &sensorID) != 1 ||
        sensorID < 0 || sensorID > 255)
    {
        printf("Invalid Sensor ID!\n");
        return 1;
    }

    printf("Enter Temperature (-327.68 to 327.67 C): ");
    if (scanf("%lf", &temperature) != 1 ||
        !isfinite(temperature) ||
        temperature < -327.68 || temperature > 327.67)
    {
        printf("Invalid Temperature!\n");
        return 1;
    }

    printf("Enter Humidity (0.00 to 100.00 %%): ");
    if (scanf("%lf", &humidity) != 1 ||
        !isfinite(humidity) ||
        humidity < 0.0 || humidity > 100.0)
    {
        printf("Invalid Humidity!\n");
        return 1;
    }
    uint8_t id = (uint8_t)sensorID;
    int16_t tempScaled = (int16_t)round(temperature * 100.0);
    uint16_t humidityScaled = (uint16_t)round(humidity * 100.0);
    uint16_t tempBits = (uint16_t)tempScaled;
    uint8_t checksum = 0;
    packet[0]=0xAA;
    packet[1]=0x55;
    packet[2]=id;
    packet[3] = tempBits & 0xFF;
    packet[4] = (tempBits >> 8) & 0xFF;
    packet[5] = humidityScaled & 0xFF;
    packet[6] = (humidityScaled >> 8) & 0xFF;
    for (int i = 0; i < 7; i++)
{
    checksum ^= packet[i];
}
    packet[7] = checksum;
    printf("\nPacket: ");
for (int i = 0; i < 8; i++)
{
    printf("%02X ", (unsigned int)packet[i]);
}
printf("\n");
    return 0;
}
