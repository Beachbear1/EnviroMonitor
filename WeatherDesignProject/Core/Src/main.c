/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "ssd1306.h"
#include <stdarg.h>
#include "aht20.h"
#include "bmp280.h"
#include "hm_10.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN 0 */

// Retarget printf to USART2
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

//debug info to UART2
int send_to_uart2(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
    return 0;
}

//sensor data to Bluetooth
int send_to_bluetooth(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    HM_10_SendMessage(buffer);
    return 0;
}

/* USER CODE END 0 */

/* Function declarations -----------------------------------------------------*/
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_I2C1_Init(void);
void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void I2C_Scanner(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();  //Bluetooth UART
    MX_USART2_UART_Init();  //Debug UART

    ssd1306_Init();  // OLED Display
    AHT20_Init(&hi2c1);  // AHT20 sensor
    HM_10_Init(&huart1);  //Bluetooth module

    uint8_t bmp280_initialized = 0;  //check if initialized

    // Initial BMP280 initialization
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(0x76 << 1), 1, 10) == HAL_OK) {
        if (!bmp280_init(&hi2c1)) {
            send_to_uart2("BMP280 initialization failed.\r\n");
        } else {
            bmp280_initialized = 1;
            send_to_uart2("BMP280 initialized successfully.\r\n");
        }
    }

    // I2C scanner
    I2C_Scanner(); // Scan I2C bus

    char text[32] = {0};

    while (1)
    {
        float temperature = -1.0f, humidity = -1.0f, pressure = -1.0f;

        // Read temp and humidity from AHT20
        temperature = AHT20_ReadTemperature();
        humidity = AHT20_ReadHumidity();

        //reinitialize BMP280 if necessary if disconnect
        if (!bmp280_initialized) {
            if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(0x76 << 1), 1, 10) == HAL_OK) {
                if (!bmp280_init(&hi2c1)) {
                    send_to_uart2("BMP280 re-initialization failed.\r\n");
                } else {
                    bmp280_initialized = 1;
                    send_to_uart2("BMP280 re-initialized successfully.\r\n");
                }
            }
        }

        // Read BMP280 pressure
        if (bmp280_initialized) {
            bmp280_measure();
            pressure = bmp280_getpressure() / 100.0;  // Convert to hPa
        }

        // Convert temp to Fahrenheit
        float temperature_f = (temperature * 9.0f / 5.0f) + 32.0f;

        // UART2 debug output to virtual terminal
        send_to_uart2("===== SENSOR READINGS =====\r\n");
        if (temperature != -999.0f && humidity != -999.0f) {
            send_to_uart2("AHT20: Temperature: %.2f F\r\n", temperature_f);
            send_to_uart2("AHT20: Humidity: %.2f %%\r\n", humidity);
        } else {
            send_to_uart2("AHT20: Not detected or failed.\r\n");
            temperature = -1.0f;
            humidity = -1.0f;
        }

        if (bmp280_initialized && pressure != -999.0f) {
            send_to_uart2("BMP280: Pressure: %.2f hPa\r\n", pressure);
        } else {
            send_to_uart2("BMP280: Not detected or failed.\r\n");
            pressure = -1.0f;
        }
        send_to_uart2("===========================\r\n\r\n");

        // Update OLED display
        ssd1306_Fill(Black);
        ssd1306_SetCursor(1, 1);
        ssd1306_WriteString("Weather Station", Font_6x8, White);

        // Display temp and humidity on OLED
        if (temperature != -1.0f && humidity != -1.0f) {
            snprintf(text, sizeof(text), "Temp: %.1f F", temperature_f);  // Fahrenheit
            ssd1306_SetCursor(1, 15);
            ssd1306_WriteString(text, Font_6x8, White);

            snprintf(text, sizeof(text), "Humidity: %.2f %%", humidity);
            ssd1306_SetCursor(1, 30);
            ssd1306_WriteString(text, Font_6x8, White);
        } else {
            ssd1306_SetCursor(1, 15);
            ssd1306_WriteString("AHT20: N/A", Font_6x8, White);
        }

        // Display pressure on OLED
        if (pressure != -1.0f) {
            snprintf(text, sizeof(text), "Pres: %.2f hPa", pressure);
        } else {
            snprintf(text, sizeof(text), "Pres: N/A");
        }
        ssd1306_SetCursor(1, 45);
        ssd1306_WriteString(text, Font_6x8, White);

        // Update the OLED screen
        ssd1306_UpdateScreen();

        // Send only the simplified sensor data over Bluetooth
        if (temperature != -999.0f && humidity != -999.0f && pressure != -999.0f) {
            char bluetoothMessage[128];
            // Format sensor data for Bluetooth transmission
            snprintf(bluetoothMessage, sizeof(bluetoothMessage), "Temp:%.2f,Humidity:%.2f,Pressure:%.2f", temperature_f, humidity, pressure);
            send_to_bluetooth(bluetoothMessage);  // Send the structured data via Bluetooth only
        }


        HAL_Delay(2000);
    }
}

// UART receive complete callback for HM-10
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == huart1.Instance) {
        HM_10_ReceiveHandler(huart);  // Process Bluetooth messages
    }
}

void I2C_Scanner(void) {
    send_to_uart2("===== I2C SCAN START =====\r\n");
    for(uint8_t i = 1; i < 128; i++) {
        if(HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i << 1), 1, 10) == HAL_OK) {
            send_to_uart2("I2C: Device found at 0x%02X\r\n", i);
        }
    }
    send_to_uart2("===== I2C SCAN END =====\r\n\r\n");
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00707CBB;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function (for HM-10 Bluetooth)
  * @param None
  * @retval None
  */
void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LD3_Pin */
  GPIO_InitStruct.Pin = LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD3_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
