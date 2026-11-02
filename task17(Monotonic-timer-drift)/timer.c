#include <windows.h>
#include <stdio.h>
#include <float.h>

int main(void)
{
    LARGE_INTEGER frequency, start, now;

    double minJitter = DBL_MAX;
    double maxJitter = -DBL_MAX;

    const int iterations = 100;

    // Step 1: Get timer frequency (number of ticks per second)
    if (!QueryPerformanceFrequency(&frequency)) {
        printf("High-resolution timer is unavailable.\n");
        return 1;
    }

    // Get starting counter value=> no of ticks at current .
    QueryPerformanceCounter(&start);

    printf("Target interval: 100 ms\n");
    printf("Iterations: %d\n\n", iterations);

    // Step 2: Run 100 iterations
    for (int i = 1; i <= iterations; i++) {

        // Calculate the absolute deadline
        /* Deadline calculation.
         frequency value is always  per second that means 1000milliseconds.
          interval is 100ms , => 0.1second 
          assume freq value is 10000000 
          1sec => 10000000
          0.1=> x?
          so x=10000000*0.1*(current iteration)+ starting ticks value gives the ticks at deadline.
        */
        LONGLONG deadline =
            start.QuadPart +
            (LONGLONG)(frequency.QuadPart * 0.1 * i);

        // Step 3: Wait until the deadline
        while (1) {
            QueryPerformanceCounter(&now);

            LONGLONG remaining =
                deadline - now.QuadPart;

            if (remaining <= 0)
                break;
            //calculating jitter value
            double remainingMs =
                (double)remaining * 1000.0 /
                (double)frequency.QuadPart;

            if (remainingMs > 2.0)
                Sleep((DWORD)(remainingMs - 1.0));
            else
                Sleep(1);
        }

        // Step 4: Measure actual timing
        QueryPerformanceCounter(&now);

        // Step 5: Calculate jitter in milliseconds
        /* now-start gives you difference ticks 
           diff ticks should be shown in milliseconds
           lets assume diff=40000 ticks and freq=20000000
           1000 ms=> 20000000 ticks
           x? ms=> 40000 ticks
           x*freq=diff*1000
           x=diff*1000 / freq
        */
        double jitterMs =
            (double)(now.QuadPart - deadline) *
            1000.0 / (double)frequency.QuadPart;

        // Step 6: Track minimum and maximum jitter
        if (jitterMs < minJitter)
            minJitter = jitterMs;

        if (jitterMs > maxJitter)
            maxJitter = jitterMs;

        printf("Iteration %3d | Jitter: %8.3f ms\n",
               i, jitterMs);
    }

    printf("\nMinimum jitter: %.3f ms\n", minJitter);
    printf("Maximum jitter: %.3f ms\n", maxJitter);

    return 0;
}