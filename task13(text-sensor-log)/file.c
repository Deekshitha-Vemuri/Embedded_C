
#include <stdio.h>
#include <time.h>
#include <windows.h>

int main(void)
{
    
    FILE *file = fopen("readings.csv", "a+");

    if (file == NULL)
    {
        printf("Error opening file.\n");
        return 1;
    }

    
    if (fseek(file, 0, SEEK_END) != 0)
    {
        printf("Error positioning file.\n");
        fclose(file);
        return 1;
    }

    long position = ftell(file);

    if (position == -1L)
    {
        printf("Error checking file position.\n");
        fclose(file);
        return 1;
    }

    
    if (position == 0)
    {
        if (fprintf(file, "timestamp,temp_c\n") < 0)
        {
            printf("Error writing header.\n");
            fclose(file);
            return 1;
        }
    }

   
    for (int i = 0; i < 10; i++)
    {
        
        time_t now = time(NULL);
        struct tm *local_time = localtime(&now);

        if (local_time == NULL)
        {
            printf("Error getting local time.\n");
            fclose(file);
            return 1;
        }

       
        char timestamp[30];

        if (strftime(timestamp, sizeof(timestamp),
                     "%Y-%m-%d %H:%M:%S", local_time) == 0)
        {
            printf("Error formatting timestamp.\n");
            fclose(file);
            return 1;
        }

       
        int temperature = 24 + (i % 2);

       
        if (fprintf(file, "%s,%d\n", timestamp, temperature) < 0)
        {
            printf("Error writing reading.\n");
            fclose(file);
            return 1;
        }

       
        if (fflush(file) != 0)
        {
            printf("Error flushing file.\n");
            fclose(file);
            return 1;
        }

        printf("Saved reading %d: %s, %d C\n",
               i + 1, timestamp, temperature);

        
        if (i < 9)
        {
            Sleep(1000);
        }
    }

  
    if (fclose(file) == EOF)
    {
        printf("Error closing file.\n");
        return 1;
    }

 
    file = fopen("readings.csv", "r");

    if (file == NULL)
    {
        printf("Error reopening file.\n");
        return 1;
    }

    printf("\n--- Contents of readings.csv ---\n");

    char line[100];

    while (fgets(line, sizeof(line), file) != NULL)
    {
        printf("%s", line);
    }

    if (ferror(file))
    {
        printf("\nError reading file.\n");
        fclose(file);
        return 1;
    }

    fclose(file);

    return 0;
}
