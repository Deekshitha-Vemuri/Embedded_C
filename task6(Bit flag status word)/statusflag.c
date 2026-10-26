
#include <stdio.h>
#include <stdint.h>
#define WIFI           (1u << 0) 
#define BATTERY_LOW    (1u << 1)  
#define SENSOR_FAULT   (1u << 2)  
#define SENSOR_ACTIVE  (1u << 3)  
#define GPS_CONNECTED  (1u << 4)  
#define STORAGE_FULL   (1u << 5)  
#define OVERHEAT       (1u << 6)  
#define MAINTENANCE    (1u << 7)  
int main(void) {
    uint8_t status = 0;
    status |= SENSOR_FAULT;
    status |= WIFI;    
    printf("After SET: 0x%02X\n", (unsigned int)status);  
    status &= (uint8_t)~SENSOR_FAULT;
    printf("After CLEAR FAULT: 0x%02X\n",(unsigned int)status);
    status ^= WIFI;
    printf("After TOGGLE WIFI: 0x%02X\n", (unsigned int)status);
    printf("\nFinal status: 0x%02X\n",
           (unsigned int)status);
    printf("FAULT=%u\n",
           (unsigned int)((status & SENSOR_FAULT) != 0));

    printf("WIFI=%u\n",
           (unsigned int)((status & WIFI) != 0));

    printf("BATTERY_LOW=%u\n",
           (unsigned int)((status & BATTERY_LOW) != 0));

    printf("SENSOR_ACTIVE=%u\n",
           (unsigned int)((status & SENSOR_ACTIVE) != 0));

    printf("GPS_CONNECTED=%u\n",
           (unsigned int)((status & GPS_CONNECTED) != 0));

    printf("STORAGE_FULL=%u\n",
           (unsigned int)((status & STORAGE_FULL) != 0));

    printf("OVERHEAT=%u\n",
           (unsigned int)((status & OVERHEAT) != 0));

    printf("MAINTENANCE=%u\n",
           (unsigned int)((status & MAINTENANCE) != 0));

    return 0;
}
