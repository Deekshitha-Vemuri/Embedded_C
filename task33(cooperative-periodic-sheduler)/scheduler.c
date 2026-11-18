
/*
 * Task 33 - Cooperative Periodic Scheduler
 *
 * Functions / APIs used:
 * GetTickCount64()
 * SwitchToThread()
 * printf()
 *
 * Implements:
 * 1. Scheduler initialization
 * 2. Periodic deadline checking
 * 3. Callback execution
 * 4. Missed-period tracking
 * 5. Maximum lateness measurement
 */

#include <stdio.h>
#include"sheduler.h"
/*
 * Initialize one scheduled task.
 */
void scheduler_init(ScheduledTask *task)
{
    if (task == NULL)
    {
        printf("[ERROR] Invalid task pointer.\n");
        return;
    }

    if (task->callback == NULL || task->period == 0)
    {
        printf("[ERROR] Invalid task configuration.\n");
        return;
    }

    // Set the first execution deadline
    task->next_deadline = GetTickCount64() + task->period;

    // Reset task statistics
    task->execution_count = 0;
    task->missed_deadlines = 0;
    task->max_lateness = 0;
}

/*
 * Run all scheduled tasks for the specified duration.
 */
void scheduler_run(
    ScheduledTask tasks[],
    unsigned int task_count,
    ULONGLONG run_duration)
{
    ULONGLONG start_time;
    ULONGLONG current_time;
    ULONGLONG lateness;
    ULONGLONG due_count;

    unsigned int i;

    if (tasks == NULL || task_count == 0 || run_duration == 0)
    {
        printf("[ERROR] Invalid scheduler configuration.\n");
        return;
    }

    start_time = GetTickCount64();

    printf("[INFO] Scheduler started.\n");

    // Keep checking tasks until the requested duration expires
    while (1)
    {
        current_time = GetTickCount64();

        

        // Check each scheduled task
        for (i = 0; i < task_count; i++)
        {
            ScheduledTask *task = &tasks[i];

            if (task->callback == NULL || task->period == 0)
            {
                continue;
            }

            // Check whether this task is due
            if (current_time >= task->next_deadline)
            {
                // Calculate how late this execution is
                lateness = current_time - task->next_deadline;

                // Update maximum lateness
                if (lateness > task->max_lateness)
                {
                    task->max_lateness = lateness;
                }

                // Count how many scheduled executions are due
                due_count = (lateness / task->period) + 1;

                /*
                 * Execute only once.
                 * Additional due periods are skipped.
                 */
                if (due_count > 1)
                {
                    task->missed_deadlines +=
                        (unsigned int)(due_count - 1);
                }

                // Execute the task callback
                task->callback();

                task->execution_count++;

                // Advance deadline beyond the current time
                task->next_deadline += due_count * task->period;

                // Account for time spent inside the callback
                while (task->next_deadline <= GetTickCount64())
                {
                    task->next_deadline += task->period;
                    task->missed_deadlines++;
                }
            }
        }
        // Stop after processing tasks due at the current time
if (current_time - start_time >= run_duration)
{
    break;
}

        // Yield the CPU to another ready thread
        SwitchToThread();
    }

    printf("[INFO] Scheduler stopped.\n");
}
