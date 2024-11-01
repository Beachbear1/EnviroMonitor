#ifndef HM_10_H
#define HM_10_H

#include "stm32l4xx_hal.h"  // STM32 HAL library

#define HM_10_BLE_BAUDRATE 9600
#define HM_10_MESSAGE_DELIMITER '!'

void HM_10_Init(UART_HandleTypeDef *huart);
void HM_10_SendMessage(char *msg);
void HM_10_Process(void);
void HM_10_ReceiveHandler(UART_HandleTypeDef *huart);

#endif /* HM_10_H */
