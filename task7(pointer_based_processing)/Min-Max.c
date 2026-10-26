
#include <stdio.h>
#include <stdbool.h>

bool findMinMax(const int *readings, int length, int *min, int *max) {
    if (readings == NULL || min == NULL || max == NULL) {
        printf("Invalid pointer provided.\n");
        return false;
    }

    if (length == 0) {
        printf("Empty array: no minimum or maximum.\n");
        return false;
    }
    *min = readings[0];
    *max = readings[0];
    for (int i = 1; i < length; i++) {
        if (readings[i] < *min) {
            *min = readings[i];
        }

        if (readings[i] > *max) {
            *max = readings[i];
        }
    }
    return true;
}
int main(void) {
    int n;

    printf("Enter array size: ");
    if (scanf("%d", &n) != 1 || n < 0 || n > 10000) {
        printf("Invalid array size.\n");
        return 1;
    }

    if (n == 0) {
        printf("Empty array: no minimum or maximum.\n");
        return 0;
    }

    int a[n];

    printf("Enter array values:\n");

    for (int i = 0; i < n; i++) {
        if (scanf("%d", &a[i]) != 1) {
            printf("Invalid input. Enter integers only.\n");
            return 1;
        }
    }
    int minimum, maximum;
    if (findMinMax(a, n, &minimum, &maximum)) {
        printf("Minimum reading: %d\n", minimum);
        printf("Maximum reading: %d\n", maximum);
    }
    return 0;
}
