/*
  Based partly on the Client code from :
  https://randomnerdtutorials.com/esp8266-nodemcu-client-server-wi-fi/

  Also used
  https://forum.arduino.cc/t/concatenate-char/376886/2
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>

ESP8266WiFiMulti WiFiMulti;

const char* ssid = "ESP8266";
const char* password = "ESP8266Test";

const char* server_name_data = "http://192.168.4.1/data";
const char* http_tag = "http://";
//const char* server_name_data = "http://";
const char* ip_addr = "192.168.4.1";
const char* data_ext = "/data";

char buf[64]; // For dynamic address creation

int i = 0;
const int STOP = 10;

String data;

unsigned long previousMillis = 0;
const long interval = 5000;

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  //IP Address with path or Domain name with URL path
  http.begin(client, serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "--";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();

  return payload;
}

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

  // Testing dynamic creating of address.
  strcat(buf, http_tag);
  strcat(buf, ip_addr);
  strcat(buf, data_ext);
  Serial.println(buf);

  /*
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
  */
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    if ((WiFiMulti.run() == WL_CONNECTED)) {
      data = httpGETRequest(buf);
      Serial.println("Data: " + data);

      //Do stuff here

      previousMillis = currentMillis;
    }
  }
}
