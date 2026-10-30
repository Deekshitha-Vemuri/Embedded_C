
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define SETTINGS_FILE "settings.bin"
#define TEMP_FILE    "settings.tmp"
#define BACKUP_FILE  "settings.bak"

#define SETTINGS_VERSION 1
#define RECORD_SIZE 11

#define DEFAULT_INTERVAL 1000u
#define DEFAULT_THRESHOLD 3000

#define MIN_INTERVAL 100u
#define MAX_INTERVAL 60000u

#define MIN_THRESHOLD (-4000)
#define MAX_THRESHOLD 12500

typedef struct {
    uint32_t interval_ms;
    int16_t threshold_x100;
} Settings;

static void put_u32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static uint32_t crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFu;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];

        for (int bit = 0; bit < 8; bit++) {
            if (crc & 1u)
                crc = (crc >> 1) ^ 0xEDB88320u;
            else
                crc >>= 1;
        }
    }

    return crc ^ 0xFFFFFFFFu;
}

static int valid_settings(Settings s)
{
    return s.interval_ms >= MIN_INTERVAL
        && s.interval_ms <= MAX_INTERVAL
        && s.threshold_x100 >= MIN_THRESHOLD
        && s.threshold_x100 <= MAX_THRESHOLD;
}

static void encode(Settings s, uint8_t record[RECORD_SIZE])
{
    record[0] = SETTINGS_VERSION;

    put_u32(&record[1], s.interval_ms);

    uint16_t threshold = (uint16_t)s.threshold_x100;
    record[5] = (uint8_t)threshold;
    record[6] = (uint8_t)(threshold >> 8);

    uint32_t crc = crc32(record, 7);
    put_u32(&record[7], crc);
}


static int decode(const uint8_t record[RECORD_SIZE], Settings *s)
{
    if (record[0] != SETTINGS_VERSION)
        return 0;

    uint32_t stored_crc = get_u32(&record[7]);

    if (stored_crc != crc32(record, 7))
        return 0;

    s->interval_ms = get_u32(&record[1]);

    uint16_t raw = (uint16_t)record[5]
                 | ((uint16_t)record[6] << 8);

  
    int32_t signed_value = (raw <= INT16_MAX)
                         ? (int32_t)raw
                         : (int32_t)raw - 65536;

    s->threshold_x100 = (int16_t)signed_value;

    return valid_settings(*s);
}


static int load_settings(Settings *s)
{
    *s = (Settings){DEFAULT_INTERVAL, DEFAULT_THRESHOLD};

    FILE *f = fopen(SETTINGS_FILE, "rb");

    if (f == NULL) {
        puts("No settings file; defaults used.");
        return 0;
    }

    uint8_t record[RECORD_SIZE];

    int ok = fread(record, 1, RECORD_SIZE, f) == RECORD_SIZE;

    
    if (ok && fgetc(f) != EOF)
        ok = 0;

    if (ferror(f))
        ok = 0;

    if (fclose(f) != 0)
        ok = 0;

    Settings loaded;

    if (ok && decode(record, &loaded)) {
        *s = loaded;
        puts("Settings loaded successfully.");
        return 1;
    }

    puts("Invalid settings; defaults used.");
    return 0;
}


static int save_settings(Settings s)
{
    if (!valid_settings(s)) {
        puts("Refusing to save invalid settings.");
        return 0;
    }

    uint8_t record[RECORD_SIZE];
    encode(s, record);

    FILE *f = fopen(TEMP_FILE, "wb");

    if (f == NULL) {
        perror("Cannot open temporary file");
        return 0;
    }

    int ok = fwrite(record, 1, RECORD_SIZE, f) == RECORD_SIZE;

    if (fflush(f) != 0)
        ok = 0;

    if (fclose(f) != 0)
        ok = 0;

    if (!ok) {
        puts("Writing settings failed.");
        return 0;
    }

    
    uint8_t verify_record[RECORD_SIZE];
    Settings verified;

    f = fopen(TEMP_FILE, "rb");

    if (f == NULL)
        return 0;

    ok = fread(verify_record, 1, RECORD_SIZE, f) == RECORD_SIZE;

    if (ok && fgetc(f) != EOF)
        ok = 0;

    if (ferror(f))
        ok = 0;

    if (fclose(f) != 0)
        ok = 0;

    if (!ok || !decode(verify_record, &verified)) {
        puts("Temporary file verification failed.");
        return 0;
    }

    
    int had_old_file = (rename(SETTINGS_FILE, BACKUP_FILE) == 0);

    if (!had_old_file) {
        FILE *old = fopen(SETTINGS_FILE, "rb");

        if (old != NULL) {
            fclose(old);
            puts("Could not preserve the old settings file.");
            return 0;
        }
    }

    if (rename(TEMP_FILE, SETTINGS_FILE) != 0) {
        perror("Installing new settings failed");

        if (had_old_file &&
            rename(BACKUP_FILE, SETTINGS_FILE) != 0) {
            puts("Warning: backup restoration failed.");
        }

        return 0;
    }

    if (had_old_file)
        remove(BACKUP_FILE);

    puts("Settings saved successfully.");
    return 1;
}

static void show_settings(Settings s)
{
    int32_t t = s.threshold_x100;
    int32_t magnitude = (t < 0) ? -t : t;

    printf("Interval = %u ms\n", (unsigned)s.interval_ms);
    printf("Threshold = %s%ld.%02ld C\n",
           t < 0 ? "-" : "",
           (long)(magnitude / 100),
           (long)(magnitude % 100));
}


int main(void)
{
    Settings s;

   
    if (!load_settings(&s)) {
        if (!save_settings(s))
            return 1;
    }

    show_settings(s);

    char line[100];
    char command[32];
    long value;
    char extra;

    puts("\nCommands:");
    puts("SET_INTERVAL 5000");
    puts("SET_THRESHOLD 35");
    puts("SHOW");
    puts("EXIT");

    while (1) {
        printf("\n> ");

        if (fgets(line, sizeof line, stdin) == NULL)
            break;

        int fields = sscanf(line, "%31s %ld %c",
                            command, &value, &extra);

        if (fields == 0)
            continue;

        if (strcmp(command, "EXIT") == 0) {
            if (fields != 1) {
                puts("Usage: EXIT");
                continue;
            }

            puts("Exiting program.");
            break;
        }

        if (strcmp(command, "SHOW") == 0) {
            if (fields != 1) {
                puts("Usage: SHOW");
                continue;
            }

            show_settings(s);
            continue;
        }

        Settings updated = s;

        if (strcmp(command, "SET_INTERVAL") == 0) {
            if (fields != 2) {
                puts("Usage: SET_INTERVAL <milliseconds>");
                continue;
            }

            if (value < MIN_INTERVAL || value > MAX_INTERVAL) {
                printf("Interval must be between %u and %u ms.\n",
                       (unsigned)MIN_INTERVAL,
                       (unsigned)MAX_INTERVAL);
                continue;
            }

            updated.interval_ms = (uint32_t)value;
        }
        else if (strcmp(command, "SET_THRESHOLD") == 0) {
            if (fields != 2) {
                puts("Usage: SET_THRESHOLD <degrees>");
                continue;
            }

            if (value < MIN_THRESHOLD / 100 ||
                value > MAX_THRESHOLD / 100) {
                puts("Threshold must be between -40 and 125 C.");
                continue;
            }

            updated.threshold_x100 = (int16_t)(value * 100);
        }
        else {
            puts("Unknown command.");
            continue;
        }

       
        if (save_settings(updated)) {
            s = updated;
            show_settings(s);
        }
        else {
            puts("Settings were not updated.");
        }
    }

    return 0;
}

