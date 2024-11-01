# WeatherDesignProject

## Overview
This repository contains code for an environmental monitoring system using the Nucleo-L432KC microcontroller, equipped with a temperature/humidity sensor and an atmospheric pressure sensor.

### Hardware Requirements

- **STM32CubeIDE**: The integrated development environment for STM32 microcontrollers, used to develop and debug the firmware.
- **Nucleo-L432KC Microcontroller**: The core microcontroller board that handles sensor data acquisition, processing, and communication.
  - [Product Page](https://www.st.com/en/evaluation-tools/nucleo-l432kc.html)
- **Temperature/Humidity Sensor (AHT20)**: Measures ambient temperature and humidity.
- **Atmospheric Pressure Sensor (BMP280)**: Measures atmospheric pressure for weather monitoring.
- **OLED Display (SSD1306)**: A display module used to show real-time sensor data directly on the device.
- **Bluetooth Module (HM-10)**: Used for wireless communication to send sensor data to an external device, such as a smartphone or computer.
- **I2C Communication**: Used to interface with the AHT20 and BMP280 sensors as well as the OLED display.
- **UART Communication**: Used for debugging via USART2 and for Bluetooth communication via USART1.
- **GPIO**: Configured to manage LEDs or other output indicators if needed.


