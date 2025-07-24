#include "log_sensor_data.h"

// Global variables for logging

static FRESULT fresult;




// Optional RTC handle (uncomment if using RTC)
// extern RTC_HandleTypeDef hrtc;

LogStatus log_sensor_init(SD_Card_Logger *logger, UART_HandleTypeDef *uart_handle)
{
    if (!logger || !uart_handle) {
        return LOG_SD_MOUNT_ERROR;
    }
    
    // Initialize logger structure
    memset(logger, 0, sizeof(SD_Card_Logger));
    logger->uart_handle = uart_handle;
    
    HAL_Delay(500);

    // Mount SD card
    fresult = f_mount(&logger->fs, "/", 1);
    if (fresult != FR_OK) {
        send_uart_message(logger, "ERROR!!! in mounting SD CARD...\r\n");
        return LOG_SD_MOUNT_ERROR;
    }
    else {
        send_uart_message(logger, "SD CARD mounted successfully...\r\n");
    }

    // Get SD card information
    if (get_sd_card_info(logger) != LOG_OK) {
        send_uart_message(logger, "Warning: Could not get SD card info\r\n");
    }

    // Create data file header if needed
    if (create_data_file_header(logger) != LOG_OK) {
        send_uart_message(logger, "ERROR!!! Could not create data file header\r\n");
        return LOG_FILE_CREATE_ERROR;
    }

    send_uart_message(logger, "Sensor data logging system initialized\r\n");
    return LOG_OK;
}

// Get SD card capacity information
LogStatus get_sd_card_info(SD_Card_Logger *logger)
{
    if (!logger) {
        return LOG_SD_MOUNT_ERROR;
    }

    fresult = f_getfree("", &logger->free_clusters, &logger->pfs);
    if (fresult != FR_OK) {
        return LOG_SD_MOUNT_ERROR;
    }

    logger->total_space_kb = (uint32_t)((logger->pfs->n_fatent - 2) * logger->pfs->csize * 0.5);
    sprintf(logger->log_buffer, "SD CARD Total Size: %lu KB\r\n", logger->total_space_kb);
    send_uart_message(logger, logger->log_buffer);
    clear_log_buffer(logger);


    logger->free_space_kb = (uint32_t)(logger->free_clusters * logger->pfs->csize * 0.5);
    sprintf(logger->log_buffer, "SD CARD Free Space: %lu KB\r\n", logger->free_space_kb);
    send_uart_message(logger, logger->log_buffer);
    clear_log_buffer(logger);

    return LOG_OK;
}

// Create CSV header if file doesn't exist
LogStatus create_data_file_header(SD_Card_Logger *logger)
{
    if (!logger) {
        return LOG_FILE_CREATE_ERROR;
    }

    // Check if data file exists
    fresult = f_open(&logger->fil, DATA_FILENAME, FA_READ);

    if (fresult == FR_NO_FILE) {
        // File doesn't exist, create it with header
        fresult = f_open(&logger->fil, DATA_FILENAME, FA_CREATE_NEW | FA_WRITE);
        if (fresult != FR_OK) {
            return LOG_FILE_CREATE_ERROR;
        }

        // Write CSV header
        strcpy(logger->log_buffer, "Timestamp,Temperature(C),Humidity(%),Pressure(hPa)\r\n");
        fresult = f_write(&logger->fil, logger->log_buffer, strlen(logger->log_buffer), &logger->bytes_written);

        if (fresult != FR_OK) {
            f_close(&logger->fil);
            return LOG_FILE_WRITE_ERROR;
        }

        f_close(&logger->fil);
        send_uart_message(logger, "New data file created with header\r\n");
    }
    else if (fresult == FR_OK) {
        // File exists, just close it
        f_close(&logger->fil);
        send_uart_message(logger, "Existing data file found\r\n");
    }
    else {
        return LOG_FILE_OPEN_ERROR;
    }

    return LOG_OK;
}






// Log sensor data to SD card
LogStatus log_sensor_data(SD_Card_Logger *logger, const SensorData *data)
{
    if (!logger || !data) {
        return LOG_FILE_WRITE_ERROR;
    }

    // Open file in append mode
    fresult = f_open(&logger->fil, DATA_FILENAME, FA_OPEN_ALWAYS | FA_WRITE);

    if (fresult != FR_OK) {
        send_uart_message(logger, "Error opening data file\r\n");
        return LOG_FILE_OPEN_ERROR;
    }

    // Move to end of file for appending
    fresult = f_lseek(&logger->fil, f_size(&logger->fil));
    if (fresult != FR_OK) {
        f_close(&logger->fil);
        return LOG_FILE_WRITE_ERROR;
    }

    // Format sensor data as CSV
    sprintf(logger->sensor_data_buffer, "%lu,%.2f,%.2f,%.2f\r\n",
            data->timestamp, data->temperature, data->humidity, data->pressure);

    // Write data to file
    fresult = f_write(&logger->fil, logger->sensor_data_buffer, strlen(logger->sensor_data_buffer), &logger->bytes_written);

    if (fresult != FR_OK) {
        f_close(&logger->fil);
        send_uart_message(logger, "Error writing to SD card\r\n");
        return LOG_FILE_WRITE_ERROR;
    }

    // Close file
    f_close(&logger->fil);

    send_uart_message(logger, "Data logged to SD card successfully\r\n");
    return LOG_OK;
}


// Display last N entries from log file
LogStatus display_last_entries(SD_Card_Logger *logger, uint8_t num_entries)
{
    if (!logger) {
        return LOG_FILE_READ_ERROR;
    }
    
    char line[256];
    uint32_t file_size;
    uint32_t line_count = 0;

    // Open file for reading
    fresult = f_open(&logger->fil, DATA_FILENAME, FA_READ);

    if (fresult != FR_OK) {
        send_uart_message(logger, "Error reading data file\r\n");
        return LOG_FILE_READ_ERROR;
    }

    file_size = f_size(&logger->fil);

    // Read file line by line to count total lines
    while (f_gets(line, sizeof(line), &logger->fil)) {
        line_count++;
    }

    // Go back to beginning
    f_lseek(&logger->fil, 0);

    // Skip header and older entries
    uint32_t skip_lines = (line_count > num_entries + 1) ? (line_count - num_entries) : 1;

    for (uint32_t i = 0; i < skip_lines; i++) {
        f_gets(line, sizeof(line), &logger->fil);
    }

    // Display remaining entries
    send_uart_message(logger, "Last sensor readings:\r\n");
    while (f_gets(line, sizeof(line), &logger->fil)) {
        send_uart_message(logger, line);
    }

    f_close(&logger->fil);
    return LOG_OK;
}

// Clear all logged data
LogStatus clear_log_file(SD_Card_Logger *logger)
{
    if (!logger) {
        return LOG_FILE_OPEN_ERROR;
    }
    
    // Delete existing file
    FRESULT fresult = f_unlink(DATA_FILENAME);

    if (fresult != FR_OK) {
        send_uart_message(logger, "Error clearing log file\r\n");
        return LOG_FILE_OPEN_ERROR;
    }

    send_uart_message(logger, "Log file cleared successfully\r\n");

    // Recreate file with header
    if (create_data_file_header(logger) != LOG_OK) {
        return LOG_FILE_CREATE_ERROR;
    }

    return LOG_OK;
}

// Check SD card status
LogStatus check_sd_card_status(SD_Card_Logger *logger)
{
    if (!logger) {
        return LOG_SD_MOUNT_ERROR;
    }
    
    // Try to open a test file
    fresult = f_open(&logger->fil, "test.tmp", FA_CREATE_ALWAYS | FA_WRITE);

    if (fresult != FR_OK) {
        return LOG_SD_MOUNT_ERROR;
    }

    f_close(&logger->fil);
    f_unlink("test.tmp");

    return LOG_OK;
}

// File operation helpers
LogStatus open_data_file_append(SD_Card_Logger *logger)
{
    if (!logger) {
        return LOG_FILE_OPEN_ERROR;
    }
    
    fresult = f_open(&logger->fil, DATA_FILENAME, FA_OPEN_ALWAYS | FA_WRITE);
    if (fresult != FR_OK) {
        return LOG_FILE_OPEN_ERROR;
    }

    // Move to end of file
    fresult = f_lseek(&logger->fil, f_size(&logger->fil));
    if (fresult != FR_OK) {
        f_close(&logger->fil);
        return LOG_FILE_WRITE_ERROR;
    }

    return LOG_OK;
}

LogStatus open_data_file_read(SD_Card_Logger *logger)
{
    if (!logger) {
        return LOG_FILE_READ_ERROR;
    }
    
    fresult = f_open(&logger->fil, DATA_FILENAME, FA_READ);
    if (fresult != FR_OK) {
        return LOG_FILE_READ_ERROR;
    }

    return LOG_OK;
}

LogStatus close_data_file(SD_Card_Logger *logger)
{
    if (!logger) {
        return LOG_FILE_WRITE_ERROR;
    }
    
    fresult = f_close(&logger->fil);
    if (fresult != FR_OK) {
        return LOG_FILE_WRITE_ERROR;
    }

    return LOG_OK;
}

// Utility functions
void send_uart_message(SD_Card_Logger *logger, const char *message)
{
    if (!logger || !logger->uart_handle || !message) {
        return;
    }
    
    HAL_UART_Transmit(logger->uart_handle, (uint8_t*)message, strlen(message), HAL_MAX_DELAY);
}

void clear_log_buffer(SD_Card_Logger *logger)
{
    if (!logger) {
        return;
    }
    
    memset(logger->log_buffer, 0, sizeof(logger->log_buffer));
}

uint32_t get_buffer_size(const char *buf)
{
    if (!buf) {
        return 0;
    }
    
    return strlen(buf);
}
