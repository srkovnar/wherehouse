# Wherehouse: Communication Subsystem

Author(s): Sam Kovnar

## About this repository

This repository contains the code used to test the Communications
subsystem of the Wherehouse, Team 23's Spring 2023 ECE senior design
project.

The Communication subsystem is responsible for interactions between the STM32F0 
microcontroller and ESP8266 Wi-Fi module.

## Hardware Dependencies

The code for ESP8266 devices has been tested on Ai-Thinker's ESP8266 ESP-01 modules.

## Software Dependencies

Arduino IDE v2.0.0 or later
- This version has been tested on Arduino v2.0.3
- Certain libraries do not work on versions earlier than v2.0.0 for Linux

ESP8266 Core for Arduino
- ESP8266WiFi.h
- ESP8266HTTPClient.h
- ESP8266WiFiMulti.h

WiFiClient.h

ESPAsyncWebServer.h

## Repository Contents

All code for the ESP8266 is written in Arduino format (.ino).
ESP8266 code files begin with 'esp8266_' followed by the name of the
specific project.

Due to the nature of the Arduino IDE, all code files must be kept in
their own folder which shares the project name with the '.ino' file.

### ESP8266 Testing Stage 1: Communications

**esp8266_ap_test**: Connection test for ESP8266 acting as Access Point

**esp8266_sta_test**: Connection test for ESP8266 acting as Station

The above two code files are a simple test of the ESP8266 WiFi libraries.
The Access Point device should produce serial output that a WiFi access point
was started successfully. Once the Access Point is created, the Station device
should be able to connect and display connection status over Serial lines.

This code was last tested 2023-02-14.

**esp8266_ap_http_test**: HTTP transfer test for ESP8266 acting as Access Point

**esp8266_sta_http_test**: HTTP transfer test for ESP8266 acting as Station

The above two code files are based on a tutorial by RandomNerdTutorials
at https://randomnerdtutorials.com/esp8266-nodemcu-client-server-wi-fi/.
Each should be loaded onto an ESP8266. When both ESP devices are active
(i.e. powered on and running in operation mode), the Station device should
be able to receive data from the Access Point device by requesting from
the /data URL.

This code was last tested 2023-02-16.

### ESP8266 Testing Stage 2: Functionality
**esp8266_serial_test**: Code for testing Serial UART communication to and from the ESP8266
- Includes a handler for interpreting UART commands
- Serial commands can be sent from the host computer to the ESP8266 via
the Arduino IDE serial monitor
  - Baud rate: 115200
  - In this way, the computer can mimic the behavior of the STM32 which will be
    managing data in the final project
    
**esp8266_sta_router_test**: Test interactions between ESP8266 station and web server via router


