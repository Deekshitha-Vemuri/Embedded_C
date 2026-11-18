
/*
 *
 * Build command (MSVC Developer Command Prompt):
 * cl /nologo /W4 /TC /std:c11 main.c scheduler.c /Fe:task33.exe
 */

#include <stdio.h>
#include "sheduler.h"

/*
 * Sensor task callback.
 * Executes every 100 milliseconds.
 */
void read_sensor(void)
{
    printf("[SENSOR] Reading temperature sensor...\n");
}

/*
 * Log task callback.
 * Executes every 1000 milliseconds.
 */
void print_log(void)
{
    printf("[LOG] System status: Running normally.\n");
}

int main(void)
{
    // Create the sensor and log tasks
    ScheduledTask tasks[2] =
    {
        {
            read_sensor,
            100,       // Period: 100 ms
            0,         // Next deadline
            0,         // Execution count
            0,         // Missed deadlines
            0          // Maximum lateness
        },

        {
            print_log,
            1000,      // Period: 1000 ms
            0,
            0,
            0,
            0
        }
    };

    unsigned int task_count = 2;
    unsigned int i;

    printf("Cooperative Periodic Scheduler\n");
    printf("--------------------------------\n");

    // Initialize each scheduled task
    for (i = 0; i < task_count; i++)
    {
        scheduler_init(&tasks[i]);
    }

    // Run the scheduler for 5 seconds
    scheduler_run(tasks, task_count, 5000);

    // Display final task statistics
    printf("\nScheduler Summary\n");
    printf("--------------------------------\n");

    printf("Sensor executions : %u\n",
           tasks[0].execution_count);

    printf("Sensor missed     : %u\n",
           tasks[0].missed_deadlines);

    printf("Sensor max lateness: %llu ms\n",
           tasks[0].max_lateness);

    printf("\nLog executions    : %u\n",
           tasks[1].execution_count);

    printf("Log missed        : %u\n",
           tasks[1].missed_deadlines);

    printf("Log max lateness  : %llu ms\n",
           tasks[1].max_lateness);

    printf("--------------------------------\n");

    return 0;
}
