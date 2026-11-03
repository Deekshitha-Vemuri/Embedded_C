
#include <stdio.h>
#include <windows.h>

int main(void)
{
    // Store the power information here
    SYSTEM_POWER_STATUS status;

    while (1)
    {
        // Get the current power status
        if (!GetSystemPowerStatus(&status))
        {
            printf("Failed to get power status.\n");
            Sleep(5000);
            continue;
        }
        // Check whether the charger is connected
        if (status.ACLineStatus == 1)
        {
            printf("AC Power       : Online\n");
        }
        else if (status.ACLineStatus == 0)
        {
            printf("AC Power       : Offline\n");
        }
        else
        {
            printf("AC Power       : Unknown\n");
        }

        // Print the battery percentage only if Windows knows it
        if (status.BatteryLifePercent != 255)
        {
            printf("Battery Status : %u%%\n",
                   (unsigned int)status.BatteryLifePercent);
        }
        else
        {
            printf("Battery Status : Unknown\n");
        }
        // Wait for 5 seconds before checking again
        Sleep(5000);
    }

    return 0;
}
