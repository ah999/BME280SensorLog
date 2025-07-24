/*
  ***************************************************************************************************************
  ***************************************************************************************************************
  ***************************************************************************************************************

  File:		  BME280_STM32.c
  Author:     ControllersTech.com
  Updated:    Dec 14, 2021

  ***************************************************************************************************************
  Copyright (C) 2017 ControllersTech.com

  This is a free software under the GNU license, you can redistribute it and/or modify it under the terms
  of the GNU General Public License version 3 as published by the Free Software Foundation.
  This software library is shared with public for educational purposes, without WARRANTY and Author is not liable for any damages caused directly
  or indirectly by this software, read more about this on the GNU General Public License.

  ***************************************************************************************************************
*/

#include "BME280_STM32.h"

extern I2C_HandleTypeDef hi2c1;
#define BME280_I2C &hi2c1

#define SUPPORT_64BIT 1
//#define SUPPORT_32BIT 1

#define BME280_ADDRESS 0xEC  // SDIO is grounded, the 7 bit address is 0x76 and 8 bit address = 0x76<<1 = 0xEC


extern float Temperature, Pressure, Humidity;

uint8_t chipID;

uint8_t TrimParam[36];
int32_t tRaw, pRaw, hRaw;

uint16_t dig_T1,  \
         dig_P1, \
         dig_H1, dig_H3;

int16_t  dig_T2, dig_T3, \
         dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9, \
		 dig_H2,  dig_H4, dig_H5, dig_H6;

// Initialize the BME280 sensor
int BME280_Init(BME280_Data *bme, I2C_HandleTypeDef *i2c_handle, uint8_t address)
{
    if (bme == NULL || i2c_handle == NULL) {
        return -1;
    }
    
    bme->i2c_handle = i2c_handle;
    bme->dev_address = address;
    
    // Read chip ID to verify device presence
    if (HAL_I2C_Mem_Read(bme->i2c_handle, bme->dev_address, ID_REG, 1, 
                         &bme->chip_id, 1, 1000) != HAL_OK) {
        return -1;
    }
    
    // Verify chip ID (BME280 should return 0x60)
    if (bme->chip_id != 0x60) {
        return -1;
    }
    
    // Initialize sensor readings to zero
    bme->Temperature = 0.0f;
    bme->Pressure = 0.0f;
    bme->Humidity = 0.0f;
    
    return 0;
}
// Read the Trimming parameters saved in the NVM ROM of the device
void TrimRead(@BME280_Data *bme)
{
	uint8_t trimdata[32];
	// Read NVM from 0x88 to 0xA1
    HAL_I2C_Mem_Read(bme->i2c_handle, bme->dev_address, 0x88, 1, trimdata, 25, HAL_MAX_DELAY);
    
    // Read NVM from 0xE1 to 0xE7
    HAL_I2C_Mem_Read(bme->i2c_handle, bme->dev_address, 0xE1, 1, (uint8_t *)trimdata+25, 7, HAL_MAX_DELAY);
    
    // Arrange the data as per the datasheet (page no. 24)
    bme->dig_T1 = (trimdata[1]<<8) | trimdata[0];
    bme->dig_T2 = (trimdata[3]<<8) | trimdata[2];
    bme->dig_T3 = (trimdata[5]<<8) | trimdata[4];
    bme->dig_P1 = (trimdata[7]<<8) | trimdata[6];
    bme->dig_P2 = (trimdata[9]<<8) | trimdata[8];
    bme->dig_P3 = (trimdata[11]<<8) | trimdata[10];
    bme->dig_P4 = (trimdata[13]<<8) | trimdata[12];
    bme->dig_P5 = (trimdata[15]<<8) | trimdata[14];
    bme->dig_P6 = (trimdata[17]<<8) | trimdata[16];
    bme->dig_P7 = (trimdata[19]<<8) | trimdata[18];
    bme->dig_P8 = (trimdata[21]<<8) | trimdata[20];
    bme->dig_P9 = (trimdata[23]<<8) | trimdata[22];
    bme->dig_H1 = trimdata[24];
    bme->dig_H2 = (trimdata[26]<<8) | trimdata[25];
    bme->dig_H3 = (trimdata[27]);
    bme->dig_H4 = (trimdata[28]<<4) | (trimdata[29] & 0x0f);
    bme->dig_H5 = (trimdata[30]<<4) | (trimdata[29]>>4);
    bme->dig_H6 = (trimdata[31]);
}

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

int BME280_Config (BME280_Data *bme, uint8_t osrs_t, uint8_t osrs_p, uint8_t osrs_h, 
                  uint8_t mode, uint8_t t_sb, uint8_t filter)
{
    if (bme == NULL) {
        return -1;
    }
	// Read the Trimming parameters
	TrimRead(bme);


	uint8_t datatowrite = 0;
	uint8_t datacheck = 0;

	// Reset the device
	datatowrite = 0xB6;  // reset sequence
	if (HAL_I2C_Mem_Write(bme->i2c_handle, bme->dev_address, RESET_REG, 1, 
                          &datatowrite, 1, 1000) != HAL_OK)
	{
		return -1;
	}

	HAL_Delay(100);
    
    // Write the humidity oversampling to 0xF2
    datatowrite = osrs_h;
    if (HAL_I2C_Mem_Write(bme->i2c_handle, bme->dev_address, CTRL_HUM_REG, 1, 
                          &datatowrite, 1, 1000) != HAL_OK) {
        return -1;
    }
    HAL_Delay(100);
    HAL_I2C_Mem_Read(bme->i2c_handle, bme->dev_address, CTRL_HUM_REG, 1, &datacheck, 1, 1000);
    if (datacheck != datatowrite) {
        return -1;
    }
    
    // Write the standby time and IIR filter coeff to 0xF5
    datatowrite = (t_sb << 5) | (filter << 2);
    if (HAL_I2C_Mem_Write(bme->i2c_handle, bme->dev_address, CONFIG_REG, 1, 
                          &datatowrite, 1, 1000) != HAL_OK) {
        return -1;
    }
    HAL_Delay(100);
    HAL_I2C_Mem_Read(bme->i2c_handle, bme->dev_address, CONFIG_REG, 1, &datacheck, 1, 1000);
    if (datacheck != datatowrite) {
        return -1;
    }
    
    // Write the pressure and temp oversampling along with mode to 0xF4
    datatowrite = (osrs_t << 5) | (osrs_p << 2) | mode;
    if (HAL_I2C_Mem_Write(bme->i2c_handle, bme->dev_address, CTRL_MEAS_REG, 1, 
                          &datatowrite, 1, 1000) != HAL_OK) {
        return -1;
    }
    HAL_Delay(100);
    HAL_I2C_Mem_Read(bme->i2c_handle, bme->dev_address, CTRL_MEAS_REG, 1, &datacheck, 1, 1000);
    if (datacheck != datatowrite) {
        return -1;
    }
    
    return 0;
}


int BMEReadRaw(BME280_Data *bme)
{
	uint8_t RawData[8];

	// Check the chip ID before reading
	HAL_I2C_Mem_Read(bme->i2c_handle, bme->dev_address, ID_REG, 1, &bme->chip_id, 1, 1000);
    
    if (bme->chip_id == 0x60) {
        // Read the Registers 0xF7 to 0xFE
        HAL_I2C_Mem_Read(bme->i2c_handle, bme->dev_address, PRESS_MSB_REG, 1, 
                         RawData, 8, HAL_MAX_DELAY);
        
        /* Calculate the Raw data for the parameters
         * Here the Pressure and Temperature are in 20 bit format and humidity in 16 bit format
         */
        bme->pRaw = (RawData[0]<<12) | (RawData[1]<<4) | (RawData[2]>>4);
        bme->tRaw = (RawData[3]<<12) | (RawData[4]<<4) | (RawData[5]>>4);
        bme->hRaw = (RawData[6]<<8) | (RawData[7]);
        
        return 0;
    }
    
    return -1;
}

/* To be used when doing the force measurement
 * the Device need to be put in forced mode every time the measurement is needed
 */
void BME280_WakeUp(BME280_Data *bme)
{
    if (bme == NULL) {
        return;
    }
    
    uint8_t datatowrite = 0;
    
    // First read the register
    HAL_I2C_Mem_Read(bme->i2c_handle, bme->dev_address, CTRL_MEAS_REG, 1, &datatowrite, 1, 1000);
    
    // Modify the data with the forced mode
    datatowrite = datatowrite | MODE_FORCED;
    
    // Write the new data to the register
    HAL_I2C_Mem_Write(bme->i2c_handle, bme->dev_address, CTRL_MEAS_REG, 1, &datatowrite, 1, 1000);
    
    HAL_Delay(100);
}

/************* COMPENSATION CALCULATION AS PER DATASHEET (page 25) **************************/

/* Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
   t_fine carries fine temperature as global value
*/
int32_t t_fine;
int32_t BME280_CompensateT_int32(BME280_Data *bme, int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T>>3) - ((int32_t)bme->dig_T1<<1))) * ((int32_t)bme->dig_T2)) >> 11;
    var2 = (((((adc_T>>4) - ((int32_t)bme->dig_T1)) * ((adc_T>>4) - ((int32_t)bme->dig_T1)))>> 12) *
            ((int32_t)bme->dig_T3)) >> 14;
    bme->t_fine = var1 + var2;
    T = (bme->t_fine * 5 + 128) >> 8;
    return T;
}

#if SUPPORT_64BIT
/**
 * @brief Pressure compensation calculation (64-bit)
 * @param bme Pointer to BME280_Data structure
 * @param adc_P Raw pressure data
 * @return Compensated pressure in Q24.8 format
 */
uint32_t BME280_CompensateP_int64(BME280_Data *bme, int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)bme->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bme->dig_P6;
    var2 = var2 + ((var1*(int64_t)bme->dig_P5)<<17);
    var2 = var2 + (((int64_t)bme->dig_P4)<<35);
    var1 = ((var1 * var1 * (int64_t)bme->dig_P3)>>8) + ((var1 * (int64_t)bme->dig_P2)<<12);
    var1 = (((((int64_t)1)<<47)+var1))*((int64_t)bme->dig_P1)>>33;
    if (var1 == 0) {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576-adc_P;
    p = (((p<<31)-var2)*3125)/var1;
    var1 = (((int64_t)bme->dig_P9) * (p>>13) * (p>>13)) >> 25;
    var2 = (((int64_t)bme->dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)bme->dig_P7)<<4);
    return (uint32_t)p;
}

#elif SUPPORT_32BIT
/**
 * @brief Pressure compensation calculation (32-bit)
 * @param bme Pointer to BME280_Data structure
 * @param adc_P Raw pressure data
 * @return Compensated pressure in Pa
 */
uint32_t BME280_CompensateP_int32(BME280_Data *bme, int32_t adc_P)
{
    int32_t var1, var2;
    uint32_t p;
    var1 = (((int32_t)bme->t_fine)>>1) - (int32_t)64000;
    var2 = (((var1>>2) * (var1>>2)) >> 11 ) * ((int32_t)bme->dig_P6);
    var2 = var2 + ((var1*((int32_t)bme->dig_P5))<<1);
    var2 = (var2>>2)+(((int32_t)bme->dig_P4)<<16);
    var1 = (((bme->dig_P3 * (((var1>>2) * (var1>>2)) >> 13 )) >> 3) + 
            ((((int32_t)bme->dig_P2) *var1)>>1))>>18;
    var1 =((((32768+var1))*((int32_t)bme->dig_P1))>>15);
    if (var1 == 0) {
        return 0; // avoid exception caused by division by zero
    }
    p = (((uint32_t)(((int32_t)1048576)-adc_P)-(var2>>12)))*3125;
    if (p < 0x80000000) {
        p = (p << 1) / ((uint32_t)var1);
    } else {
        p = (p / (uint32_t)var1) * 2;
    }
    var1 = (((int32_t)bme->dig_P9) * ((int32_t)(((p>>3) * (p>>3))>>13)))>>12;
    var2 = (((int32_t)(p>>2)) * ((int32_t)bme->dig_P8))>>13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + bme->dig_P7) >> 4));
    return p;
}
#endif

/**
 * @brief Humidity compensation calculation
 * @param bme Pointer to BME280_Data structure
 * @param adc_H Raw humidity data
 * @return Compensated humidity in Q22.10 format
 */
uint32_t BME280_CompensateH_int32(BME280_Data *bme, int32_t adc_H)
{
    int32_t v_x1_u32r;
    v_x1_u32r = (bme->t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)bme->dig_H4) << 20) - 
            (((int32_t)bme->dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * 
            (((((((v_x1_u32r * ((int32_t)bme->dig_H6)) >> 10) * 
            (((v_x1_u32r * ((int32_t)bme->dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + 
            ((int32_t)2097152)) * ((int32_t)bme->dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * 
            ((int32_t)bme->dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (uint32_t)(v_x1_u32r>>12);
}

/*********************************************************************************************************/


/* measure the temp, pressure and humidity
 * the values will be stored in the parameters passed to the function
 */
void BME280_Measure (BME280_Data *bme)
{
    if (bme == NULL) {
        return;
    }
    
    if (BME280_ReadRaw(bme) == 0) {
        // Temperature measurement
        if (bme->tRaw == 0x800000) {
            bme->Temperature = 0; // value in case temp measurement was disabled
        } else {
            bme->Temperature = (BME280_CompensateT_int32(bme, bme->tRaw))/100.0f;  // as per datasheet, the temp is x100
        }
        
        // Pressure measurement
        if (bme->pRaw == 0x800000) {
            bme->Pressure = 0; // value in case pressure measurement was disabled
        } else {
#if SUPPORT_64BIT
            bme->Pressure = (BME280_CompensateP_int64(bme, bme->pRaw))/256.0f;  // as per datasheet, the pressure is x256
#elif SUPPORT_32BIT
            bme->Pressure = (BME280_CompensateP_int32(bme, bme->pRaw));  // as per datasheet, the pressure is Pa
#endif
        }
        
        // Humidity measurement
        if (bme->hRaw == 0x8000) {
            bme->Humidity = 0; // value in case humidity measurement was disabled
        } else {
            bme->Humidity = (BME280_CompensateH_int32(bme, bme->hRaw))/1024.0f;  // as per datasheet, the humidity is x1024
        }
    } else {
        // If the device is detached
        bme->Temperature = bme->Pressure = bme->Humidity = 0;
    }
}