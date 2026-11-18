
/*
 * Task 32 - Device State Machine
 *
 * Build command (MSVC Developer Command Prompt):
 * cl /nologo /W4 /TC /std:c11 main.c state_machine.c /Fe:task32.exe
 *
 * Functions used:
 * state_transition()
 * get_state_name()
 * get_event_name()
 */

#include "state_machine.h"

// Decide the next state based on the current state and event
DeviceState state_transition(DeviceState current_state,
                             DeviceEvent event)
{
    switch (current_state)
    {
        case STATE_BOOT:

            if (event == EVENT_NONE)
            {
                return STATE_INIT;
            }
            break;

        case STATE_INIT:

            if (event == EVENT_INIT_OK)
            {
                return STATE_RUN;
            }
            break;

        case STATE_RUN:

            if (event == EVENT_READ_FAIL)
            {
                return STATE_FAULT;
            }
            break;

        case STATE_FAULT:

            if (event == EVENT_NONE)
            {
                return STATE_RECOVER;
            }
            break;

        case STATE_RECOVER:

            if (event == EVENT_RETRY_OK)
            {
                return STATE_RUN;
            }
            break;

        default:
            break;
    }

    // Invalid event: retain the current state
    return current_state;
}

// Return a readable state name
const char *get_state_name(DeviceState state)
{
    switch (state)
    {
        case STATE_BOOT:    return "BOOT";
        case STATE_INIT:    return "INIT";
        case STATE_RUN:     return "RUN";
        case STATE_FAULT:   return "FAULT";
        case STATE_RECOVER: return "RECOVER";
        default:            return "UNKNOWN";
    }
}

// Return a readable event name
const char *get_event_name(DeviceEvent event)
{
    switch (event)
    {
        case EVENT_NONE:      return "NONE";
        case EVENT_INIT_OK:   return "INIT_OK";
        case EVENT_READ_FAIL: return "READ_FAIL";
        case EVENT_RETRY_OK:  return "RETRY_OK";
        default:              return "UNKNOWN";
    }
}
