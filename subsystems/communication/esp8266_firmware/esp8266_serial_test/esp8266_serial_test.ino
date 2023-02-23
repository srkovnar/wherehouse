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
#define SKIPWIFI 1 //0 = connect, 1 = don't connect automatically
#define VERBOSE 1
//#define VERBOSE 0 //If 0, don't print anything unnecessary

#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64
#define MAX_IP_LENGTH 16

#define MAX_STRING_LENGTH 32 //For miscellaneous strings, such as dbname, root, etc.

#define CONN_TIMEOUT 10

#define ACK "ACK" //Return this on a success (if no data to return)
#define NACK "NACK" //Return this on a failure

#define HTTP_TAG "http://"

ESP8266WiFiMulti WiFiMulti;

//const char* ssid      = "Wherehouse";
char ssid[MAX_SSID_LENGTH+1] = "Wherehouse"; //The +1 is for the null terminator
//const char* password  = "12345678";
char password[MAX_PASSWORD_LENGTH+1] = "12345678";

const char* http_tag = "http://";
//const char* ip_addr = "192.168.4.1";
//const char* ip_addr = "192.168.0.101";
char ip_addr[MAX_IP_LENGTH+1] = "192.168.1.102";
//const char* data_ext = "/data";

const char* data_ext = "/demo/esp-data.php";
const char* post_data_ext = "/demo/post-esp-data.php";
const char* get_data_ext = "/demo/get-esp-data.php";

//Maybe replace post-esp-data.php with esp-data.php

String apiKeyValue = "tPmAT5Ab3j7F9";
String shelfNumber = "One";
String led = "WACK";
String F = "70.25";

char device_id[9] = "1";

float stock = 0;


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
const char modifier__run = '!';

// Read/Write
const char* cmd__ip     = "WF+IP";    // Hub IP Address
const char* cmd__ssid   = "WF+SSID";  // Wi-Fi SSID
const char* cmd__pass   = "WF+PW";    // Wi-Fi Password
const char* cmd__id     = "WF+ID";    // Device ID

const char* cmd__connect      = "WF+CONN";  // Connect to network
const char* cmd__disconnect   = "WF+DISC";  // Disconnect from network

const char* cmd__send   = "WF+SEND";  // Send data
const char* cmd__config = "WF+CFG";   // Get configuration (item name, weight, tare)

//char cmd[64];

int incomingByte = 0; // for incoming serial data

int i = 0;
const int STOP = 10;

String data;

unsigned long previousMillis = 0;
const long interval = 5000;

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

  int L1;
  int L2;
  int minlength;
  int i;
  int mismatch;

  L1 = strlen(str1);
  L2 = strlen(str2);
  minlength = min(L1, L2);

  i = 0;
  mismatch = 0;
  while (i < minlength) {
    if (str1[i] != str2[i]) {
      mismatch++;
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

  int cmp;
  
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
    cmp = strcmp(value, arg);
    if (cmp == 0) {
      Serial.println(ACK);
    }
    else {
      Serial.println(NACK);
    }
  }
  else if (cmd[index] == modifier__run) {
    Serial.println(NACK);
    if (VERBOSE) {
      Serial.println("\t! option unavailable for this command");
    }
    return 2;
  }
  else {
    Serial.println(NACK);
    if (VERBOSE) {
      Serial.println("\tModifier unrecognized. Should be ?, =, or !");
    }
    return 3;
  }

  return 0;
}

int connect() {
  int timer = 0;

  if (VERBOSE) {
    Serial.print("Connecting to ");
    Serial.println(ssid);
  }

  WiFi.begin(ssid, password);
  while ((WiFi.status() != WL_CONNECTED) && (timer < CONN_TIMEOUT)) {
    timer++;
    delay(1000);
    if (VERBOSE) {Serial.print(".");}
  }
  
  if (timer >= CONN_TIMEOUT) {
    if (VERBOSE) {
      Serial.println("\tConnection timed out.");
    }
    else {
      Serial.println(NACK);
    }
    return 1;
  }
  else {
    if (VERBOSE) {
      Serial.println("");
      Serial.println("Connected to WiFi!");
      // Print IP (Not sure what this does)
      Serial.println("");
      Serial.println(WiFi.localIP()); //This is the IP of the ESP8266
    }
    else {
      Serial.println(ACK);
    }
    return 0;
  }
}

int disconnect() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
  }
  if (VERBOSE) {
    Serial.print("Status: ");
    Serial.println(WiFi.status());
  }
  else {
    Serial.print(ACK);
  }
  return 0;
}

int get_config() {
  WiFiClient client;
  HTTPClient http;

  String http_request_data;
  String server_name;
  String full_address;
  String payload;

  int http_response_code;

  // Build server name
  char temp_server_name[64] = "";

  if (WiFi.status() != WL_CONNECTED) {
    if (VERBOSE) {
      Serial.println("\tERROR: Cannot get config, not connected to WiFi");
    }
    else {
      Serial.println(NACK);
    }
    return 1; //No Wifi error
  }

  strcat(temp_server_name, HTTP_TAG);
  strcat(temp_server_name, ip_addr);
  strcat(temp_server_name, get_data_ext);

  http_request_data = ("?api_key=" + apiKeyValue + "&device=" + device_id);
  server_name = String(temp_server_name);
  full_address = server_name + http_request_data;

  if (VERBOSE) {
    Serial.print("\tAccessing server at:");
    Serial.println(full_address);
  }

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.begin(client, full_address);
  if (VERBOSE) {Serial.println("\tStarted HTTP successfully!");}

  http_response_code = http.GET();
  if (VERBOSE) {Serial.println("\tGET complete");}

  payload = "--";

  if (http_response_code > 0) {
    payload = http.getString();
    if (VERBOSE) {
      Serial.print("HTTP response code: ");
      Serial.println(http_response_code);
    }
    Serial.println(payload);
  }
  else {
    Serial.println(NACK);
    if (VERBOSE) {
      Serial.print("Error code: ");
      Serial.println(http_response_code);
    }
  }

  http.end();

  if (http_response_code == 200) {
    return 0;
  }
  else {
    return 1;
  }
}

/*
int try_run(const char* cmd, const char* cmd_stub) {
  int stub_length = strlen(cmd_stub);
  int cmd_length = strlen(cmd);

  if (cmd_length < stub_length) {return 1;}

  if (cmd[stub_length] == modifier__run) {
    
  }
  else {
    return 1;
  }
}
*/

int handler(const char* cmd) {
  //Serial.print("Received: ");
  //Serial.println(cmd);

  const char* cmd_header = "WF+";

  int mismatch;
  int i;
  bool status;

  int temp;
  int out;
  int cmd_length;

  out = 77;

  mismatch = compare_char_array(cmd_header, cmd);

  if (mismatch) {
    Serial.print("Invalid command; must start with ");
    Serial.println(cmd_header);
    return 1;
  }

  // === VALUE COMMANDS ===
  temp = compare_char_array(cmd, cmd__ssid);
  if (temp == 0) {
    out = value_tool(7, cmd, ssid);
    return out;
  }

  temp = compare_char_array(cmd, cmd__pass);
  if (temp == 0) {
    out = value_tool(5, cmd, password);
    return out;
  }

  temp = compare_char_array(cmd, cmd__ip);
  if (temp == 0) {
    out = value_tool(5, cmd, ip_addr);
    return out;
  }

  temp = compare_char_array(cmd, cmd__id);
  if (temp == 0) {
    out = value_tool(5, cmd, device_id);
    return out;
  }

  // === '!' COMMANDS ===
  temp = compare_char_array(cmd, cmd__connect);
  if (temp == 0) {
    //Serial.println("Connecting...");
    cmd_length = strlen(cmd__connect);
    if (strlen(cmd) > cmd_length) {
      if (cmd[cmd_length] == modifier__run) {
        out = connect();
      }
      else if (cmd[cmd_length] == modifier__get) {
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("Connected");
          return 0;
        }
        else {
          Serial.println("Disconnected");
          return 0;
        }
      }
      else {
        Serial.println(NACK);
        if (VERBOSE) {Serial.print("\tWF+CONN (connect) command should end with '?' or '!'");}
        return 1;
      }
    }
  }

  temp = compare_char_array(cmd, cmd__disconnect);
  if (temp == 0) {
    if (cmd[7] == modifier__run) {
      out = disconnect();
      return out;
    }
    else {
      if (VERBOSE) {
        Serial.println("\tWF+DISC (disconnect) command only accepts '!' ending.");
      }
      else {
        Serial.println(NACK);
      }
      return 1;
    }
  }

  temp = compare_char_array(cmd, cmd__config);
  if (temp == 0) {
    if (cmd[6] == modifier__run) {
      out = get_config();
    }
    else {
      if (VERBOSE) {
        Serial.println("\tWF+CFG (Get config) command only available with '!' ending.");
      }
      else {
        Serial.println(NACK);
      }
      return 1;
    }
  }

  if (out == 77) {
    Serial.println(NACK);
    if (VERBOSE) {Serial.println("\tUnrecognized command.");}
  }

  return out;
}


// === MAIN OPERATION STARTS HERE ===
void setup() {
  int temp;
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  delay(1000);

  if (SKIPWIFI) {
    Serial.println("Skipping WiFi connection.");
  }
  else {
    /*
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
    */
    temp = connect();

    // Testing dynamic creating of address.
    strcat(buf, HTTP_TAG);
    strcat(buf, ip_addr);
    strcat(buf, data_ext);
    Serial.println(buf);

    strcat(buf_post, HTTP_TAG);
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
