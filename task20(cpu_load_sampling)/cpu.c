
#include <stdio.h>
#include <stdint.h>
#include <windows.h>

// Convert FILETIME into one 64-bit value (high*2^32+low)
uint64_t fileTimeToUint64(FILETIME time)
{
    return ((uint64_t)time.dwHighDateTime << 32) |
           time.dwLowDateTime;
}

int main(void)
{
    FILETIME idle, kernel, user;
    uint64_t oldIdle, oldKernel, oldUser;
    uint64_t newIdle, newKernel, newUser;
    // Get the first CPU time reading
    if (!GetSystemTimes(&idle, &kernel, &user))
    {
        printf("Failed to get system CPU times.\n");
        return 1;
    }

    oldIdle = fileTimeToUint64(idle);
    oldKernel = fileTimeToUint64(kernel);
    oldUser = fileTimeToUint64(user);

    // Take 10 CPU usage samples
    for (int i = 1; i <= 10; i++)
    {
        // Wait for one second
        Sleep(1000);

        // Get the next CPU time reading
        if (!GetSystemTimes(&idle, &kernel, &user))
        {
            printf("Failed to get system CPU times.\n");
            return 1;
        }

        newIdle = fileTimeToUint64(idle);
        newKernel = fileTimeToUint64(kernel);
        newUser = fileTimeToUint64(user);
        /*idle -> free 
     kernel- >os operation+idle
     user->application working
     Total =change in kernel+change in user
     busy=Total-change in idle
     */

        // Find the CPU time changes during this interval
        uint64_t idleDiff = newIdle - oldIdle;
        uint64_t kernelDiff = newKernel - oldKernel;
        uint64_t userDiff = newUser - oldUser;

        // Kernel time already includes idle time
        uint64_t total = kernelDiff + userDiff;

        if (total > 0)
        {
            uint64_t busy = total - idleDiff;
            double cpuUsage = ((double)busy / total) * 100.0;

            printf("CPU Sample %d: %.2f%%\n", i, cpuUsage);
        }
        else
        {
            printf("CPU Sample %d: Unable to calculate\n", i);
        }

        // Save this reading for the next comparison
        oldIdle = newIdle;
        oldKernel = newKernel;
        oldUser = newUser;
    }

    return 0;
}
