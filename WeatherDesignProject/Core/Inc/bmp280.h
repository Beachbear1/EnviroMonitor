#ifndef BMP280_H_
#define BMP280_H_

#include <stdint.h>
#include "stm32l4xx_hal.h"  // Include STM32 HAL for I2C

// BMP280 initialization
uint8_t bmp280_init(I2C_HandleTypeDef *hi2c);  // Initialize BMP280

// Perform a measurement
void bmp280_measure(void);

// Get the last measured pressure and temperature
#define bmp280_getpressure()    (_bmp280_pres)
#define bmp280_gettemperature() (_bmp280_temp)

// Calculate altitude based on the pressure
double bmp280_getaltitude(void);

// External variables for temperature and pressure
extern int32_t _bmp280_temp;   // Temperature in hundredths of a degree Celsius
extern uint32_t _bmp280_pres;  // Pressure in Pascals

#endif /* BMP280_H_ */
