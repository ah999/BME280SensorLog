/*
  ***************************************************************************************************************
  ***************************************************************************************************************
  ***************************************************************************************************************

  File:		  BME280_STM32.h
  Author:     ControllersTech.com
  Updated:    Dec 14, 2021
  Refactored: Using BME280_Data struct

  ***************************************************************************************************************
  Copyright (C) 2017 ControllersTech.com

  This is a free software under the GNU license, you can redistribute it and/or modify it under the terms
  of the GNU General Public License version 3 as published by the Free Software Foundation.
  This software library is shared with public for educational purposes, without WARRANTY and Author is not liable for any damages caused directly
  or indirectly by this software, read more about this on the GNU General Public License.

  ***************************************************************************************************************
*/


#ifndef INC_BME280_STM32_H_
#define INC_BME280_STM32_H_

#include "stm32f4xx_hal.h"

/* Configuration for the BME280

 * @osrs is the oversampling to improve the accuracy
 *       if osrs is set to OSRS_OFF, the respective measurement will be skipped
 *       It can be set to OSRS_1, OSRS_2, OSRS_4, etc. Check the header file
 *
 * @mode can be used to set the mode for the device
 *       MODE_SLEEP will put the device in sleep
 *       MODE_FORCED device goes back to sleep after one measurement. You need to use the BME280_WakeUP() function before every measurement
 *       MODE_NORMAL device performs measurement in the normal mode. Check datasheet page no 16
 *
 * @t_sb is the standby time. The time sensor waits before performing another measurement
 *       It is used along with the normal mode. Check datasheet page no 16 and page no 30
 *
 * @filter is the IIR filter coefficients
 *         IIR is used to avoid the short term fluctuations
 *         Check datasheet page no 18 and page no 30
 */

// Function prototypes
int BME280_Init(BME280_Data *bme, I2C_HandleTypeDef *i2c_handle, uint8_t address);

int BME280_Config(BME280_Data *bme, uint8_t osrs_t, uint8_t osrs_p, uint8_t osrs_h, 
                  uint8_t mode, uint8_t t_sb, uint8_t filter);


// Read the Trimming parameters saved in the NVM ROM of the device
void TrimRead(BME280_Data *bme);

/* To be used when doing the force measurement
 * the Device need to be put in forced mode every time the measurement is needed
 */
void BME280_WakeUP(BME280_Data *bme);

//read the raw data
BMEReadRaw(BME280_Data *bme);

/* measure the temp, pressure and humidity
 * the values will be stored in the parameters passed to the function
 */
void BME280_Measure (BME280_Data *bme);

// BME280 Data Structure
typedef struct {
    I2C_HandleTypeDef *i2c_handle;
    uint8_t dev_address;
    uint8_t chip_id;
    
    // Raw sensor data
    int32_t tRaw, pRaw, hRaw;
    int32_t t_fine;
    
    // Trimming parameters
    uint16_t dig_T1, dig_P1, dig_H1, dig_H3;
    int16_t dig_T2, dig_T3,
            dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9,
            dig_H2, dig_H4, dig_H5, dig_H6;
    
    // Compensated sensor readings
    float Temperature, Pressure, Humidity;
} BME280_Data;


// Oversampling definitions
#define OSRS_OFF    	0x00
#define OSRS_1      	0x01
#define OSRS_2      	0x02
#define OSRS_4      	0x03
#define OSRS_8      	0x04
#define OSRS_16     	0x05

// MODE Definitions
#define MODE_SLEEP      0x00
#define MODE_FORCED     0x01
#define MODE_NORMAL     0x03

// Standby Time
#define T_SB_0p5    	0x00
#define T_SB_62p5   	0x01
#define T_SB_125    	0x02
#define T_SB_250    	0x03
#define T_SB_500    	0x04
#define T_SB_1000   	0x05
#define T_SB_10     	0x06
#define T_SB_20     	0x07

// IIR Filter Coefficients
#define IIR_OFF     	0x00
#define IIR_2       	0x01
#define IIR_4       	0x02
#define IIR_8       	0x03
#define IIR_16      	0x04


// REGISTERS DEFINITIONS
#define ID_REG      	0xD0
#define RESET_REG  		0xE0
#define CTRL_HUM_REG    0xF2
#define STATUS_REG      0xF3
#define CTRL_MEAS_REG   0xF4
#define CONFIG_REG      0xF5
#define PRESS_MSB_REG   0xF7


#endif /* INC_BME280_STM32_H_ */
