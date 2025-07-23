#ifndef LOG_SENSOR_DATA_H
#define LOG_SENSOR_DATA_H

#include "main.h"
#include "fatfs.h"
#include "string.h"
#include "stdio.h"
#include "stm32f4xx_hal.h"

// Data logging status
typedef enum {
    LOG_OK = 0,
    LOG_SD_MOUNT_ERROR,
    LOG_FILE_CREATE_ERROR,
    LOG_FILE_WRITE_ERROR,
    LOG_FILE_READ_ERROR,
    LOG_FILE_OPEN_ERROR
} LogStatus;

// Sensor data structure
typedef struct {
    float temperature;
    float humidity;
    float pressure;
    uint32_t timestamp;
} SensorData;

// Configuration defines
#define LOG_BUFFER_SIZE         1024
#define UART_BUFFER_SIZE        256
#define SENSOR_DATA_BUFFER_SIZE 256
#define DATA_FILENAME          "sensor_data.csv"
#define LOG_INTERVAL_MS        5000

// Function prototypes
LogStatus log_sensor_init(void);
LogStatus log_sensor_data(SensorData *data);
LogStatus log_sensor_data_with_rtc(SensorData *data);
LogStatus create_data_file_header(void);
LogStatus display_last_entries(uint8_t num_entries);
LogStatus clear_log_file(void);
LogStatus get_sd_card_info(void);

// Utility functions
void send_uart_message(const char *message);
void clear_log_buffer(void);
uint32_t get_buffer_size(const char *buf);
LogStatus check_sd_card_status(void);

// File operations
LogStatus open_data_file_append(void);
LogStatus open_data_file_read(void);
LogStatus close_data_file(void);

#endif // LOG_SENSOR_DATA_H
