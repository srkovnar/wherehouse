// ====================================================================
// This is the code used to set up new ESP8266 chips.
//
// This must be flashed and run on the ESP8266 BEFORE the actual code,
// because the 1MB ESP8266 ESP-01s variants do not like it when you
// try to read from EEPROM before writing. If you do that, it generates
// Exception 9 (Unaligned address).
//
// As of Tuesday, April 18, esp8266_release.ino is out of date, because
// I changed the memory structure slightly. This will need to be
// updated eventually. esp8266_release_1mb.ino is up to date with the
// memory structure set up in this code.
//
// OPERATION: This code should run, initialize the needed EEPROM
// memory locations, and then attempt to read from them. The code will
// output whether setup was successful over serial UART lines.
// If successful, you can safely re-flash the ESP8266 with the full
// release code.
// ====================================================================

#include <Arduino.h>
#include <WiFiClient.h>
#include <Hash.h>
#include <EEPROM.h>

#include "communication.h"

#define FAIL_VALUE "bad_string"

const int initial_device_id = 3; 
// ^ Set this to the device ID you want loaded on the chip.
// This can be any value except 0, which is reserved.

void setup() {
  Serial.begin(115200);
  delay(3000);

  EEPROM.begin(128);
  delay(500);

  struct {
    char m_ssid[MAX_SSID_LENGTH+1] = "new_ssid";
    char m_pw[MAX_PASSWORD_LENGTH+1] = "new_password";
    uint32_t m_ip = 0;
    uint32_t m_device_id = initial_device_id;
  } wifi_data;
  // Total byte size = 33 + 33 + 4 + 4 = 74 Bytes

  EEPROM.put(EEPROM_START_ADDRESS, wifi_data);
  EEPROM.commit();

  delay(2000);

  strcpy(wifi_data.m_ssid, FAIL_VALUE);
  strcpy(wifi_data.m_pw, FAIL_VALUE);
  wifi_data.m_ip = 0xFF;
  wifi_data.m_device_id = 0;
  delay(500);

  EEPROM.get(EEPROM_START_ADDRESS, wifi_data);
  Serial.println("Testing EEPROM storage...");
  
  Serial.print("\tSSID: ");
  if (strcmp(wifi_data.m_ssid, FAIL_VALUE) == 0) {
    Serial.println("Failed. Recovered ");
  }
  else {
    Serial.print("Success! Recovered ");
  }
  Serial.println(wifi_data.m_ssid);


  Serial.print("\tPassword: ");
  if (strcmp(wifi_data.m_pw, FAIL_VALUE) == 0) {
    Serial.println("Failed. Recovered ");
  }
  else {
    Serial.print("Success! Recovered ");
  }
  Serial.println(wifi_data.m_pw);


  Serial.print("\tIP: ");
  if (wifi_data.m_ip != 0) {
    Serial.print("Failed. Recovered ");
  }
  else {
    Serial.print("Success! Recovered ");
  }
  Serial.println(wifi_data.m_ip);

  Serial.print("\tDevice ID: ");
  if (wifi_data.m_device_id == initial_device_id) {
    Serial.print("Success! Recovered ");
  }
  else {
    Serial.print("Failed. Recovered ");
  }
  Serial.println(wifi_data.m_device_id);

  Serial.println("Tests complete.");
}

void loop() {
  // put your main code here, to run repeatedly:

}
