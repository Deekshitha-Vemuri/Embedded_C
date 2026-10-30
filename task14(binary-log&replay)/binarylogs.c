
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define RECORD_SIZE 3
#define TOTAL_RECORDS 10

void encode_record(uint8_t record[RECORD_SIZE],
                   uint8_t id,
                   int16_t temperature)
{
    uint16_t raw_temp = (uint16_t)temperature;

    record[0] = id;
    record[1] = (uint8_t)(raw_temp & 0xFF);
    record[2] = (uint8_t)(raw_temp >> 8);
}

int16_t decode_temperature(const uint8_t record[RECORD_SIZE])
{
    uint16_t raw_temp =
        (uint16_t)record[1] |
        ((uint16_t)record[2] << 8);

    return (int16_t)raw_temp;
}

int main(void)
{
    FILE *fp;
    uint8_t record[RECORD_SIZE];

    uint8_t ids[TOTAL_RECORDS];
    double temperatures[TOTAL_RECORDS];
    long scaled[TOTAL_RECORDS];

    printf("Enter 10 Sensor IDs (1-255), separated by spaces:\n");

    for (int i = 0; i < TOTAL_RECORDS; i++)
    {
        unsigned int id;

        if (scanf("%u", &id) != 1 || id < 1 || id > UINT8_MAX)
        {
            printf("Invalid ID. IDs must be between 1 and 255.\n");
            return 1;
        }

        ids[i] = (uint8_t)id;
    }

    printf("\nEnter 10 temperatures in Celsius, separated by spaces:\n");

    for (int i = 0; i < TOTAL_RECORDS; i++)
    {
        if (scanf("%lf", &temperatures[i]) != 1)
        {
            printf("Invalid temperature input.\n");
            return 1;
        }

        scaled[i] = lround(temperatures[i] * 100.0);

        if (scaled[i] < INT16_MIN || scaled[i] > INT16_MAX)
        {
            printf("Temperature must be between -327.68 and 327.67 C.\n");
            return 1;
        }
    }

    fp = fopen("sensor_log.bin", "wb");

    if (fp == NULL)
    {
        printf("Unable to open file for writing.\n");
        return 1;
    }

    for (int i = 0; i < TOTAL_RECORDS; i++)
    {
        encode_record(record, ids[i], (int16_t)scaled[i]);

        if (fwrite(record, 1, RECORD_SIZE, fp) != RECORD_SIZE)
        {
            printf("Error writing record.\n");
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);

    printf("\nAll sensor records written successfully.\n");

    fp = fopen("sensor_log.bin", "rb");

    if (fp == NULL)
    {
        printf("Unable to open file for reading.\n");
        return 1;
    }

    printf("\nReplayed Sensor Records:\n");

    while (1)
    {
        size_t bytes_read = fread(record, 1, RECORD_SIZE, fp);

        if (bytes_read == 0)
        {
            if (ferror(fp)){
                printf("Error while reading file.\n");
                fclose(fp);
                return 1;
            }

            if (feof(fp))
              break;
        }

        if (bytes_read != RECORD_SIZE)
        {
            printf("Incomplete record found.\n");
            break;
        }

        int16_t temperature = decode_temperature(record);

        printf("ID=%u Temperature=%.2f C\n",
               (unsigned)record[0],
               temperature / 100.0);
    }

    fclose(fp);

    return 0;
}
