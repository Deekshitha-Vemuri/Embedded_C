
#include <stdio.h>
#include <stdint.h>
typedef struct {
    uint8_t sensor_id;
    uint32_t interval_ms;
    int16_t threshold_c;
} SensorConfig;
int modifyInterval(SensorConfig *config, uint32_t new_interval_ms) {
    if (new_interval_ms < 100 || new_interval_ms > 60000) {
        return 0;
    }
    config->interval_ms = new_interval_ms;
    return 1;
}
int main() {
    SensorConfig sensor;
    unsigned int id;
    unsigned int interval;
    int threshold;
    printf("Enter Sensor ID (1-10): ");
    if (scanf("%u", &id) != 1 || id < 1 || id > 10) {
        printf("Invalid Sensor ID.\n");
        return 1;
    }
    printf("Enter sampling interval in ms (100-60000): ");
    if (scanf("%u", &interval) != 1 ||
        interval < 100 || interval > 60000) {
        printf("Invalid sampling interval.\n");
        return 1;
    }
    printf("Enter temperature threshold in Celsius (-40 to 125): ");
    if (scanf("%d", &threshold) != 1 ||
        threshold < -40 || threshold > 125) {
        printf("Invalid temperature threshold.\n");
        return 1;
    }
    sensor.sensor_id = (uint8_t)id;
    sensor.interval_ms = (uint32_t)interval;
    sensor.threshold_c = (int16_t)threshold;

    printf("\nOriginal Configuration:\n");
    printf("SensorID: %u\n", (unsigned int)sensor.sensor_id);
    printf("SamplingInterval: %u ms\n", (unsigned int)sensor.interval_ms);
    printf("Threshold: %d C\n", (int)sensor.threshold_c);

    unsigned int new_interval;

    printf("\nEnter new sampling interval in ms: ");
    if (scanf("%u", &new_interval) != 1) {
        printf("Invalid input.\n");
        return 1;
    }

    if (modifyInterval(&sensor, (uint32_t)new_interval)) {
        printf("\nUpdated Configuration:\n");
        printf("Sensor ID: %u\n", (unsigned int)sensor.sensor_id);
        printf("Sampling Interval: %u ms\n", (unsigned int)sensor.interval_ms);
        printf("Threshold: %d C\n", (int)sensor.threshold_c);
    } else {
        printf("Invalid interval. Must be between 100 and 60000 ms.\n");
        printf("Original configuration remains unchanged.\n");
    }

    return 0;
}
