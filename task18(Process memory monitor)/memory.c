
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <psapi.h>

int main(void)
{
    // Get a handle to our current process
    HANDLE process = GetCurrentProcess();

    // Store the memory information here
    PROCESS_MEMORY_COUNTERS pmc;
    pmc.cb = sizeof(pmc);

    // Get the working set before allocating memory
    if (!GetProcessMemoryInfo(process, &pmc, sizeof(pmc)))
    {
        printf("Failed to get memory information.\n");
        return 1;
    }

    SIZE_T before = pmc.WorkingSetSize;

    // Allocate 20 MiB of memory
    size_t size = 20 * 1024 * 1024;
    char *buffer = malloc(size);

    // Check if memory allocation worked
    if (buffer == NULL)
    {
        printf("Memory allocation failed.\n");
        return 1;
    }

    // Touch one byte in every 4 KiB page
    for (size_t i = 0; i < size; i += 4096)
    {
        buffer[i] = 1;
    }

    // Get the working set after touching the pages
    if (!GetProcessMemoryInfo(process, &pmc, sizeof(pmc)))
    {
        printf("Failed to get memory information.\n");
        free(buffer);
        return 1;
    }

    SIZE_T after = pmc.WorkingSetSize;

    // Convert bytes to MiB
    double beforeMiB = (double)before / (1024.0 * 1024.0);
    double afterMiB = (double)after / (1024.0 * 1024.0);

    // Find the change in the working set
    double changeMiB = afterMiB - beforeMiB;

    // Display the results
    printf("Working set before: %.2f MiB\n", beforeMiB);
    printf("Working set after:  %.2f MiB\n", afterMiB);
    printf("Working set change: %+.2f MiB\n", changeMiB);

    // Release the allocated memory 
    free(buffer);

    return 0;
}
