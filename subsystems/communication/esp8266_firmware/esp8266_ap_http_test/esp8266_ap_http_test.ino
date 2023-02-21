/*  Based on tutorial here:
    https://randomnerdtutorials.com/esp8266-nodemcu-client-server-wi-fi/

    This project uses 3rd party libraries not included in the ESP8266 core.
    I'll try doing this with the official libraries later.

    Libraries needed are here:
    https://github.com/me-no-dev/ESPAsyncTCP (not used, but needed for ESPAsyncWebServer)
    https://github.com/me-no-dev/ESPAsyncWebServer
*/

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

/* Your WiFi Soft Access Point settings */
const char* ssid = "ESP8266";          //this will be the network name
const char* password = "ESP8266Test";  //this will be the network password

AsyncWebServer server(80);

String readData() {
  return String("Here's your data!");
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.print("Configuring WiFi access point...");
  
  /* You can remove the password parameter if you want the AP to be open. */
  boolean result = WiFi.softAP(ssid, password);
  if(result==true) {
    IPAddress myIP = WiFi.softAPIP();
  
    Serial.println("done!");
    Serial.println("");
    Serial.print("WiFi network name: ");
    Serial.println(ssid);
    Serial.print("WiFi network password: ");
    Serial.println(password);
    Serial.print("Host IP Address: ");
    Serial.println(myIP);
    Serial.println("");
  }

  else {
    Serial.println("error! Something went wrong...");
  }

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readData().c_str());
  });

  Serial.println("\nStarting server...");
  server.begin();
  Serial.println("Server started successfully.");
}

void loop() {
  //Serial.printf("Number of connected devices (stations) = %d\n", WiFi.softAPgetStationNum());
  //delay(3000);
}