#include <Arduino.h>
#include <WiFiClient.h>
#include <Hash.h>
#include <EEPROM.h> // Not sure if this board even HAS an eeprom... but it's worth a try.

#include "communication.h"

#define FAIL_VALUE "failed"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(3000);

  EEPROM.begin(128);
  delay(500);

  struct {
    char m_ssid[MAX_SSID_LENGTH+1] = "alpha";
    char m_pw[MAX_PASSWORD_LENGTH+1] = "beta";
    uint32_t m_ip = 0;
  } wifi_data;

  EEPROM.put(EEPROM_START_ADDRESS, wifi_data);
  EEPROM.commit();

  delay(2000);

  strcpy(wifi_data.m_ssid, FAIL_VALUE);
  strcpy(wifi_data.m_pw, FAIL_VALUE);
  wifi_data.m_ip = 0xFF;
  delay(500);

  EEPROM.get(EEPROM_START_ADDRESS, wifi_data);
  Serial.println("Testing EEPROM storage...");
  Serial.print("\tSSID: ");
  if (strcmp(wifi_data.m_ssid, FAIL_VALUE) == 0) {
    Serial.println("Failed.");
  }
  else {
    Serial.print("Success! Recovered ");
    Serial.println(wifi_data.m_ssid);
  }

  Serial.print("\tPassword: ");
  if (strcmp(wifi_data.m_pw, FAIL_VALUE) == 0) {
    Serial.println("Failed");
  }
  else {
    Serial.print("Success! Recovered ");
    Serial.println(wifi_data.m_pw);
  }

  Serial.print("\tIP: ");
  if (wifi_data.m_ip != 0) {
    Serial.print("Failed. ");
    Serial.println(wifi_data.m_ip);
  }
  else {
    Serial.print("Success! Recovered ");
    Serial.println(wifi_data.m_ip);
  }

  Serial.println("Tests complete.");
}

void loop() {
  // put your main code here, to run repeatedly:

}
