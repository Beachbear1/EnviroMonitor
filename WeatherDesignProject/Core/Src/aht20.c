#include "aht20.h"
#include "stm32l4xx_hal.h"

#define AHT20_ADDRESS 0x38 << 1
#define AHT20_INIT_CMD 0xBE
#define AHT20_MEASURE_CMD 0xAC

I2C_HandleTypeDef *_hi2c;

void AHT20_Init(I2C_HandleTypeDef *hi2c) {
    _hi2c = hi2c;
    uint8_t cmd[3] = { AHT20_INIT_CMD, 0x08, 0x00 };
    HAL_I2C_Master_Transmit(_hi2c, AHT20_ADDRESS, cmd, 3, HAL_MAX_DELAY);
    HAL_Delay(40);  // Wait for sensor initialization
}

float AHT20_ReadTemperature(void) {
    uint8_t cmd[3] = { AHT20_MEASURE_CMD, 0x33, 0x00 };
    uint8_t data[6];

    // Check if AHT20 is connected
    if (HAL_I2C_IsDeviceReady(_hi2c, AHT20_ADDRESS, 1, HAL_MAX_DELAY) != HAL_OK) {
        return -999.0f;  // Return a specific error value
    }

    HAL_I2C_Master_Transmit(_hi2c, AHT20_ADDRESS, cmd, 3, HAL_MAX_DELAY);
    HAL_Delay(80);  // Wait for measurement to complete
    HAL_I2C_Master_Receive(_hi2c, AHT20_ADDRESS, data, 6, HAL_MAX_DELAY);

    uint32_t temp_raw = (data[3] & 0x0F) << 16 | data[4] << 8 | data[5];
    return ((float)temp_raw / 1048576.0f) * 200.0f - 50.0f;
}

float AHT20_ReadHumidity(void) {
    uint8_t cmd[3] = { AHT20_MEASURE_CMD, 0x33, 0x00 };
    uint8_t data[6];

    // Check if AHT20 is connected
    if (HAL_I2C_IsDeviceReady(_hi2c, AHT20_ADDRESS, 1, HAL_MAX_DELAY) != HAL_OK) {
        return -999.0f;  // Return a specific error value
    }

    HAL_I2C_Master_Transmit(_hi2c, AHT20_ADDRESS, cmd, 3, HAL_MAX_DELAY);
    HAL_Delay(80);  // Wait for measurement to complete
    HAL_I2C_Master_Receive(_hi2c, AHT20_ADDRESS, data, 6, HAL_MAX_DELAY);

    uint32_t hum_raw = data[1] << 12 | data[2] << 4 | (data[3] >> 4);
    return ((float)hum_raw / 1048576.0f) * 100.0f;
}
