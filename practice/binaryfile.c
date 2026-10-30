#include <stdio.h>
int main(void)
{
    FILE *file = fopen("sensor_log.bin", "wb");
    if (file == NULL)
    {
        printf("Could not open file.\n");
        return 1;
    }
    printf("Binary file opened successfully.\n");

    fclose(file);
   

    return 0;
}
