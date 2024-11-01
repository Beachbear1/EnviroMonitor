#include "bmp280.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stm32l4xx_hal.h"  // STM32 HAL library for I2C communication

#define BMP280_ADDRESS (0x76 << 1)  // Default I2C address
#define BMP280_REG_CALIB 0x88
#define BMP280_REG_PRESS_MSB 0xF7
#define BMP280_REG_TEMP_MSB  0xFA
#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_CONFIG    0xF5
#define BMP280_REG_CHIPID    0xD0
#define BMP280_CHIP_ID       0x58

I2C_HandleTypeDef *_hi2c_bmp280;

// Calibration data
uint16_t dig_T1;
int16_t dig_T2, dig_T3;
uint16_t dig_P1;
int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
int32_t t_fine;

// Variables for temperature and pressure
int32_t _bmp280_temp = 0;  // Temperature in hundredths of degree Celsius
uint32_t _bmp280_pres = 0; // Pressure in Pa

// Write to BMP280 register
void bmp280_writemem(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    HAL_I2C_Master_Transmit(_hi2c_bmp280, BMP280_ADDRESS, data, 2, HAL_MAX_DELAY);
}

// Read from BMP280 register
void bmp280_readmem(uint8_t reg, uint8_t *buff, uint16_t size) {
    HAL_I2C_Mem_Read(_hi2c_bmp280, BMP280_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT, buff, size, HAL_MAX_DELAY);
}

// Read calibration data
void bmp280_getcalibration(void) {
    uint8_t calib_data[24];
    bmp280_readmem(BMP280_REG_CALIB, calib_data, 24);

    dig_T1 = (calib_data[1] << 8) | calib_data[0];
    dig_T2 = (calib_data[3] << 8) | calib_data[2];
    dig_T3 = (calib_data[5] << 8) | calib_data[4];
    dig_P1 = (calib_data[7] << 8) | calib_data[6];
    dig_P2 = (calib_data[9] << 8) | calib_data[8];
    dig_P3 = (calib_data[11] << 8) | calib_data[10];
    dig_P4 = (calib_data[13] << 8) | calib_data[12];
    dig_P5 = (calib_data[15] << 8) | calib_data[14];
    dig_P6 = (calib_data[17] << 8) | calib_data[16];
    dig_P7 = (calib_data[19] << 8) | calib_data[18];
    dig_P8 = (calib_data[21] << 8) | calib_data[20];
    dig_P9 = (calib_data[23] << 8) | calib_data[22];
}

// Initialize BMP280 sensor
uint8_t bmp280_init(I2C_HandleTypeDef *hi2c) {
    _hi2c_bmp280 = hi2c;

    uint8_t chip_id;
    bmp280_readmem(BMP280_REG_CHIPID, &chip_id, 1);
    if (chip_id != BMP280_CHIP_ID) {
        return 0;  // BMP280 not detected
    }

    bmp280_getcalibration();  // Get calibration data

    // Configure the sensor (normal mode, oversampling)
    bmp280_writemem(BMP280_REG_CTRL_MEAS, 0x57); // Normal mode, pressure and temperature oversampling x4
    bmp280_writemem(BMP280_REG_CONFIG, 0x90);    // Standby time = 62.5 ms, filter = x4

    return 1;  // BMP280 initialized successfully
}

// Measure temperature and pressure
void bmp280_measure(void) {
    uint8_t data[6];
    int64_t temp_raw, press_raw, var1, var2;
    int64_t p;

    // Read raw data
    if (HAL_I2C_IsDeviceReady(_hi2c_bmp280, BMP280_ADDRESS, 1, HAL_MAX_DELAY) != HAL_OK) {
        _bmp280_pres = -999;  // Set invalid pressure value if BMP280 is not detected
        return;
    }

    bmp280_readmem(BMP280_REG_PRESS_MSB, data, 6);

    press_raw = (int32_t)(((uint32_t)(data[0] << 16) | (data[1] << 8) | data[2]) >> 4);
    temp_raw = (int32_t)(((uint32_t)(data[3] << 16) | (data[4] << 8) | data[5]) >> 4);

    // Temperature calculation
    var1 = ((((temp_raw >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((temp_raw >> 4) - ((int32_t)dig_T1)) * ((temp_raw >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    _bmp280_temp = (t_fine * 5 + 128) >> 8;

    // Pressure calculation
    var1 = (((int64_t)t_fine) >> 1) - (int64_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int64_t)dig_P6);
    var2 = var2 + ((var1 * ((int64_t)dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int64_t)dig_P4) << 16);
    var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int64_t)dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int64_t)dig_P1)) >> 15);

    if (var1 == 0) {
        _bmp280_pres = -999;  // Set invalid pressure value to avoid division by zero
    } else {
        p = (((int64_t)1048576 - press_raw) - (var2 >> 12)) * 3125;
        if (p < 0x80000000) {
            p = (p << 1) / ((int64_t)var1);
        } else {
            p = (p / (int64_t)var1) * 2;
        }
        var1 = (((int64_t)dig_P9) * ((int64_t)((p >> 3) * (p >> 3)) >> 13)) >> 12;
        var2 = (((int64_t)(p >> 2)) * ((int64_t)dig_P8)) >> 13;
        p = (int64_t)((int64_t)p + ((var1 + var2 + dig_P7) >> 4));

        _bmp280_pres = (uint32_t)p;
    }
}

// Calculate altitude based on pressure
double bmp280_getaltitude(void) {
    return (1.0 - pow(_bmp280_pres / 101325.0, 0.1903)) * 44330.0;
}
