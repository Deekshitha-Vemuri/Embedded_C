#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main(void)
{
    uint8_t packet[8];
    char input[100];
    printf("Enter packet as 8 bytes: ");
    if (fgets(input, sizeof(input), stdin) == NULL)
    {
        printf("Input error\n");
        return 1;
    }
        int count = 0;
    char *token = strtok(input, " \t\r\n");

    while (token != NULL)
    {
        // Check whether we already received 8 bytes
        if (count >= 8)
        {
            printf("Error: Too many bytes\n");
            return 1;
        }

        // Each byte must contain exactly 2 characters
        if (strlen(token) != 2)
        {
            printf("Error: Each byte must have 2 hex digits\n");
            return 1;
        }

        // Both characters must be hexadecimal digits
        if (!isxdigit((unsigned char)token[0]) ||
            !isxdigit((unsigned char)token[1]))
        {
            printf("Error: Invalid hexadecimal byte\n");
            return 1;
        }

        // Convert the validated hexadecimal text to a number
        packet[count] = (uint8_t)strtoul(token, NULL, 16);

        count++;
        token = strtok(NULL, " \t\r\n");
    }

    // Check whether exactly 8 bytes were received
    if (count != 8)
    {
        printf("Error: Expected 8 bytes\n");
        return 1;
    }
    if (packet[0] != 0xAA || packet[1] != 0x55)
{
    printf("Error: Invalid packet header\n");
    return 1;
}
uint8_t checksum = 0;

for (int i = 0; i < 7; i++)
{
    checksum ^= packet[i];
}

if (checksum != packet[7])
{
    printf("Error: Invalid checksum\n");
    return 1;
}

printf("Input validated successfully!\n");



uint16_t rawTemp =
    ((uint16_t)packet[4] << 8) | packet[3];

int16_t signedTemp = (int16_t)rawTemp;

float temperature = signedTemp / 100.0f;

uint16_t rawHumidity =
    ((uint16_t)packet[6] << 8) | packet[5];

float humidity = rawHumidity / 100.0f;
if (humidity > 100.0f)
{
    printf("Error: Decoded humidity value is out of range (0-100%%)\n");
    return 1;
}

printf("Sensor ID: %u\n", (unsigned int)packet[2]);
printf("Temperature: %.2f C\n", (double)temperature);
printf("Humidity: %.2f %%\n", (double)humidity);
}