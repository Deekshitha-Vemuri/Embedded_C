
 // Build using: cl /TC /std:c11 producer_consumer.c
#define _WIN32_WINNT 0x0600
#include <stdio.h>
#include <windows.h>
#define QUEUE_SIZE 10
#define TOTAL_READINGS 100
#define PRODUCER_DELAY 10
typedef struct
{
    int id;
    float temperature;
} SensorReading;
SensorReading queue[QUEUE_SIZE];
int front = 0;
int rear = 0;
int count = 0;
unsigned int producedCount = 0;
unsigned int processedCount = 0;
unsigned int droppedCount = 0;
int producerDone = 0;
CRITICAL_SECTION cs;
CONDITION_VARIABLE notEmpty;
DWORD WINAPI producerThread(LPVOID param)
{
    for (int i = 0; i < TOTAL_READINGS; i++)
    {
        // Generate a sensor reading
        SensorReading reading;
        reading.id = i + 1;
        reading.temperature = 20.0f + (float)(i % 150) / 10.0f;
        // Lock the queue before accessing shared data
        EnterCriticalSection(&cs);
        producedCount++;
        if (count < QUEUE_SIZE)
        {
            // Insert the reading into the queue
            queue[rear] = reading;
            rear = (rear + 1) % QUEUE_SIZE;
            count++;
            printf("Produced %d: Temperature = %.1f C\n", reading.id, reading.temperature);
            // Wake the consumer because data is available
            WakeConditionVariable(&notEmpty);
        }
        else
        {
            // Drop the reading if the queue is full
            droppedCount++;
            printf("Dropped %d: Queue is full\n", reading.id);
        }
        // Release the queue lock
        LeaveCriticalSection(&cs);
        // Wait 10 milliseconds before generating the next reading
        Sleep(PRODUCER_DELAY);
    }
    // Inform the consumer that production is complete
    EnterCriticalSection(&cs);
    producerDone = 1;
    WakeAllConditionVariable(&notEmpty);
    LeaveCriticalSection(&cs);
    return 0;
}
DWORD WINAPI consumerThread(LPVOID param)
{
    while (1)
    {
        SensorReading reading;
        // Lock the queue before accessing shared data
        EnterCriticalSection(&cs);
        // Wait while the queue is empty and producer is still running
        while (count == 0 && !producerDone)
        {
            SleepConditionVariableCS(&notEmpty, &cs, INFINITE);
        }
        // Exit when production is complete and the queue is empty
        if (count == 0 && producerDone)
        {
            LeaveCriticalSection(&cs);
            break;
        }
        // Retrieve one reading from the queue
        reading = queue[front];
        front = (front + 1) % QUEUE_SIZE;
        count--;
        // Release the lock before processing the reading
        LeaveCriticalSection(&cs);
        // Process the reading
        EnterCriticalSection(&cs);
        processedCount++;
        printf("Consumed %d: Temperature = %.1f C\n", reading.id, reading.temperature);
        LeaveCriticalSection(&cs);
    }
    return 0;
}
int main()
{
    HANDLE producer;
    HANDLE consumer;
    // Initialize synchronization objects
    InitializeCriticalSection(&cs);
    InitializeConditionVariable(&notEmpty);
    // Create the consumer thread
    consumer = CreateThread(NULL, 0, consumerThread, NULL, 0, NULL);
    if (consumer == NULL)
    {
        printf("Consumer thread creation failed\n");
        DeleteCriticalSection(&cs);
        return 1;
    }
    // Create the producer thread
    producer = CreateThread(NULL, 0, producerThread, NULL, 0, NULL);
    if (producer == NULL)
    {
        printf("Producer thread creation failed\n");
        EnterCriticalSection(&cs);
        producerDone = 1;
        WakeAllConditionVariable(&notEmpty);
        LeaveCriticalSection(&cs);
        WaitForSingleObject(consumer, INFINITE);
        CloseHandle(consumer);
        DeleteCriticalSection(&cs);
        return 1;
    }
    // Wait for the producer to finish
    WaitForSingleObject(producer, INFINITE);
    // Wait for the consumer to process remaining readings
    WaitForSingleObject(consumer, INFINITE);
    // Display final statistics
    printf("\nProducer-Consumer Results\n");
    printf("-------------------------\n");
    printf("Total produced  : %u\n", producedCount);
    printf("Total processed : %u\n", processedCount);
    printf("Total dropped   : %u\n", droppedCount);
    // Close thread handles
    CloseHandle(producer);
    CloseHandle(consumer);
    // Clean up the critical section
    DeleteCriticalSection(&cs);
    return 0;
}
