#include "log_sensor_data.h"

// Global variables for logging
static FATFS fs;
static FIL fil;
static FRESULT fresult;
static char log_buffer[LOG_BUFFER_SIZE];
static char uart_buffer[UART_BUFFER_SIZE];
static char sensor_data_buffer[SENSOR_DATA_BUFFER_SIZE];
static UINT bytes_written, bytes_read;
static DWORD free_clusters;
static FATFS *pfs;
static uint32_t total_space, free_space;

// External UART handle (should be defined in main.c)
extern UART_HandleTypeDef huart1;

// Optional RTC handle (uncomment if using RTC)
// extern RTC_HandleTypeDef hrtc;

// Initialize SD card and logging system
LogStatus log_sensor_init(void)
{
    HAL_Delay(500);

    // Mount SD card
    fresult = f_mount(&fs, "/", 1);
    if (fresult != FR_OK) {
        send_uart_message("ERROR!!! in mounting SD CARD...\r\n");
        return LOG_SD_MOUNT_ERROR;
    }
    else {
        send_uart_message("SD CARD mounted successfully...\r\n");
    }

    // Get SD card information
    if (get_sd_card_info() != LOG_OK) {
        send_uart_message("Warning: Could not get SD card info\r\n");
    }

    // Create data file header if needed
    if (create_data_file_header() != LOG_OK) {
        send_uart_message("ERROR!!! Could not create data file header\r\n");
        return LOG_FILE_CREATE_ERROR;
    }

    send_uart_message("Sensor data logging system initialized\r\n");
    return LOG_OK;
}

// Get SD card capacity information
LogStatus get_sd_card_info(void)
{
    fresult = f_getfree("", &free_clusters, &pfs);
    if (fresult != FR_OK) {
        return LOG_SD_MOUNT_ERROR;
    }

    total_space = (uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5);
    sprintf(log_buffer, "SD CARD Total Size: %lu KB\r\n", total_space);
    send_uart_message(log_buffer);
    clear_log_buffer();

    free_space = (uint32_t)(free_clusters * pfs->csize * 0.5);
    sprintf(log_buffer, "SD CARD Free Space: %lu KB\r\n", free_space);
    send_uart_message(log_buffer);
    clear_log_buffer();

    return LOG_OK;
}

// Create CSV header if file doesn't exist
LogStatus create_data_file_header(void)
{
    // Check if data file exists
    fresult = f_open(&fil, DATA_FILENAME, FA_READ);

    if (fresult == FR_NO_FILE) {
        // File doesn't exist, create it with header
        fresult = f_open(&fil, DATA_FILENAME, FA_CREATE_NEW | FA_WRITE);
        if (fresult != FR_OK) {
            return LOG_FILE_CREATE_ERROR;
        }

        // Write CSV header
        strcpy(log_buffer, "Timestamp,Temperature(C),Humidity(%),Pressure(hPa)\r\n");
        fresult = f_write(&fil, log_buffer, strlen(log_buffer), &bytes_written);

        if (fresult != FR_OK) {
            f_close(&fil);
            return LOG_FILE_WRITE_ERROR;
        }

        f_close(&fil);
        send_uart_message("New data file created with header\r\n");
    }
    else if (fresult == FR_OK) {
        // File exists, just close it
        f_close(&fil);
        send_uart_message("Existing data file found\r\n");
    }
    else {
        return LOG_FILE_OPEN_ERROR;
    }

    return LOG_OK;
}






// Log sensor data to SD card
LogStatus log_sensor_data(SensorData *data)
{
    // Open file in append mode
    fresult = f_open(&fil, DATA_FILENAME, FA_OPEN_ALWAYS | FA_WRITE);

    if (fresult != FR_OK) {
        send_uart_message("Error opening data file\r\n");
        return LOG_FILE_OPEN_ERROR;
    }

    // Move to end of file for appending
    fresult = f_lseek(&fil, f_size(&fil));
    if (fresult != FR_OK) {
        f_close(&fil);
        return LOG_FILE_WRITE_ERROR;
    }

    // Format sensor data as CSV
    sprintf(sensor_data_buffer, "%lu,%.2f,%.2f,%.2f\r\n",
            data->timestamp, data->temperature, data->humidity, data->pressure);

    // Write data to file
    fresult = f_write(&fil, sensor_data_buffer, strlen(sensor_data_buffer), &bytes_written);

    if (fresult != FR_OK) {
        f_close(&fil);
        send_uart_message("Error writing to SD card\r\n");
        return LOG_FILE_WRITE_ERROR;
    }

    // Close file
    f_close(&fil);

    send_uart_message("Data logged to SD card successfully\r\n");
    return LOG_OK;
}


// Display last N entries from log file
LogStatus display_last_entries(uint8_t num_entries)
{
    char line[256];
    uint32_t file_size;
    uint32_t line_count = 0;

    // Open file for reading
    fresult = f_open(&fil, DATA_FILENAME, FA_READ);

    if (fresult != FR_OK) {
        send_uart_message("Error reading data file\r\n");
        return LOG_FILE_READ_ERROR;
    }

    file_size = f_size(&fil);

    // Read file line by line to count total lines
    while (f_gets(line, sizeof(line), &fil)) {
        line_count++;
    }

    // Go back to beginning
    f_lseek(&fil, 0);

    // Skip header and older entries
    uint32_t skip_lines = (line_count > num_entries + 1) ? (line_count - num_entries) : 1;

    for (uint32_t i = 0; i < skip_lines; i++) {
        f_gets(line, sizeof(line), &fil);
    }

    // Display remaining entries
    send_uart_message("Last sensor readings:\r\n");
    while (f_gets(line, sizeof(line), &fil)) {
        send_uart_message(line);
    }

    f_close(&fil);
    return LOG_OK;
}

// Clear all logged data
LogStatus clear_log_file(void)
{
    // Delete existing file
    fresult = f_unlink(DATA_FILENAME);

    if (fresult != FR_OK) {
        send_uart_message("Error clearing log file\r\n");
        return LOG_FILE_OPEN_ERROR;
    }

    send_uart_message("Log file cleared successfully\r\n");

    // Recreate file with header
    if (create_data_file_header() != LOG_OK) {
        return LOG_FILE_CREATE_ERROR;
    }

    return LOG_OK;
}

// Check SD card status
LogStatus check_sd_card_status(void)
{
    // Try to open a test file
    fresult = f_open(&fil, "test.tmp", FA_CREATE_ALWAYS | FA_WRITE);

    if (fresult != FR_OK) {
        return LOG_SD_MOUNT_ERROR;
    }

    f_close(&fil);
    f_unlink("test.tmp");

    return LOG_OK;
}

// File operation helpers
LogStatus open_data_file_append(void)
{
    fresult = f_open(&fil, DATA_FILENAME, FA_OPEN_ALWAYS | FA_WRITE);
    if (fresult != FR_OK) {
        return LOG_FILE_OPEN_ERROR;
    }

    // Move to end of file
    fresult = f_lseek(&fil, f_size(&fil));
    if (fresult != FR_OK) {
        f_close(&fil);
        return LOG_FILE_WRITE_ERROR;
    }

    return LOG_OK;
}

LogStatus open_data_file_read(void)
{
    fresult = f_open(&fil, DATA_FILENAME, FA_READ);
    if (fresult != FR_OK) {
        return LOG_FILE_READ_ERROR;
    }

    return LOG_OK;
}

LogStatus close_data_file(void)
{
    fresult = f_close(&fil);
    if (fresult != FR_OK) {
        return LOG_FILE_WRITE_ERROR;
    }

    return LOG_OK;
}

// Utility functions
void send_uart_message(const char *message)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)message, strlen(message), HAL_MAX_DELAY);
}

void clear_log_buffer(void)
{
    memset(log_buffer, 0, sizeof(log_buffer));
}

uint32_t get_buffer_size(const char *buf)
{
    return strlen(buf);
}
