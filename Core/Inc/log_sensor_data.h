#ifndef LOG_SENSOR_DATA_H
#define LOG_SENSOR_DATA_H

#include "main.h"
#include "fatfs.h"
#include "string.h"
#include "stdio.h"
#include "stm32f4xx_hal.h"



// Configuration defines
#define LOG_BUFFER_SIZE         1024
#define UART_BUFFER_SIZE        256
#define SENSOR_DATA_BUFFER_SIZE 256
#define DATA_FILENAME          "sensor_data.csv"
#define LOG_INTERVAL_MS        5000

// Data logging status
typedef enum {
    LOG_OK = 0,
    LOG_SD_MOUNT_ERROR,
    LOG_FILE_CREATE_ERROR,
    LOG_FILE_WRITE_ERROR,
    LOG_FILE_READ_ERROR,
    LOG_FILE_OPEN_ERROR
} LogStatus;

// The main struct for the SD Card Logger
typedef struct {
    // FatFs handles
    FATFS fs;
    FIL fil;
    FATFS *pfs;
    
    // Buffers for data handling
    char log_buffer[LOG_BUFFER_SIZE];
    char uart_buffer[UART_BUFFER_SIZE];
    char sensor_data_buffer[SENSOR_DATA_BUFFER_SIZE];
    
    // File I/O tracking
    UINT bytes_written;
    UINT bytes_read;
    
    // SD Card Information
    DWORD free_clusters;
    uint32_t total_space_kb;
    uint32_t free_space_kb;
    
    // Hardware Handles (to be passed in during initialization)
    UART_HandleTypeDef *uart_handle;
    // RTC_HandleTypeDef *rtc_handle; // Optional
} SD_Card_Logger;

// Sensor data structure
typedef struct {
    float temperature;
    float humidity;
    float pressure;
    uint32_t timestamp;
} SensorData;


// Function declarations
LogStatus log_sensor_init(SD_Card_Logger *logger, UART_HandleTypeDef *uart_handle);
LogStatus get_sd_card_info(SD_Card_Logger *logger);
LogStatus create_data_file_header(SD_Card_Logger *logger);
LogStatus log_sensor_data(SD_Card_Logger *logger, const SensorData *data);
LogStatus display_last_entries(SD_Card_Logger *logger, uint8_t num_entries);
LogStatus clear_log_file(SD_Card_Logger *logger);
LogStatus check_sd_card_status(SD_Card_Logger *logger);


// Utility functions
void send_uart_message(SD_Card_Logger *logger, const char *message);
void clear_log_buffer(SD_Card_Logger *logger);
uint32_t get_buffer_size(const char *buf);
LogStatus check_sd_card_status(SD_Card_Logger *logger);

// File operation helpers
LogStatus open_data_file_append(SD_Card_Logger *logger);
LogStatus open_data_file_read(SD_Card_Logger *logger);
LogStatus close_data_file(SD_Card_Logger *logger);


#endif // LOG_SENSOR_DATA_H
