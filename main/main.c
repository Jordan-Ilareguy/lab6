#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"  
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_spiffs.h"
#include "fs_helpers.h"
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"

/**
 * Mount SPIFFS, open/write/read files, log fake and real samples to CSV,
 * then print the CSV back to the console
 * You can also save data from CSV to Excel
 */

void app_main() {
  
    //*---------------------------------------------------------
    //Demo 1
    // Mount SPIFFS at "/spiffs" and print total/used bytes.
    // If the mount fails, fs_mount_or_die() will abort (via ESP_ERROR_CHECK).
    
    /*

    fs_mount_or_die();

    while (1) {
        ESP_LOGI("MAIN", "Still alive, looping...");  // Heartbeat to prove the task is running
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    
   ---------------------------------------------------------

    //Demo 2.1
    
     * Demonstrates how to:
     *   1. Mount SPIFFS filesystem.
     *   2. Create/write a file in SPIFFS.
     *   3. Read the file back and display its contents.
     *   4. Reboot the ESP32 to prove that the file persists across resets.
     */
    // 1: Mount the SPIFFS filesystem.
    // If the partition is invalid/unformatted, fs_mount_or_die() will format and mount it.
    
    /*

    fs_mount_or_die();

    // 2: Open a file for writing in SPIFFS.
    // Path must begin with the mount point "/spiffs".
    FILE *f = fopen("/spiffs/fav_song.txt", "w");
    if (!f) { 
        // fopen() failed → log and exit function (can't continue without file access).
        printf("open for write failed\n"); 
        return; 
    }

    // 3: Write text into the file.
    // fprintf() works just like on a normal computer.
    fprintf(f, "You're the lullaby\nThat's singing me to sleep\nYou are the other half\nYou're like a missing piece\n");

    // 4: Close the file to ensure data is flushed to flash.
    fclose(f);
    printf("[+] wrote fav_song.txt\n");

    // 5: Reopen the same file and read back its contents.
    // This uses our helper function defined in fs_helpers.c.
    fs_print_file("/spiffs/fav_song.txt");

    // 6: Prove persistence.
    // Wait 2 seconds, then reboot the ESP32.
    // After reboot, SPIFFS will still contain "fav_song.txt".
    printf("Rebooting in 10s to prove persistence...\n");
    vTaskDelay(pdMS_TO_TICKS(10000));  // Delay = 10000 ms = 10 seconds
    esp_restart();                    // Trigger a software reset
    
    */

    //---------------------------------------------------------

    //Demo 2.2
    /**
     * Flow:
     *   1. Mount SPIFFS.
     *   2. Create/write a file ("fav_song.txt") if it doesn't exist.
     *   3. Read back the file contents.
     *   4. Append a new line to the file.
     *   5. Read back again to verify the appended line.
     *   6. Reboot to demonstrate persistence across resets.
     */
    
    /*

    // Step 1: Mount SPIFFS
    fs_mount_or_die();

    // Step 2: Open fav_song.txt for appending
    FILE *f = fopen("/spiffs/fav_song.txt", "a");
    if (!f) {
        printf("open for write failed\n");
        return;
    }
    fprintf(f, "Hello File System Lab!\nLine 2.\n");
    fclose(f);
    printf("[+] wrote fav_song.txt\n");

    // Step 3: Read back file
    fs_print_file("/spiffs/fav_song.txt");

    // Step 4: Append a new line
    f = fopen("/spiffs/fav_song.txt", "a");   // "a" = append mode
    if (!f) {
        printf("open for append failed\n");
        return;
    }
    fprintf(f, "Line 3 after update.\n");
    fclose(f);
    printf("[+] appended new line to fav_song.txt\n");

    // Step 5: Read file again (should now include the appended line)
    fs_print_file("/spiffs/fav_song.txt");

    // Step 6: Reboot after 10 seconds to show persistence
    printf("Rebooting in 10s to prove persistence...\n");
    vTaskDelay(pdMS_TO_TICKS(10000));
    esp_restart();
    
    */

    //---------------------------------------------------------

    //Demo 3.1
    /**
     * Demonstrates:
     *   1. Mounting the SPIFFS filesystem.
     *   2. Logging sample CSV data to a file.
     *   3. Reading back the file to verify contents.
     */
    // 1: Mount SPIFFS.
    
    /*

    fs_mount_or_die();

    // 2: Append 10 rows of fake data to "data.csv".
    // Each row has the format: time step, servo angle, sensor reading.
    // If "data.csv" does not exist, it will be created automatically.
    log_csv_sample("/spiffs/data.csv", 10);

    // 3: Open "data.csv" for reading and print its contents line by line
    // to the serial monitor. This verifies that data was logged successfully.
    fs_print_file("/spiffs/data.csv");
    
    */

    //---------------------------------------------------------

    /*

    // Demo 3.2: 20 Samples over 20 x 2 = 40s
    
    fs_mount_or_die();       // make /spiffs available
    adc_oneshot_setup();     // init ADC channel

    log_pot_samples_csv(LOG_PATH, 20, SAMPLE_PERIOD_MS);  // log 20 samples
    fs_print_file(LOG_PATH); // verify contents
    
    */

    //---------------------------------------------------------

    // Demo 3.3: CSV to Excel
    
    fs_mount_or_die();
    adc_oneshot_setup();

    // user starts to log within 6s pressing Ctrl+T then Ctrl+L
    vTaskDelay(pdMS_TO_TICKS(6000)); 

    log_pot_samples_csv(LOG_PATH, 20, SAMPLE_PERIOD_MS);

    // When ready to export, print the file as pure CSV over USB:
    print_csv_file_only(LOG_PATH);

    // Give time for all UART data to transmit
    vTaskDelay(pdMS_TO_TICKS(2000));

    fs_print_file(LOG_PATH); // verify contents

    // Unmount SPIFFS and end the program
    esp_vfs_spiffs_unregister(NULL);
    

    //---------------------------------------------------------

}