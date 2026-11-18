
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <windows.h>

/*
 * Functions / APIs used:
 * GetTickCount64()
 */

/* Function pointer for a scheduled task callback */
typedef void (*TaskCallback)(void);

/* Stores information about one scheduled task */
typedef struct
{
    TaskCallback callback;

    ULONGLONG period;
    ULONGLONG next_deadline;

    unsigned int execution_count;
    unsigned int missed_deadlines;

    ULONGLONG max_lateness;

} ScheduledTask;

/* Scheduler function declarations */
void scheduler_init(ScheduledTask *task);

void scheduler_run(ScheduledTask tasks[],
                   unsigned int task_count,
                   ULONGLONG run_duration);

#endif
