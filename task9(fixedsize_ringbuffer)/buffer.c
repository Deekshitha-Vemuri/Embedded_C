#include <stdio.h>
#define MAX 8

int buffer[MAX];
int head = 0;
int tail = -1;
int count = 0;

void display() {
    if (count == 0) {
        printf("Buffer is empty!\n");
        return;
    }

    printf("Buffer values: ");
    for (int i = 0; i < count; i++) {
        printf("%d ", buffer[(head + i) % MAX]);
    }
    printf("\n");
}

void enqueue(int value) {
    if (count == MAX) {
        printf("Buffer is full value cant be inserted!\n");
        return;
    }
    tail = (tail + 1) % MAX;
    buffer[tail] = value;
    printf("%d inserted",value);
    count++;
}
int dequeue() {
    if (count == 0) {
        printf("Buffer is empty!\n");
        return -1;
    }
    int value = buffer[head];
    head = (head + 1) % MAX;
    count--;
    return value;
}

int main() {
    int choice, value;

    while (1) {
        printf("\n1. PUT\n");
        printf("2. GET (Display all values)\n");
        printf("3. REMOVE (Dequeue)\n");
        printf("4. EXIT\n");
        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid input!\n");
            break;
        }

        switch (choice) {
            case 1:
                printf("Enter value (0-255): ");
                if (scanf("%d", &value) != 1) {
                    printf("Invalid input!\n");
                    return 1;
                }

                if (value < 0 || value > 255) {
                    printf("Invalid byte value!\n");
                    break;
                }

                enqueue(value);
                break;

            case 2:
                display();
                break;

            case 3:
                value = dequeue();
                if (value != -1) {
                    printf("Removed value: %d\n", value);
                }
                break;

            case 4:
                printf("Exiting program.\n");
                return 0;

            default:
                printf("Invalid choice! Try again.\n");
        }
    }

    return 0;
}

   