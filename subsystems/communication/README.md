# Wherehouse: Communication Subsystem

Author(s): Sam Kovnar

## About this repository

This repository contains the code used to test the Communications
subsystem of the Wherehouse, Team 23's Spring 2023 ECE senior design
project.

The Communication subsystem is responsible for interactions between the STM32F0 
microcontroller and ESP8266 Wi-Fi module.

## TL;DR for TAs and Instructors

The code you're looking for is at `subsystems/communication/esp8266_firmware/esp8266_release/esp8266_release.ino`. All of the other files in `subsystems/communication/esp8266_firmware` are used for testing. If you want to see my code, that's where you should look.

## Hardware Dependencies

The code for ESP8266 devices has been tested on Ai-Thinker's ESP8266 ESP-01 modules.

## Software Dependencies

### General

Operating System: Ubuntu 22.04 LTS
- Code functionality on other operating systems can be neither confirmed nor denied

Arduino IDE v2.0.0 or later
- This version has been tested on Arduino v2.0.3
- Certain libraries do not work on versions earlier than v2.0.0 for Linux

### ESP8266 Core for Arduino IDE

ESP8266 Core for Arduino, *version 3.1.1*
- ESP8266WiFi.h
- ESP8266HTTPClient.h
- ESP8266WiFiMulti.h

Multiple libraries used in the code contained in this project utilize the ESP8266
core for Arduino IDE. You must make sure that it is installed.

1.  Open Arduino IDE
2.  Navigate to File > Preferences
3.  Find the textbox labeled "Additional Board Manager URLs"
4.  Insert the following link into the box: http://arduino.esp8266.com/stable/package_esp8266com_index.json, then click "OK".
  - You can have multiple URLs in the box. Just separate them with commas.
5.  Navigate to "Boards Manager"
6.  Search for "esp8266"
7.  Install the board labeled "esp8266 by ESP8266 Community"

### Additional Libraries

ESPAsyncWebServer.h
- https://github.com/me-no-dev/ESPAsyncWebServer

ESPAsyncTCP.h
- https://github.com/me-no-dev/ESPAsyncTCP

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

### ESP8266 Firmware
**esp8266_release**: Firmware to be used on device
- This is the code being used in the *midterm demo*!


