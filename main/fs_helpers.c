/**
 * @file fs_helpers.c
 * @brief Helper functions for mounting and interacting with the SPIFFS filesystem and ADC on ESP32.
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"          
#include "esp_err.h"  
#include "esp_log.h"     
#include "esp_spiffs.h"     // SPIFFS filesystem support
#include "esp_system.h"
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "fs_helpers.h"

// Handle for oneshot ADC
static adc_oneshot_unit_handle_t adc1_handle = NULL;

// Tag used for ESP_LOG macros to identify logs from this file
static const char *TAG = "FS";

/**
 * @brief Mount the SPIFFS filesystem and log its total/used size.
 *
 * This function mounts the SPIFFS partition defined in the partition table.
 * If mounting fails and the `format_if_mount_failed` flag is set, it will
 * automatically format the partition and retry.
 *
 * On success, the function logs the total and used size of the SPIFFS partition.
 * On failure, the program aborts (due to ESP_ERROR_CHECK).
 *
 * @param None
 * @return void
 */
void fs_mount_or_die() {
    // Configure the SPIFFS mount settings
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",         // Mount point in the virtual filesystem
        .partition_label = NULL,        // NULL = use default "spiffs" partition
        .max_files = 8,                 // Max number of files that can be open at once using fopen()
        .format_if_mount_failed = true  // Format partition if mounting fails
    };

    // Attempt to register (mount) the SPIFFS filesystem
    // If it fails, ESP_ERROR_CHECK aborts the program with an error message
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    // Variables to hold total and used size of SPIFFS
    size_t total = 0, used = 0;

    // Query the filesystem info (total and used space in bytes)
    ESP_ERROR_CHECK(esp_spiffs_info(NULL, &total, &used));

    // Log the result
    ESP_LOGI(TAG, "SPIFFS mounted. total=%u bytes, used=%u bytes",
             (unsigned)total, (unsigned)used);
}

/**
 * @brief Print the contents of a file stored in SPIFFS.
 *
 * This function opens a file in read-only mode, reads it line by line,
 * and prints its contents to the console. It is mainly used for debugging
 * to verify file contents inside the SPIFFS filesystem.
 *
 * @param path Absolute or relative file path (e.g., "/spiffs/data.txt").
 * @return void
 */
void fs_print_file(const char *path) {
    // Try to open the file in read mode
    FILE *f = fopen(path, "r");

    // If opening fails, print an error and return
    if (!f) {
        printf("[-] open for read failed: %s\n", path);
        return;
    }

    // File opened successfully → announce which file is being read
    printf("[*] contents of %s:\n", path);

    // Buffer to hold lines read from the file
    char buf[128];

    // Read and print each line until EOF is reached
    while (fgets(buf, sizeof buf, f)) {
        printf("%s", buf);
    }

    // Close the file after reading
    fclose(f);

    // Add a newline for readability
    printf("\n");
}

/**
 * @brief Append simulated CSV data to a file in SPIFFS.
 *
 * This function opens (or creates) a file in append mode and writes
 * `samples` rows of fake data in CSV format: 
 *      time step, servo angle, sensor reading
 *
 * @param path     File path (e.g., "/spiffs/data.csv")
 * @param samples  Number of rows to append
 *
 * @return void
 */
void log_csv_sample(const char *path, int samples) {
    // Step 1: Open the file in append mode ("a").
    // If the file doesn't exist, it will be created.
    FILE *f = fopen(path, "a");
    if (!f) { 
        // fopen failed (e.g., no filesystem, path invalid, etc.)
        printf("open for append failed: %s\n", path); 
        return; 
    }

    // Step 2: Print a message to the serial monitor to show progress.
    printf("[+] appending %d rows to %s\n", samples, path);

    // Step 3: Write 'samples' rows of fake data.
    // Each row is written in CSV format: time, angle, sensor
    for (int t = 0; t < samples; t++) {
        int angle = (t * 15) % 180;          // Pretend “servo angle” (cycles 0–179°)
        int sensor = 100 + (t * 3) % 50;     // Pretend “sensor value” (100–149)
        fprintf(f, "%d,%d,%d\n", t, angle, sensor);
        // Example row: "0,0,100"
    }

    // Step 4: Close the file to flush buffers and save changes.
    fclose(f);
}

/**
 * @brief Initialize ADC in one-shot mode for the potentiometer/thermistor channels.
 */
void adc_oneshot_setup() {
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_ID
    };
    adc_oneshot_new_unit(&unit_cfg, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten    = ADC_ATTEN_DB_12   // allows ~0–3.3V range
    };
    
    // Configure both potentiometer and thermistor channels
    adc_oneshot_config_channel(adc1_handle, ADC_CH_POT, &chan_cfg);
    adc_oneshot_config_channel(adc1_handle, ADC_CH_THERMISTOR, &chan_cfg);
}

/**
 * @brief Read 'samples' ADC values and return the average.
 * 
 * @param ch       ADC channel to read from
 * @param samples  Number of samples to average
 * @return int     Average raw ADC value (0–4095 for 12-bit)
 */
int adc_read_avg(adc_channel_t ch, int samples) {
    long sum = 0;
    for (int i = 0; i < samples; ++i) {
        int raw = 0;
        adc_oneshot_read(adc1_handle, ch, &raw);
        sum += raw;
        vTaskDelay(pdMS_TO_TICKS(2));  // small pause for stability
    }
    return (int)(sum / samples);
}

/**
 * @brief Append potentiometer readings to a CSV file.
 * 
 * @param path     File path in SPIFFS (e.g., "/spiffs/data.csv")
 * @param samples  Number of rows to log
 * @param period   Delay between samples (ms)
 */

void log_thermistor_samples_csv(const char *path, int samples, int period) {
    FILE *f = fopen(path, "a");  // append mode - adds to existing file
    if (!f) {
        printf("open for append failed: %s\n", path);
        return;
    }
    
    // Write header only if file is empty (new file)
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) {
        fprintf(f, "index,temperature_C\n");
    }

    for (int i = 0; i < samples; i++) {
        printf("Collecting sample %d of %d...\n", i + 1, samples);
        int raw = adc_read_avg(ADC_CH_THERMISTOR, SAMPLES);
        
        // Thermistor temperature calculation using Beta equation
        const float Vin = 3.3f;           // Supply voltage
        const float R_fixed = 10000.0f;   // 10k series resistor
        const float R0 = 10000.0f;        // Thermistor resistance at 25°C
        const float T0 = 25.0f + 273.15f; // 25°C in Kelvin
        const float B = 3950.0f;          // Beta coefficient
        
        // Convert ADC to voltage
        float VRT = ((float)raw * Vin) / (float)ADC_MAX;
        
        // Calculate thermistor resistance using voltage divider
        // Divider: Vin -> R_fixed -> node(VRT) -> Thermistor -> GND
        float RT = (R_fixed * VRT) / (Vin - VRT);
        
        // Beta equation: 1/T = 1/T0 + (1/B)*ln(RT/R0)
        float T_kelvin = 1.0f / ((1.0f / T0) + (logf(RT / R0) / B));
        float temperature = T_kelvin - 273.15f; // Convert to Celsius

        fprintf(f, "#%d, %.2f°C\n", i, temperature);
        fflush(f); // flush to SPIFFS
        vTaskDelay(pdMS_TO_TICKS(period));
    }

    fclose(f);
}

/**
 * @brief Print only the contents of a CSV file, without extra log messages.
 * 
 * This function opens the specified file from SPIFFS and streams its content
 * directly to the serial terminal exactly as stored — ideal for exporting
 * clean CSV output that can be redirected to a file and opened in Excel.
 * 
 * @param path  File path in SPIFFS (e.g., "/spiffs/data.csv")
 * 
 * @note This function suppresses ESP-IDF system logs so the serial output
 *       contains only the CSV lines, making it easier to capture or redirect.
 *       If the file is not found, it prints a short CSV-formatted error
 *       message so the output remains readable in Excel.
 */
void print_csv_file_only(const char *path) {
    // Suppress ESP-IDF info/debug logs so only our CSV is printed
    esp_log_level_set("*", ESP_LOG_WARN);

    // Open file for reading
    FILE *f = fopen(path, "r");
    if (f) {
        char buf[256];
        size_t n;

        // Read chunks from file and print exactly as they are
        // Each chunk may contain multiple CSV lines already ending in '\n'
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
            fwrite(buf, 1, n, stdout);  // write to standard output (serial)
        }
        fclose(f);
    } else {
        // If file couldn't be opened, still output valid CSV format
        printf("error,message\r\n,Could not open file\r\n");
    }
}


