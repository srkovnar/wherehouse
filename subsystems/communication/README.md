# Wherehouse: Communication Subsystem

Author(s): Sam Kovnar

## Table of Contents

1. [TL;DR for TAs and Instructors](#tldr)
2. [About this folder](#about)
3. [Hardware Dependencies](#hardware)
4. [Software Dependencies](#software)
    1. [General](#sw_general)
    2. [ESP8266 Core for Arduino IDE](#esp8266core)
    3. [Additional Firmware Libraries](#fw_libraries)
    4. [KiCad Libraries](#kicad_libraries)
5. [ESP8266 Firmware](#esp_firmware)
    1. [Testing Phase 1](#esp_fw_t1)
    2. [Testing Phase 2](#esp_fw_t2)
    3. [Release Firmware](#esp_fw_release)
6. [Schematics](#schematics)
7. [Documentation](#documentation)

## TL;DR for TAs and Instructors <a name="tldr"></a>

The code you're looking for is at `subsystems/communication/esp8266_firmware/esp8266_release/esp8266_release.ino`. All of the other files in `subsystems/communication/esp8266_firmware` are used for testing. If you want to see my code, that's where you should look.

## About this folder <a name="about"></a>

This folder (`wherehouse/subsystems/communication`) contains the code used to test the Communications
subsystem of the Wherehouse, Team 23's Spring 2023 ECE senior design
project.

The Communication subsystem is responsible for interactions between the STM32F0 
microcontroller and ESP8266 Wi-Fi module.

## Hardware Dependencies <a name="hardware"></a>

The code for ESP8266 devices has been tested on Ai-Thinker's ESP8266 ESP-01 modules.

For flashing and testing the ESP8266 ESP-01 modules, I recommend using a ESP-01 USB
adapter. You can find these from multiple different vendors. I found mine on eBay.

## Software Dependencies <a name="software"></a>

### General <a name="sw_general"></a>

Operating System: Ubuntu 22.04 LTS
- Code functionality on other operating systems can be neither confirmed nor denied

Arduino IDE *(version 2.0.0 or later)*
- This version has been tested on Arduino v2.0.3
- Certain libraries do not work on versions earlier than v2.0.0 for Linux

KiCad *(version 6.0)*

### ESP8266 Core for Arduino IDE <a name="esp8266core"></a>

ESP8266 Core for Arduino *(version 3.1.1)*
- ESP8266WiFi.h
- ESP8266HTTPClient.h
- ESP8266WiFiMulti.h

Multiple libraries used in the code contained in this project utilize the ESP8266
Core for Arduino IDE. You must make sure that it is installed.

1.  Open Arduino IDE
2.  Navigate to File > Preferences
3.  Find the textbox labeled "Additional Board Manager URLs"
4.  Insert the following link into the box: http://arduino.esp8266.com/stable/package_esp8266com_index.json, then click "OK".
    - You can have multiple URLs in the box. Just separate them with commas.
5.  Navigate to "Boards Manager"
6.  Search for "esp8266"
7.  Install the board labeled "esp8266 by ESP8266 Community"

### Arduino IDE Libraries <a name="fw_libraries"></a>

ESPAsyncWebServer.h (https://github.com/me-no-dev/ESPAsyncWebServer)
- Used by ESP8266 firmware and test code

ESPAsyncTCP.h (https://github.com/me-no-dev/ESPAsyncTCP)
- Required for ESPAsyncWebServer.h to work

To install an Arduino library:
1. Download the library from Github.
    1. Open the Github link to the library in a web browser
    2. Click on the button that says "<> Code". This will bring up a menu.
    3. In the menu that appears, click on "Download ZIP". This will download a .zip folder containing all of the files needed for the library to work.
    4. Keep track of where this .zip file is. It will probably be in your "Downloads" folder.
2. Open Arduino IDE
3. Add the library to your Arduino libraries.
    1. In the toolbar at the top of the Arduino IDE window, click "Sketch" to bring up a drop-down menu.
    2. Hover over "Include Library", then click on "Add .ZIP Library...". A window will appear showing your files.
    3. In the window that just appeared, find the .zip file that you downloaded in step 1. It is most likely in your downloads folder with a name that matches the name of the library (For example, the .zip for ESPAsyncWebServer.h is called `ESPAsyncWebServer-Master.zip`).
    4. Click on the .zip file, then click the "Open" button. This will put the library in your Arduino libraries folder (usually `~/Arduino/libraries` or something similar).
    5. You can delete the .zip file now. You don't need it anymore.
4. The library is now available for you to use, and can be targeted by adding `#include <LIBRARY_NAME.h>` to your code, where "LIBRARY_NAME" is replaced with the name of your library, for example, ESPAsyncWebServer.h.

### KiCad Libraries <a name="kicad_libraries"></a>

kicad-ESP8266 (https://github.com/jdunmire/kicad-ESP8266)
- Contains schematic and PCB footprints for most commonly used ESP8266 modules, including the ESP-01 which is being used in this project.

To install a KiCad library:
1. Download files from the Github link listed above
2. Open KiCad
3. In the toolbar, go to "Preferences", then click on "Manage Symbol Libraries". A window will appear.
4. In the window that just appeared, click on "Global Libraries" near the top.
5. Click on the Folder icon near the bottom.
6. In the file browser that appears, find the file `kicad-ESP8266/ESP8266.lib`. Click on it, then click "Open".
7. Name it "ESP8266", then click "OK".
8. Repeat steps 3 through 7, but go to "Manage Footprint Libraries" instead of "Manage Symbol Libraries".

## Firmware <a name="esp_firmware"></a>

All code for the ESP8266 is written in Arduino format (.ino).
ESP8266 code files begin with 'esp8266_' followed by the name of the
specific project.

Due to the nature of the Arduino IDE, all code files must be kept in
their own folder which shares the project name with the '.ino' file.

### ESP8266 Testing Stage 1: Communications <a name="esp_fw_t1"></a>

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

### ESP8266 Testing Stage 2: Functionality <a name="esp_fw_t2"></a>

**esp8266_serial_test**: Code for testing Serial UART communication to and from the ESP8266
- Includes a handler for interpreting UART commands
- Serial commands can be sent from the host computer to the ESP8266 via
the Arduino IDE serial monitor
  - Baud rate: 115200
  - In this way, the computer can mimic the behavior of the STM32 which will be
    managing data in the final project
    
**esp8266_sta_router_test**: Test interactions between ESP8266 station and web server via router

### Release Firmware <a name="esp_fw_release"></a>

**esp8266_release**: Firmware to be used on device
- *This is the code being used in the midterm demo*!

## Schematics <a name="schematics"></a>

Schematics for the Communication subsystem are stored in the 
`wherehouse/subsystems/communication/Schematics` directory.

## Documentation <a name="documentation"></a>

This folder contains:
- Activity Diagram (`communication_activity_diagram.pdf`), which shows interactions between
microcontroller, wi-fi module, and the web server.
- Block Diagram (`communication_block_diagram.pdf`), which shows the signal connections between
the microcontroller and the wi-fi module.
- PCB Design Review Submission (`communication_pcb_design_review_submission_1.pdf`), which was submitted for the subsystem PCB design review. The schematic used was Version 2.

