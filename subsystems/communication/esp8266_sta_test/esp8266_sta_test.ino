/*
  Based partly on the Client code from :
  https://randomnerdtutorials.com/esp8266-nodemcu-client-server-wi-fi/
*/

#include <ESP8266WiFi.h>
//#include <ESP8266HTTPClient.h>
//#include <WiFiClient.h>
//#include <ESP8266WiFiMulti.h>

const char* ssid = "ESP8266";
const char* password = "ESP8266Test";

int i = 0;
const int STOP = 10;

void setup() {
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  delay(1000);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected to WiFi!");

  while (i < STOP) {
    Serial.print("Waiting... (");
    Serial.print(i);
    Serial.print("/");
    Serial.print(STOP);
    Serial.println(")");
    i++;
    delay(500);
  }

  WiFi.disconnect();
  //WiFi.mode(WIFI_OFF); //This is similar to disconnect, but saves previous credentials (you can turn it back on)
  Serial.println("Disconnected");
}

void loop() {
  // put your main code here, to run repeatedly:
}
