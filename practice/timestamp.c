#include <stdio.h>
#include <time.h>

int main(void)
{
    time_t current_time = time(NULL);

    struct tm *local_time = localtime(&current_time);

    if (local_time == NULL)
    {
        printf("Failed to get local time.\n");
        return 1;
    }

    printf("Year: %d\n", local_time->tm_year + 1900);
    printf("Month: %d\n", local_time->tm_mon + 1);
    printf("Day: %d\n", local_time->tm_mday);

    printf("Hour: %d\n", local_time->tm_hour);
    printf("Minute: %d\n", local_time->tm_min);
    printf("Second: %d\n", local_time->tm_sec);

    char timestamp[30];

    strftime(timestamp, sizeof(timestamp),
             "%Y-%m-%d %H:%M:%S", local_time);

    printf("Timestamp: %s\n", timestamp);
    return 0;
}
