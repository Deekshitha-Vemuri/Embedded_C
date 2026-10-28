
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define MAX_LENGTH 100

uint8_t crc8(const uint8_t *data, size_t length)
{
    uint8_t crc = 0x00;

    for (size_t i = 0; i < length; i++)
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

int main(void)
{
    char input[MAX_LENGTH + 2];
    size_t length;

    printf("Enter a string (1-%d characters): ", MAX_LENGTH);

    if (fgets(input, sizeof(input), stdin) == NULL)
    {
        printf("Error reading input.\n");
        return 1;
    }

    length = strcspn(input, "\n");

    if (input[length] == '\n')
        input[length] = '\0';

    if (length == 0)
    {
        printf("Error: String cannot be empty.\n");
        return 1;
    }

    if (length > MAX_LENGTH)
    {
        printf("Error: String exceeds %d characters.\n", MAX_LENGTH);
        return 1;
    }

    uint8_t crc = crc8((uint8_t *)input, length);

    printf("Input string: %s\n", input);
    printf("String length: %zu\n", length);
    printf("CRC-8: 0x%02X\n", crc);

    return 0;
}
