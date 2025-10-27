#ifndef FS_HELPERS_H
#define FS_HELPERS_H

#include "hal/adc_types.h"

static const char LOG_PATH[]  = "/spiffs/potdata.csv";        // file to store pot samples (Demo 3.2)
static const char TEMP_PATH[] = "/spiffs/thermodata.csv";     // file to store thermistor samples (Demo 3.2)

// Potentiometer config (Demo 3.2)
#define ADC_BITS          12
#define ADC_MAX           ((1 << ADC_BITS) - 1)  // 4095
#define SAMPLES           8                      // average this many samples
#define SAMPLE_PERIOD_MS  2000                    // delay between prints/logs
#define POT               4  // GPIO number where potentiometer is connected
#define ADC_CH_POT        ADC_CHANNEL_3   // GPIO4

// GPIO mapping (Demo 3.2)
#define THERMISTOR       5  // GPIO number where thermistor is connected
#define ADC_UNIT_ID       ADC_UNIT_1
#define ADC_CH_THERMISTOR ADC_CHANNEL_4   // GPIO5

// File system & file handling functions (Demo 1, 2.1, 2.2, 3.1)
void fs_mount_or_die();
void fs_print_file(const char *path);
void log_csv_sample(const char *path, int samples);
void log_thermistor_samples_csv(const char *path, int samples, int period);

// ADC functions (Demo 3.2)
// Log potentiometer samples to CSV at 'path'; take 'samples' readings spaced 'period' ms apart
void log_pot_samples_csv(const char *path, int samples, int period);
void adc_oneshot_setup();
int adc_read_avg(adc_channel_t ch, int samples); // Read and average multiple ADC samples from specified channel

// csv to excel (Demo 3.3)
void print_csv_file_only(const char *path); // Function declaration for printing CSV file only over serial

// Forward declaration of adc_oneshot_setup function
void adc_oneshot_setup(void);

#endif
