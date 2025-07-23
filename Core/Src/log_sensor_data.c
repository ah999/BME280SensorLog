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

