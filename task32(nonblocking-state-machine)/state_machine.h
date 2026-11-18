
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

// Device states
typedef enum
{
    STATE_BOOT,
    STATE_INIT,
    STATE_RUN,
    STATE_FAULT,
    STATE_RECOVER

} DeviceState;

// Device events
typedef enum
{
    EVENT_NONE,
    EVENT_INIT_OK,
    EVENT_READ_FAIL,
    EVENT_RETRY_OK

} DeviceEvent;

// State transition function
DeviceState state_transition(DeviceState current_state,
                             DeviceEvent event);

// Get readable state and event names
const char *get_state_name(DeviceState state);
const char *get_event_name(DeviceEvent event);

#endif
