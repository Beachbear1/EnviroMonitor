# Environment Sensor Project (WeatherDesignProject & WeatherBT)

This repository contains the code for a weather station project using STM32 microcontroller (Nucleo-L432KC) and a mobile Bluetooth interface for receiving weather data.

## What is this repository for?

The project reads temperature, humidity, and pressure data using sensors connected to the STM32 board and displays it on an OLED screen. The data is also transmitted via Bluetooth to a mobile application created using Capacitor.

### WeatherDesignProject

#### Requirements
- **STM32CubeIDE**: Development environment for STM32 microcontrollers.
- **Hardware Components**:
  - [Nucleo-L432KC Microcontroller](https://www.st.com/en/evaluation-tools/nucleo-l432kc.html)
  - AHT20 Temperature/Humidity Sensor
  - BMP280 Atmospheric Pressure Sensor
  - SSD1306 OLED Display
  - HM-10 Bluetooth Module

### WeatherBT (Mobile Bluetooth App)

#### Requirements
- **Capacitor**: A cross-platform tool for building mobile apps with JavaScript.
- **@capacitor-community/bluetooth-le**: Library for Bluetooth Low Energy (BLE) communication.
