/*
  Based partly on the Client code from :
  https://randomnerdtutorials.com/esp8266-nodemcu-client-server-wi-fi/

  Also used
  https://forum.arduino.cc/t/concatenate-char/376886/2

  https://reference.arduino.cc/reference/en/language/functions/communication/serial/read/

*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>

#define DOTDELAY 2000 //milliseconds
#define SKIPWIFI 0 //0 = connect, 1 = don't connect

ESP8266WiFiMulti WiFiMulti;

const char* ssid      = "Wherehouse";
const char* password  = "12345678";

//const char* server_name_data = "http://192.168.4.1/data";
const char* http_tag = "http://";
//const char* ip_addr = "192.168.4.1";
const char* ip_addr = "192.168.0.101";
//const char* data_ext = "/data";

const char* data_ext = "/demo/esp-data.php";
const char* post_data_ext = "/demo/post-esp-data.php";
const char* test_data_ext = "/demo/test-esp-data.php";
const char* get_data_ext = "/demo/get-esp-data.php";

//Maybe replace post-esp-data.php with esp-data.php

String apiKeyValue = "tPmAT5Ab3j7F9";
String shelfNumber = "One";
String led = "WACK";
String F = "70.25";

String seek_id = "14";


//For connecting to database
const char* dbname = "esp3866_db";
const char* username = "root";
const char* dbpassword = "";


char buf[64]; // For dynamic address creation
char buf_post[64]; //For POST testing
char buf_test2[64]; //For httpGetTest2

int incomingByte = 0; // for incoming serial data

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
  //int httpResponseCode = http.GET("");
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

String httpGETRequest_test(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  String temp_server_name = String(serverName);

  String httpRequestData = ("api_key=" + apiKeyValue + "&shelfnumber=" + shelfNumber + "&temperature=" + F + "&led=" + led);
  Serial.print("httpRequestData: ");
  Serial.println(httpRequestData);

  //strcat(temp_server_name, serverName);
  //strcat(temp_server_name, httpRequestData);

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  http.begin(client, temp_server_name+httpRequestData); // Does this work???
  // ^ Expects (client, String) arguments

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

String httpGETRequest_id(const char* serverName, int id) {
  /* Read one row from the server by ID */
  WiFiClient client;
  HTTPClient http;

  String temp_server_name = String(serverName);
  id_str = String(id);

  String httpRequestData = ("?api_key=" + apiKeyValue + "&id=" + id_str);
  Serial.print("httpRequestData: ");
  Serial.println(httpRequestData);

  //strcat(temp_server_name, serverName);
  //strcat(temp_server_name, httpRequestData);

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String temp48 = temp_server_name + httpRequestData;
  Serial.print("\tThis is where we're requesting from: ");
  Serial.println(temp48);

  http.begin(client, temp_server_name+httpRequestData); // Does this work???
  // ^ Expects (client, String) arguments

  Serial.println("\tStarted HTTP successfully");

  int httpResponseCode = http.GET();

  Serial.println("\tGet complete");

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

void httpPOSTRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, serverName);

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String httpRequestData = "api_key=" + apiKeyValue + "&shelfnumber=" + shelfNumber + "&temperature=" + F + "&led=" + led;
  Serial.print("httpRequestData: ");
  Serial.println(httpRequestData);

  int httpResponseCode = http.POST(httpRequestData);

  if (httpResponseCode > 0){
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();

  return;
}

void setup() {
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  delay(1000);

  if (SKIPWIFI) {
    Serial.println("Skipping WiFi connection.");
  }
  else {
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(2000);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("Connected to WiFi!");

    // Print IP (Not sure what this does)
    Serial.println("");
    Serial.println(WiFi.localIP()); //This is the IP of the ESP8266

    // Testing dynamic creating of address.
    strcat(buf, http_tag);
    strcat(buf, ip_addr);
    strcat(buf, data_ext);
    Serial.println(buf);

    strcat(buf_post, http_tag);
    strcat(buf_post, ip_addr);
    strcat(buf_post, post_data_ext);
    Serial.println(buf_post);

    strcat(buf_test2, http_tag);
    strcat(buf_test2, ip_addr);
    strcat(buf_test2, get_data_ext);
    Serial.println(buf_test2);

    // Uncomment below to disconnect after ten seconds
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

    // Uncomment below to try posting
    /*
    if ((WiFiMulti.run() == WL_CONNECTED)) {
      httpPOSTRequest(buf_post);
    }
    Serial.println("Post finished.\n");
    */
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    
    if ((WiFiMulti.run() == WL_CONNECTED)) {
      //data = httpGETRequest(buf);
      //data = httpGETRequest_test(buf_post);
      data = httpGETRequest_id(buf_test2, 14);
      Serial.println("Data: " + data);
      previousMillis = currentMillis;
    }
    else {
      Serial.println("WiFi Disconnected");
    }
  }
}
