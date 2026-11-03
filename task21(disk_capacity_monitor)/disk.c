
#include <stdio.h>
#include <windows.h>

int main(void)
{
    // Store the disk space information
    ULARGE_INTEGER freeBytes;
    ULARGE_INTEGER totalBytes;
    ULARGE_INTEGER totalFreeBytes;

    // Set the drive path and warning limit
    LPCSTR drive = "C:\\";
    double threshold = 10.0;  // 10 GiB

    // Get the disk space information
    if (!GetDiskFreeSpaceExA(
            drive,
            &freeBytes,
            &totalBytes,
            &totalFreeBytes))
    {
        printf("Failed to get disk information.\n");
        printf("Please check the drive path or permissions.\n");
        return 1;
    }

    // Convert bytes into GiB
    double freeGiB =
        (double)totalFreeBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);

    double totalGiB =
        (double)totalBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);

    // Calculate occupied space
    double usedGiB = totalGiB - freeGiB;

    printf("Drive           : %s\n", drive);
    printf("Total Capacity  : %.2f GiB\n", totalGiB);
    printf("Used Space      : %.2f GiB\n", usedGiB);
    printf("Free Space      : %.2f GiB\n", freeGiB);

    // Warn if free space is below the limit
    if (freeGiB < threshold)
    {
        printf("Status          : LOW STORAGE WARNING!\n");
    }
    else
    {
        printf("Status          : OK\n");
    }

    return 0;
}
