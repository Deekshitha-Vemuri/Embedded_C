
/*
 * Task 32 - Nonblocking Device State Machine
 *
 * Build command (MSVC Developer Command Prompt):
 * cl /nologo /W4 /TC /std:c11 main.c state_machine.c /Fe:task32.exe
 *
 * Functions used:
 * printf()
 * state_transition()
 * get_state_name()
 * get_event_name()
 */

#include <stdio.h>
#include "state_machine.h"

// Simulated events for normal operation and repeated faults
static const DeviceEvent events[] =
{
    EVENT_INIT_OK,

    EVENT_READ_FAIL,
    EVENT_RETRY_OK,

    EVENT_READ_FAIL,
    EVENT_RETRY_OK
};

int main(void)
{
    DeviceState current_state = STATE_BOOT;
    DeviceState next_state;

    DeviceEvent event = EVENT_NONE;

    unsigned int event_index = 0;
    unsigned int iteration = 1;

    unsigned int total_events =
        sizeof(events) / sizeof(events[0]);

    printf("Device State Machine Started\n");
    printf("-----------------------------\n");

    // Main event-processing loop
    while (1)
    {
        event = EVENT_NONE;

        // BOOT and FAULT perform automatic state transitions
        if (current_state == STATE_BOOT ||
            current_state == STATE_FAULT)
        {
            event = EVENT_NONE;
        }
        else
        {
            // Process only one event per iteration
            if (event_index >= total_events)
            {
                break;
            }

            event = events[event_index++];
        }

        printf("\n[ITERATION %u]\n", iteration++);

        printf("[EVENT] %s\n", get_event_name(event));

        // Calculate the next state
        next_state = state_transition(current_state, event);

        // Log only when a state transition occurs
        if (next_state != current_state)
        {
            printf("[STATE] %s -> %s\n",
                   get_state_name(current_state),
                   get_state_name(next_state));

            // Update the current state
            current_state = next_state;

            // Perform actions separately from transition logic
            switch (current_state)
            {
                case STATE_INIT:
                    printf("[ACTION] Initializing device...\n");
                    break;

                case STATE_RUN:
                    printf("[ACTION] Device operating normally\n");
                    break;

                case STATE_FAULT:
                    printf("[ERROR] Device fault detected\n");
                    break;

                case STATE_RECOVER:
                    printf("[ACTION] Starting recovery...\n");
                    break;

                default:
                    break;
            }
        }
        else
        {
            printf("[WARNING] No valid state transition\n");
        }
    }

    printf("\n-----------------------------\n");
    printf("[INFO] Event processing completed\n");
    printf("[INFO] Final state: %s\n",
           get_state_name(current_state));

    return 0;
}
