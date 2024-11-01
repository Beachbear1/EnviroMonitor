#include "hm_10.h"
#include <string.h>

UART_HandleTypeDef *_huart_hm10;
char message_buffer[128];
uint8_t message_index = 0;

void HM_10_Init(UART_HandleTypeDef *huart) {
    _huart_hm10 = huart;
    HAL_UART_Receive_IT(_huart_hm10, (uint8_t *)message_buffer, 1);  // Start receiving
}

void HM_10_SendMessage(char *msg) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s%c", msg, HM_10_MESSAGE_DELIMITER);
    HAL_UART_Transmit(_huart_hm10, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
}

void HM_10_Process(void) {

}

void HM_10_ReceiveHandler(UART_HandleTypeDef *huart) {
    if (huart->Instance == _huart_hm10->Instance) {
        if (message_buffer[message_index] == HM_10_MESSAGE_DELIMITER) {
            message_buffer[message_index] = '\0';  // Null-terminate the message
            HM_10_SendMessage("Received");         // Echo received message (for testing)
            message_index = 0;
        } else {
            message_index++;
            if (message_index >= sizeof(message_buffer)) {
                message_index = 0;  // Reset if the message is too long
            }
        }

        HAL_UART_Receive_IT(_huart_hm10, (uint8_t *)message_buffer + message_index, 1);  // Continue receiving
    }
}
