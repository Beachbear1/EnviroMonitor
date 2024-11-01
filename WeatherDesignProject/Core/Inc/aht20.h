#ifndef AHT20_H
#define AHT20_H

#include "stm32l4xx_hal.h"

// Function to initialize AHT20
void AHT20_Init(I2C_HandleTypeDef *hi2c);

// Functions to read temperature and humidity
float AHT20_ReadTemperature(void);
float AHT20_ReadHumidity(void);

#endif /* AHT20_H */
