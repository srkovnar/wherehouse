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

#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64
#define MAX_IP_LENGTH 16

#define MAX_STRING_LENGTH 32 //For miscellaneous strings, such as dbname, root, etc.

ESP8266WiFiMulti WiFiMulti;

//const char* ssid      = "Wherehouse";
char ssid[MAX_SSID_LENGTH] = "Wherehouse";
//const char* password  = "12345678";
char password[MAX_PASSWORD_LENGTH] = "12345678";

const char* http_tag = "http://";
//const char* ip_addr = "192.168.4.1";
//const char* ip_addr = "192.168.0.101";
char ip_addr[MAX_IP_LENGTH] = "192.168.0.101";
//const char* data_ext = "/data";

const char* data_ext = "/demo/esp-data.php";
const char* post_data_ext = "/demo/post-esp-data.php";

//Maybe replace post-esp-data.php with esp-data.php

String apiKeyValue = "tPmAT5Ab3j7F9";
String shelfNumber = "One";
String led = "WACK";
String F = "70.25";


//For connecting to database
const char* dbname = "esp3866_db";
const char* username = "root";
const char* dbpassword = "";


char buf[64]; // For dynamic address creation
char buf_post[64]; //For POST testing

char cmd[128]; //Storage for command
int status;



// === COMMAND MATCH STRINGS ===
const char modifier__get = '?';
const char modifier__set = '=';
const char modifier__exe = '!';

const char* cmd__ip     = "WF+IP";
const char* cmd__ssid   = "WF+SSID";
const char* cmd__pass   = "WF+PW";


//char cmd[64];

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
//  int httpResponseCode = http.GET("");
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

String httpGETRequest_test(
  const char* serverName
){
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

int compare_char_array(const char* str1, const char* str2) {
  // Return number of mismatches between str1 and str2.
  // If they are different lengths, it will compare based on the 
  // length of the shorter string and will not consider additional characters.
  // (see if one string contains the other).

  // === Declare local variables ===
  int L1;
  int L2;
  int minlength;
  int i;
  int mismatch;

  // === Program Operation ===
  //Serial.println("\tStarting the new function...");

  L1 = strlen(str1);
  L2 = strlen(str2);
  minlength = min(L1, L2);

  i = 0;
  mismatch = 0;
  while (i < minlength) {
    if (str1[i] != str2[i]) {
      //Serial.print("\tmismatch on character ");
      //Serial.println(str1[i]);
      mismatch++;
    }
    i++;
  }

  return mismatch;
}

bool match_char_arrays(const char* str1, const char* str2) {
  /*  Similar to compare_char_arrays, but with bool return.*/
  int L1;
  int L2;
  int minlength;
  int i;
  bool mismatch;

  L1 = strlen(str1);
  L2 = strlen(str2);
  minlength = min(L1, L2);

  i = 0;
  mismatch = false;
  while (i < minlength) {
    if (str1[i] != str2[i]) {
      mismatch = true;
    }
    i++;
  }

  return mismatch;
}

int value_tool(int index, const char* cmd, char* value){
  /* this will be capable of setting and getting values, like the IP address*/
  int k;
  int n;
  char buffer[MAX_STRING_LENGTH];
  char arg[MAX_STRING_LENGTH];
  
  if (strlen(cmd) <= index) {
    Serial.println("\tIt's not long enough, mate.");
    return 1;
  }

  //Get argument from command
  k = 0;
  n = 0;
  while (cmd[k] != '\0') {
    n = k - index - 1;
    if (k > index) { //Get everything after the ?, =, or !
      arg[n] = cmd[k];
      arg[n+1] = '\0'; //Make sure there's a null terminator at the end
    }
    k++;
  }
  //Serial.print("Argument: ");
  //Serial.println(arg);
  
  if (cmd[index] == modifier__get) {
    //Serial.println("\tIt's a get command");
    Serial.println(value);
  }
  else if (cmd[index] == modifier__set) {
    //Serial.println("\tIt's a set command");
    strcpy(value, arg);
  }
  else if (cmd[index] == modifier__exe) {
    //Serial.println("\t! modifier unavailable for this command");
    Serial.println("NACK");
    return 2;
  }
  else {
    Serial.println("\tModifier unrecognized. Should be ?, =, or !");
    return 3;
  }

  return 0;
}

int handler(const char* cmd) {
  //Serial.print("Received: ");
  //Serial.println(cmd);

  const char* cmd_header = "WF+";

  int mismatch;
  int i;
  bool status;

  int temp;
  int out;

  mismatch = compare_char_array(cmd_header, cmd);
  //Serial.print("Number of mismatches: ");
  //Serial.println(mismatch);

  if (mismatch) {
    Serial.print("Invalid command; must start with ");
    Serial.println(cmd_header);
    return 1;
  }
  /*
  else {
    Serial.println("Command has valid header.");
  }
  */

  //temp = 0;
  temp = compare_char_array(cmd, cmd__ssid);
  if (temp == 0) {
    //Serial.println("Found an SSID command!");
    out = value_tool(7, cmd, ssid);
  }

  temp = compare_char_array(cmd, cmd__pass);
  if (temp == 0) {
    //Serial.println("Found a password command");
    out = value_tool(5, cmd, password);
  }

  temp = compare_char_array(cmd, cmd__ip);
  if (temp == 0) {
    //Serial.println("Found an IP command");
    out = value_tool(5, cmd, ip_addr);
  }


  return 0;
}


// === MAIN OPERATION STARTS HERE ===
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

  /*
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    if ((WiFiMulti.run() == WL_CONNECTED)) {
      //data = httpGETRequest(buf);
      data = httpGETRequest_test(buf_post);
      Serial.println("Data: " + data);
      previousMillis = currentMillis;
    }
    else {
      Serial.println("WiFi Disconnected");
    }
  }
  */

  // Read string
  // https://reference.arduino.cc/reference/en/language/functions/communication/serial/readstring/
  //This works, but I want to try doing it with an if instead
  while (Serial.available() == 0) {} // Wait for data available
  //int menuChoice = Serial.parseInt();
  String temp_command = Serial.readString(); // Has to be Arduino String, not string
  temp_command.trim(); // remove \r and \n
  //Serial.print("Received ");
  //Serial.println(temp_command);

  status = handler(temp_command.c_str());
}
