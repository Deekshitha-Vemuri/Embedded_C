
#include "bme680.h"
#include "uart.h"

#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Default sensor sampling interval in milliseconds
#define DEFAULT_SAMPLE_INTERVAL_MS 2000

// Minimum and maximum allowed sampling intervals
#define MIN_SAMPLE_INTERVAL_MS 500
#define MAX_SAMPLE_INTERVAL_MS 60000

// Maximum number of pending commands in the queue
#define COMMAND_QUEUE_LENGTH 8

static const char *TAG = "TASK37";

// --------------------------------------------------
// Command definitions
// --------------------------------------------------

typedef enum
{
    COMMAND_SET_INTERVAL,
    COMMAND_START,
    COMMAND_STOP
} command_type_t;

// Structure used to pass commands between FreeRTOS tasks.
typedef struct
{
    command_type_t type;
    uint32_t interval_ms;
} command_t;

// Queue handle shared by the command and sensor tasks.
static QueueHandle_t command_queue = NULL;

// --------------------------------------------------
// Remove leading and trailing spaces from a command.
// --------------------------------------------------

static char *trim_whitespace(char *text)
{
    while (isspace((unsigned char)*text))
    {
        text++;
    }

    char *end = text + strlen(text);

    while (end > text && isspace((unsigned char)*(end - 1)))
    {
        end--;
    }

    *end = '\0';

    return text;
}

// --------------------------------------------------
// Parse and process commands received from the PC.
// --------------------------------------------------

static void process_command(char *text)
{
    char *command_text = trim_whitespace(text);

    // Ignore empty commands.
    if (command_text[0] == '\0')
    {
        return;
    }

    command_t command;

    // ----------------------------------------------
    // SET_INTERVAL command
    // Example: SET_INTERVAL 2000
    // ----------------------------------------------

    if (strncmp(command_text, "SET_INTERVAL", 12) == 0 &&
        isspace((unsigned char)command_text[12]))
    {
        unsigned int interval = 0;
        char extra;

        // Make sure the command contains one valid integer.
        if (sscanf(command_text + 12, "%u %c", &interval, &extra) != 1)
        {
            uart_send_line("ERROR;INVALID_INTERVAL_FORMAT");
            return;
        }

        if (interval < MIN_SAMPLE_INTERVAL_MS ||
            interval > MAX_SAMPLE_INTERVAL_MS)
        {
            uart_send_line("ERROR;INTERVAL_OUT_OF_RANGE");
            return;
        }

        command.type = COMMAND_SET_INTERVAL;
        command.interval_ms = interval;

        if (xQueueSend(command_queue, &command, 0) == pdPASS)
        {
            char response[64];

            snprintf(response, sizeof(response),
                     "ACK;SET_INTERVAL;%u", interval);

            uart_send_line(response);
        }
        else
        {
            uart_send_line("ERROR;COMMAND_QUEUE_FULL");
        }

        return;
    }

    // ----------------------------------------------
    // START command
    // ----------------------------------------------

    if (strcmp(command_text, "START") == 0)
    {
        command.type = COMMAND_START;
        command.interval_ms = 0;

        if (xQueueSend(command_queue, &command, 0) == pdPASS)
        {
            uart_send_line("ACK;START;ACCEPTED");
        }
        else
        {
            uart_send_line("ERROR;COMMAND_QUEUE_FULL");
        }

        return;
    }

    // ----------------------------------------------
    // STOP command
    // ----------------------------------------------

    if (strcmp(command_text, "STOP") == 0)
    {
        command.type = COMMAND_STOP;
        command.interval_ms = 0;

        if (xQueueSend(command_queue, &command, 0) == pdPASS)
        {
            uart_send_line("ACK;STOP;ACCEPTED");
        }
        else
        {
            uart_send_line("ERROR;COMMAND_QUEUE_FULL");
        }

        return;
    }

    uart_send_line("ERROR;UNKNOWN_COMMAND");
}

// --------------------------------------------------
// UART command handling task
// --------------------------------------------------

static void uart_command_task(void *pvParameters)
{
    char input_buffer[128];

    while (1)
    {
        // Wait for a complete command line from the Windows PC.
        if (uart_read_line(input_buffer, sizeof(input_buffer)))
        {
            ESP_LOGI(TAG, "Received command line: %s", input_buffer);

            // Split commands separated by semicolons.
            char *save_pointer = NULL;

            char *token = strtok_r(
                input_buffer,
                ";",
                &save_pointer
            );

            while (token != NULL)
            {
                process_command(token);

                token = strtok_r(
                    NULL,
                    ";",
                    &save_pointer
                );
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// --------------------------------------------------
// Sensor sampling task
// --------------------------------------------------

static void sensor_task(void *pvParameters)
{
    // Initial sampling interval.
    uint32_t sample_interval_ms = DEFAULT_SAMPLE_INTERVAL_MS;

    // Sensor sampling remains disabled until START is received.
    bool sampling_enabled = false;

    command_t command;

    ESP_LOGI(TAG, "Sensor task ready. Waiting for START command.");

    while (1)
    {
        // Wait for commands until the next sampling deadline.
        TickType_t wait_time = sampling_enabled
            ? pdMS_TO_TICKS(sample_interval_ms)
            : portMAX_DELAY;

        if (xQueueReceive(command_queue, &command, wait_time) == pdPASS)
        {
            // ------------------------------------------
            // Process SET_INTERVAL
            // ------------------------------------------

            if (command.type == COMMAND_SET_INTERVAL)
            {
                sample_interval_ms = command.interval_ms;

                ESP_LOGI(TAG, "Sampling interval updated: %" PRIu32 " ms",
                         sample_interval_ms);
            }

            // ------------------------------------------
            // Process START
            // ------------------------------------------

            else if (command.type == COMMAND_START)
            {
                if (!sampling_enabled)
                {
                    sampling_enabled = true;

                    ESP_LOGI(TAG, "Sensor sampling started");

                    char response[64];

                    snprintf(response, sizeof(response),
                             "ACK;START;PACKET_EVERY;%" PRIu32,
                             sample_interval_ms);

                    uart_send_line(response);
                }
            }

            // ------------------------------------------
            // Process STOP
            // ------------------------------------------

            else if (command.type == COMMAND_STOP)
            {
                sampling_enabled = false;

                ESP_LOGI(TAG, "Sensor sampling stopped");

                uart_send_line("ACK;STOP;SAMPLING_DISABLED");
            }
        }
        else
        {
            // No command arrived during the configured
            // interval. Time to take the next measurement.

            if (sampling_enabled)
            {
                ESP_LOGI(TAG, "Starting new sensor sampling cycle");

                esp_err_t result = bme680_start_forced_measurement();

                if (result != ESP_OK)
                {
                    ESP_LOGE(TAG, "Failed to start sensor measurement");
                    continue;
                }

                // Wait until the sensor completes its measurement.
                result = bme680_wait_for_measurement();

                if (result != ESP_OK)
                {
                    ESP_LOGE(TAG, "Measurement did not complete");
                    continue;
                }

                // ------------------------------------------
                // Read temperature
                // ------------------------------------------

                float temperature = 0.0f;

                result = bme680_read_temperature(&temperature);

                if (result != ESP_OK)
                {
                    ESP_LOGE(TAG, "Temperature reading failed");
                    continue;
                }

                // ------------------------------------------
                // Read pressure
                // ------------------------------------------

                float pressure = 0.0f;

                result = bme680_read_pressure(&pressure);

                if (result != ESP_OK)
                {
                    ESP_LOGE(TAG, "Pressure reading failed");
                    continue;
                }

                // ------------------------------------------
                // Read humidity
                // ------------------------------------------

                float humidity = 0.0f;

                result = bme680_read_humidity(&humidity);

                if (result != ESP_OK)
                {
                    ESP_LOGE(TAG, "Humidity reading failed");
                    continue;
                }

                // ------------------------------------------
                // Send one complete sensor data message
                // ------------------------------------------

                char data_message[128];

                snprintf(data_message, sizeof(data_message),
                         "DATA;%.2f;%.2f;%.2f",
                         temperature,
                         pressure,
                         humidity);

                uart_send_line(data_message);

                ESP_LOGI(TAG, "Sensor data transmitted");
            }
        }
    }
}

// --------------------------------------------------
// Main application entry point
// --------------------------------------------------

void app_main(void)
{
    ESP_LOGI(TAG, "Task 37 started");

    // --------------------------------------------------
    // Initialize I2C and register BME680
    // --------------------------------------------------

    esp_err_t result = bme680_init();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "BME680 initialization failed");
        return;
    }

    // --------------------------------------------------
    // Verify BME680 chip ID
    // --------------------------------------------------

    result = bme680_detect();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "BME680 detection failed");
        return;
    }

    ESP_LOGI(TAG, "BME680 initialization and detection completed");

    // --------------------------------------------------
    // Read calibration coefficients
    // Calibration data is required for sensor compensation.
    // It only needs to be read once during initialization.
    // --------------------------------------------------

    result = bme680_read_calibration_data();

    if (result != ESP_OK)
    {
        ESP_LOGE(TAG, "Calibration reading failed");
        return;
    }

    ESP_LOGI(TAG, "Calibration data read successful");

    // --------------------------------------------------
    // Create command queue
    // --------------------------------------------------

    command_queue = xQueueCreate(
        COMMAND_QUEUE_LENGTH,
        sizeof(command_t)
    );

    if (command_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create command queue");
        return;
    }

    // --------------------------------------------------
    // Create UART command handling task
    // --------------------------------------------------

    BaseType_t task_result = xTaskCreate(
        uart_command_task,
        "uart_command_task",
        4096,
        NULL,
        5,
        NULL
    );

    if (task_result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create UART command task");
        vQueueDelete(command_queue);
        return;
    }

    // --------------------------------------------------
    // Create sensor sampling task
    // --------------------------------------------------

    task_result = xTaskCreate(
        sensor_task,
        "sensor_task",
        4096,
        NULL,
        5,
        NULL
    );

    if (task_result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create sensor task");
        return;
    }

    ESP_LOGI(TAG, "UART command task created");
    ESP_LOGI(TAG, "Sensor task created");
    ESP_LOGI(TAG, "Waiting for commands from Windows PC");
}
