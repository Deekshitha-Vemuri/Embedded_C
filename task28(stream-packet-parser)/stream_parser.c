/*
 *
 * Virtual COM ports:
 * Sender   : COM6
 * Receiver : COM7
 *
 * Packet format:
 * AA 55 01 C4 09 88 13 A8
 * 
 * build: cl /nologo /W4 /TC /std:c11 stream_parser.c
 */

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define SENDER_PORT       "COM5"
#define RECEIVER_PORT     "COM5"

#define PACKET_SIZE       8
#define BUFFER_SIZE       128
#define READ_CHUNK_SIZE   64

#define HEADER_1          0xAA
#define HEADER_2          0x55

/* Ring buffer structure */

typedef struct
{
    uint8_t data[BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t count;

} RingBuffer;

/* Parser statistics */

typedef struct
{
    unsigned long packets_ok;
    unsigned long skipped_bytes;
    unsigned long checksum_errors;
    unsigned long buffer_overflows;

} ParserStats;


/* Function declarations */

void ring_buffer_init(RingBuffer *rb);

int ring_buffer_push(RingBuffer *rb, uint8_t value);

int ring_buffer_peek(
    const RingBuffer *rb,
    size_t offset,
    uint8_t *value
);

int ring_buffer_pop(RingBuffer *rb, uint8_t *value);

uint8_t calculate_checksum(
    const uint8_t packet[PACKET_SIZE]
);

void process_packets(
    RingBuffer *rb,
    ParserStats *stats
);

void print_parser_stats(const ParserStats *stats);

HANDLE open_serial_port(
    const char *port_name,
    int is_receiver
);


/* Display Windows error */

void print_win_error(const char *operation)
{
    DWORD error_code = GetLastError();

    char *message = NULL;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error_code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&message,
        0,
        NULL
    );

    fprintf(
        stderr,
        "[ERROR] %s failed. GetLastError=%lu",
        operation,
        (unsigned long)error_code
    );

    if (message != NULL)
    {
        fprintf(stderr, " - %s", message);
        LocalFree(message);
    }
    else
    {
        fprintf(stderr, "\n");
    }
}


/* Open and configure COM port */

HANDLE open_serial_port(
    const char *port_name,
    int is_receiver
)
{
    char device_name[32];

    HANDLE port;

    DCB dcb;

    COMMTIMEOUTS timeouts;

    snprintf(
        device_name,
        sizeof(device_name),
        "\\\\.\\%s",
        port_name
    );

    port = CreateFileA(
        device_name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (port == INVALID_HANDLE_VALUE)
    {
        print_win_error("CreateFileA");
        return INVALID_HANDLE_VALUE;
    }

    if (!SetupComm(port, 4096, 4096))
    {
        print_win_error("SetupComm");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    memset(&dcb, 0, sizeof(dcb));

    dcb.DCBlength = sizeof(dcb);

    if (!GetCommState(port, &dcb))
    {
        print_win_error("GetCommState");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;

    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = TRUE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(port, &dcb))
    {
        print_win_error("SetCommState");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    memset(&timeouts, 0, sizeof(timeouts));

    if (is_receiver)
    {
        timeouts.ReadIntervalTimeout = 25;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.ReadTotalTimeoutConstant = 150;
    }
    else
    {
        timeouts.WriteTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = 1000;
    }

    if (!SetCommTimeouts(port, &timeouts))
    {
        print_win_error("SetCommTimeouts");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    if (!PurgeComm(
        port,
        PURGE_RXCLEAR | PURGE_TXCLEAR
    ))
    {
        print_win_error("PurgeComm");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    printf(
        "[INFO] Opened %s at 115200 8N1.\n",
        device_name
    );

    return port;
}


/* Initialize ring buffer */

void ring_buffer_init(RingBuffer *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}


/* Insert byte into ring buffer */

int ring_buffer_push(RingBuffer *rb, uint8_t value)
{
    if (rb->count >= BUFFER_SIZE)
    {
        return 0;
    }

    rb->data[rb->head] = value;

    rb->head = (rb->head + 1) % BUFFER_SIZE;

    rb->count++;

    return 1;
}


/* Read byte without removing it */

int ring_buffer_peek(
    const RingBuffer *rb,
    size_t offset,
    uint8_t *value
)
{
    size_t index;

    if (offset >= rb->count)
    {
        return 0;
    }

    index = (rb->tail + offset) % BUFFER_SIZE;

    *value = rb->data[index];

    return 1;
}


/* Remove oldest byte from ring buffer */

int ring_buffer_pop(RingBuffer *rb, uint8_t *value)
{
    if (rb->count == 0)
    {
        return 0;
    }

    *value = rb->data[rb->tail];

    rb->tail = (rb->tail + 1) % BUFFER_SIZE;

    rb->count--;

    return 1;
}


/* Calculate XOR checksum */

uint8_t calculate_checksum(
    const uint8_t packet[PACKET_SIZE]
)
{
    uint8_t checksum = 0;

    size_t i;

    for (i = 0; i < PACKET_SIZE - 1; i++)
    {
        checksum ^= packet[i];
    }

    return checksum;
}


/* Display valid packet */

void display_packet(
    const uint8_t packet[PACKET_SIZE]
)
{
    uint16_t raw_temperature;

    uint16_t raw_humidity;

    double temperature;

    double humidity;

    raw_temperature =
        (uint16_t)(
            (uint16_t)packet[3] |
            ((uint16_t)packet[4] << 8)
        );

    raw_humidity =
        (uint16_t)(
            (uint16_t)packet[5] |
            ((uint16_t)packet[6] << 8)
        );

    temperature = raw_temperature / 100.0;

    humidity = raw_humidity / 100.0;

    printf(
        "[PACKET OK] ID=%u, Temperature=%.2f C, Humidity=%.2f %%\n",
        (unsigned int)packet[2],
        temperature,
        humidity
    );
}


/* Search header, extract packet and validate checksum */

void process_packets(
    RingBuffer *rb,
    ParserStats *stats
)
{
    uint8_t first;

    uint8_t second;

    uint8_t packet[PACKET_SIZE];

    uint8_t discarded;

    size_t i;

    for (;;)
    {
        if (rb->count < 2)
        {
            return;
        }

        ring_buffer_peek(rb, 0, &first);

        ring_buffer_peek(rb, 1, &second);

        /* Search for AA 55 */

        if (first != HEADER_1 || second != HEADER_2)
        {
            ring_buffer_pop(rb, &discarded);

            stats->skipped_bytes++;

            continue;
        }

        /* Wait until complete packet is available */

        if (rb->count < PACKET_SIZE)
        {
            return;
        }

        /* Copy candidate packet without removing bytes */

        for (i = 0; i < PACKET_SIZE; i++)
        {
            ring_buffer_peek(rb, i, &packet[i]);
        }

        /* Validate checksum */

        if (calculate_checksum(packet) != packet[7])
        {
            stats->checksum_errors++;

            fprintf(
                stderr,
                "[WARN] Checksum error: calculated=0x%02X received=0x%02X\n",
                calculate_checksum(packet),
                packet[7]
            );

            /*
             * Remove only one byte.
             * Continue searching for the next possible header.
             */

            ring_buffer_pop(rb, &discarded);

            stats->skipped_bytes++;

            continue;
        }

        /* Valid packet: remove all eight bytes */

        for (i = 0; i < PACKET_SIZE; i++)
        {
            ring_buffer_pop(rb, &discarded);
        }

        stats->packets_ok++;

        display_packet(packet);
    }
}


/* Print parser statistics */

void print_parser_stats(const ParserStats *stats)
{
    printf("\n========== Parser Summary ==========\n");

    printf("Valid packets    : %lu\n", stats->packets_ok);

    printf("Skipped bytes    : %lu\n", stats->skipped_bytes);

    printf("Checksum errors  : %lu\n", stats->checksum_errors);

    printf("Buffer overflows : %lu\n", stats->buffer_overflows);

    printf("====================================\n");
}


/* Read arbitrary chunks from COM port */

int read_and_parse(
    HANDLE receiver,
    RingBuffer *rb,
    ParserStats *stats
)
{
    uint8_t read_buffer[READ_CHUNK_SIZE];

    DWORD bytes_read = 0;

    DWORD i;

    if (!ReadFile(
        receiver,
        read_buffer,
        sizeof(read_buffer),
        &bytes_read,
        NULL
    ))
    {
        print_win_error("ReadFile");

        return 0;
    }

    if (bytes_read == 0)
    {
        printf("[INFO] Read timeout: no bytes available.\n");

        return 1;
    }

    printf(
        "[RX] ReadFile returned %lu byte(s):",
        (unsigned long)bytes_read
    );

    for (i = 0; i < bytes_read; i++)
    {
        printf(
            " %02X",
            (unsigned int)read_buffer[i]
        );
    }

    printf("\n");

    /* Accumulate received bytes */

    for (i = 0; i < bytes_read; i++)
    {
        if (!ring_buffer_push(rb, read_buffer[i]))
        {
            stats->buffer_overflows++;

            fprintf(
                stderr,
                "[ERROR] Ring buffer full. Dropping byte 0x%02X.\n",
                read_buffer[i]
            );

            process_packets(rb, stats);

            if (!ring_buffer_push(rb, read_buffer[i]))
            {
                fprintf(
                    stderr,
                    "[ERROR] Could not store incoming byte.\n"
                );

                continue;
            }
        }
    }

    /* Process all available packets */

    process_packets(rb, stats);

    return 1;
}




int main(void)
{
    /*
     * Test stream:
     *
     * 00 = Noise byte
     *
     * AA 55 = Header
     *
     * 01 = Sensor ID
     *
     * C4 09 = Temperature (25.00 C)
     *
     * 88 13 = Humidity (50.00 %)
     *
     * A8 = Checksum
     */

    static const uint8_t test_stream[] =
    {
        0x00,

        0xAA, 0x55, 0x01, 0xC4,
        0x09, 0x88, 0x13, 0xA8
    };

    /* Send data in different chunk sizes */

    static const size_t chunk_sizes[] =
    {
        2, 3, 4
    };

    HANDLE sender = INVALID_HANDLE_VALUE;

    HANDLE receiver = INVALID_HANDLE_VALUE;

    RingBuffer rb;

    ParserStats stats = { 0, 0, 0, 0 };

    size_t offset = 0;

    size_t total_size =
        sizeof(test_stream) / sizeof(test_stream[0]);

    size_t chunk_index;

    int success = 1;

    printf("Task 28 - Serial Stream Packet Parser\n");

    printf("-------------------------------------\n");

    printf(
        "Sender: %s | Receiver: %s\n\n",
        SENDER_PORT,
        RECEIVER_PORT
    );

    ring_buffer_init(&rb);

    /* Open sender COM port */

    sender = open_serial_port(SENDER_PORT, 0);

    if (sender == INVALID_HANDLE_VALUE)
    {
        fprintf(
            stderr,
            "[FATAL] Could not open sender port.\n"
        );

        success = 0;

        goto cleanup;
    }

    /* Open receiver COM port */

    receiver = open_serial_port(RECEIVER_PORT, 1);

    if (receiver == INVALID_HANDLE_VALUE)
    {
        fprintf(
            stderr,
            "[FATAL] Could not open receiver port.\n"
        );

        success = 0;

        goto cleanup;
    }

    printf(
        "\n[INFO] Sending sample stream in chunks: 2, 3, 4 bytes.\n"
    );

    /* Send and receive chunks */

    for (
        chunk_index = 0;
        chunk_index < sizeof(chunk_sizes) / sizeof(chunk_sizes[0]);
        chunk_index++
    )
    {
        size_t chunk_size = chunk_sizes[chunk_index];

        DWORD bytes_written = 0;

        if (offset + chunk_size > total_size)
        {
            chunk_size = total_size - offset;
        }

        printf(
            "[TX] Writing %lu byte(s) on %s.\n",
            (unsigned long)chunk_size,
            SENDER_PORT
        );

        if (!WriteFile(
            sender,
            test_stream + offset,
            (DWORD)chunk_size,
            &bytes_written,
            NULL
        ))
        {
            print_win_error("WriteFile");

            success = 0;

            goto cleanup;
        }

        if (bytes_written != (DWORD)chunk_size)
        {
            fprintf(
                stderr,
                "[ERROR] Partial write: requested=%lu written=%lu.\n",
                (unsigned long)chunk_size,
                (unsigned long)bytes_written
            );

            success = 0;

            goto cleanup;
        }

        printf(
            "[TX] Bytes written: %lu\n",
            (unsigned long)bytes_written
        );

        offset += chunk_size;

        Sleep(30);

        if (!read_and_parse(receiver, &rb, &stats))
        {
            success = 0;

            goto cleanup;
        }
    }

    /* Final read */

    if (!read_and_parse(receiver, &rb, &stats))
    {
        success = 0;

        goto cleanup;
    }

    if (rb.count > 0)
    {
        fprintf(
            stderr,
            "[WARN] Stream ended with %lu incomplete byte(s).\n",
            (unsigned long)rb.count
        );
    }

    print_parser_stats(&stats);

    /* Verify expected result */

    if (
        stats.packets_ok == 1 &&
        stats.skipped_bytes == 1
    )
    {
        printf(
            "\n[PASS] Task 28 sample result matched: "
            "packet OK ID=1; skipped=1.\n"
        );
    }
    else
    {
        fprintf(
            stderr,
            "\n[FAIL] Expected one valid packet and "
            "one skipped noise byte.\n"
        );

        success = 0;
    }

cleanup:

    if (receiver != INVALID_HANDLE_VALUE)
    {
        CloseHandle(receiver);

        printf("[INFO] Receiver handle closed.\n");
    }

    if (sender != INVALID_HANDLE_VALUE)
    {
        CloseHandle(sender);

        printf("[INFO] Sender handle closed.\n");
    }

    return success ? 0 : 1;
}