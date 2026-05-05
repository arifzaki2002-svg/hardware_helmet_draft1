# Smart Industrial Safety Helmet

An IoT-based safety helmet designed to monitor worker status in open-space factory and construction site environments (Note: This system is specifically calibrated for open spaces, not multi-storey buildings).

## Hardware Components
* ESP32 Microcontroller
* MPU6050 (Accelerometer/Gyroscope for fall detection)
* Neo-6M GPS Module (Currently in the testing/integration phase)

## Pin Wiring Guide
* **MPU6050:** SDA -> GPIO 21 | SCL -> GPIO 22
* **GPS Module:** TX -> GPIO 16 | RX -> GPIO 17
* SOS Button
* Override Button

## Software Requirements
* Arduino IDE
* ESP32 Board Manager installed
* Required Libraries: `<Wire.h>`, `<TinyGPS++.h>`, 
