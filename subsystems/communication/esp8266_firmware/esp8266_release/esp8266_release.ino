/*
  Based partly on the Client code from :
  https://randomnerdtutorials.com/esp8266-nodemcu-client-server-wi-fi/

  Also used
  https://forum.arduino.cc/t/concatenate-char/376886/2

  https://reference.arduino.cc/reference/en/language/functions/communication/serial/read/

*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <Hash.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <EEPROM.h> // Not sure if this board even HAS an eeprom... but it's worth a try.
// Using EEPROM for storing wifi data so that I don't have to reconfigure it every single time.

#define DOTDELAY 2000 //milliseconds
//#define VERBOSE 1
#define VERBOSE 0 //If 0, don't print anything unnecessary

#define START_AS_AP 1
// If this is 0, we skip straight to station mode,
// and wi-fi must be configured manually over serial communication.

#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64
#define MAX_IP_LENGTH 16

#define MAX_STRING_LENGTH 32 //For miscellaneous strings, such as dbname, root, etc.

#define CONN_TIMEOUT 10

#define ACK "ACK" //Return this on a success (if no data to return)
#define NACK "NACK" //Return this on a failure
#define HTTP_TAG "http://"

// Error reasons
#define WRONG_MODIFIER    1
#define UNRECOGNIZED_CMD  2
#define FAILED            3

// Decorators
#define GET_ONLY      0
#define SET_ONLY      1
#define RUN_ONLY      2
#define GET_OR_SET    3
#define SET_OR_RUN    4
#define GET_OR_RUN    5
#define ANY_MODIFIER  6

ESP8266WiFiMulti WiFiMulti;

AsyncWebServer server(80);

char local_ssid[MAX_SSID_LENGTH+1] = "Wherehouse Unit ";
char local_password[MAX_PASSWORD_LENGTH+1] = "mypassword";

/*
char ssid[MAX_SSID_LENGTH+1] = "Wherehouse"; //The +1 is for the null terminator
char password[MAX_PASSWORD_LENGTH+1] = "12345678";
char ip_addr[MAX_IP_LENGTH+1] = "192.168.1.102";
*/

char ssid[MAX_SSID_LENGTH+1];
char password[MAX_PASSWORD_LENGTH+1];
char ip_addr[MAX_IP_LENGTH+1];

const char* server_folder = "/demo"; // The folder where all the php and html files are kept, inside htdocs
const char* data_ext = "/esp-data.php"; // Unused
const char* post_data_ext = "/post-esp-data.php";
const char* get_data_ext = "/get-esp-data.php";

String apiKeyValue = "tpmat5ab3j7f9";
char api_key[32] = "tpmat5ab3j7f9";

char device_id[9] = "1";

char stock[MAX_STRING_LENGTH] = "0";
char name[MAX_STRING_LENGTH] = "Widgets";


//For connecting to database
const char* dbname = "esp3866_db";
const char* username = "root";
const char* dbpassword = "";

bool switch_to_station; // We're not in station mode, and we should be in access mode.
bool switch_to_access; // We're in access mode, and we should be in station mode.

int status; //Status returned from handling command


// === COMMAND MATCH STRINGS ===
const char modifier__get = '?';
const char modifier__set = '=';
const char modifier__run = '!';

// Read/Write
const char* cmd__ip     = "WF+IPAD";    // Hub IP Address
const char* cmd__ssid   = "WF+SSID";  // Wi-Fi SSID
const char* cmd__pass   = "WF+PWRD";    // Wi-Fi Password
const char* cmd__id     = "WF+DVID";    // Device ID

const char* cmd__stock  = "WF+STCK"; // Amount of item in stock
const char* cmd__name   = "WF+NAME";  // Name of item

const char* cmd__connect      = "WF+CONN";  // Connect to network
const char* cmd__disconnect   = "WF+DCON";  // Disconnect from network

const char* cmd__send   = "WF+SEND";  // Send data
const char* cmd__config = "WF+CNFG";   // Get configuration (item name, weight, tare)

const char* cmd__access  = "WF+APMD"; // Go back to Access Point mode

// Miscellaneous
const char* cmd__ping   = "WF+PING"; 

String data;

unsigned long previous_time;
unsigned long current_time;
const long interval = 500;

// This seems wack, but you can actually program HTML into the C code.
// All you have to do is make sure this gets sent when someone accesses the
// web server. (See the `setup` function for details on how this gets sent).

// Each one of these const char rawliterals is its own HTML page.
// You could probably embed PHP in the same way, although I haven't tried it.
const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
      html {
        font-family: Helvetica;
        display: inline-block;
        margin: 0px auto;
        text-align: center;
      }
      h2 { font-size: 3.0rem; }
      p { font-size: 1.0rem; }
      .button {
        background-color: #195B6A;
        border: none;
        color: white;
        padding: 16px 40px;
        text-decoration: none;
        font-size: 30px;
        marin: 2px;
        cursor: pointer;
      }
      .button2 {
        background-color: #77878A;
      }
    </style>
  </head>
  <body>
    <h1>ESP8266 Web Server</h1>
    <p>
      Current SSID: <span id="ssid">%SSID%</span>
    </p>
    <p>
      Current password: <span id="password">%PASSWORD%</span>
    </p>
    <p>
      Current IP Address: <span id="ip">%IP%</span>
    </p>
    <div>
      <form action="/ssid" method="get">
        <label for="fssid2">Wi-Fi Network Name (SSID):</label><br>
        <input type="text" id="fssid2" name="fssid2" required>
        <input type="submit" value="Update">
      </form>
    </div>
    <div>
      <form action="/password" method="get">
        <label for="fpass2">Wi-Fi Password:</label><br>
        <input type="text" id="fpass2" name="fpass2" required>
        <input type="submit" value="Update">
      </form>
    </div>
    <div>
      <form action="/ip" method="get">
        <label for="fipad2">Server IP Address:</label><br>
        <input type="text" id="fipad2" name="fipad2" required>
        <input type="submit" value="Update">
      </form>
    </div>
    <a href="/continue"><button class="button">Activate</button>
  </body>
  </html>
)rawliteral";


String html_replacer(const String& var) {
  // This is passed to the ESP Async Web Server in order
  // to replace %variable% with the actual value stored in memory.
  // The ESPAsyncWebServer.h library accomodates this.
  // For example, %SSID% in the HTML code will be replaced with the value of ssid.

  if (var == "SSID") {
    return String(ssid);
  }
  else if (var == "PASSWORD") {
    return String(password);
  }
  else if (var == "IP") {
    return String(ip_addr);
  }
  else {
    return String();
  }
}


void print_error(const char* cmd, int reason, int decorator) {
  // This handles serial printing of errors.
  // Nothing too fancy here, but there are a lot of 
  // preprocessor-defined values in here for readability.

  Serial.println(NACK);

  if (VERBOSE) {
    if (reason == 1) {
      Serial.print("\t");
      Serial.print(cmd);
      if (decorator == GET_ONLY) {
        Serial.println(" only available with '?' modifier.");
      }
      else if (decorator == SET_ONLY) {
        Serial.println(" only available with '=' modifier.");
      }
      else if (decorator == RUN_ONLY) {
        Serial.println(" only available with '!' modifier.");
      }
      else if (decorator == GET_OR_SET) {
        Serial.println(" only available with '?' or '=' modifiers.");
      }
      else if (decorator == SET_OR_RUN) {
        Serial.println(" only available with '=' or '!' modifiers.");
      }
      else if (decorator == GET_OR_RUN) {
        Serial.println(" only available with '?' or '!' modifiers.");
      }
      else if (decorator == ANY_MODIFIER) {
        Serial.println(" must be followed by '?', '=value', or '!' modifiers.");
      }
    }
    else if (reason == UNRECOGNIZED_CMD) {
      Serial.print("\t Command '");
      Serial.print(cmd);
      Serial.println("' is unrecognized. Check for typos.");
    }
  }
}


void enter_station_mode(const char* new_ssid, const char* new_password) {
  /*
  if (VERBOSE) {Serial.println("Attempting to switch modes.");}
  if (VERBOSE) {Serial.print("\tStopping server...");}
  delay(1000);
  server.end();
  if (VERBOSE) {Serial.println("done!");}
  */

  if (VERBOSE) {Serial.print("\tStopping access point...");}
  //https://stackoverflow.com/questions/39688410/how-to-switch-to-normal-wifi-mode-to-access-point-mode-esp8266
  WiFi.softAPdisconnect(true); // Trying this to turn off the network.
  WiFi.disconnect();
  delay(1000);
  if (VERBOSE) {Serial.println("done!");}

  //delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(new_ssid, new_password); // Now we connect to the router!
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    if (VERBOSE) {Serial.println("Successfully connected to new network!");}
  }
  else {
    if (VERBOSE) {Serial.println("Something went wrong, wifi connection failed.");}
  }
}

void enter_access_mode() {
  if (VERBOSE) {Serial.println("Attempting to enter access point (setup) mode.");}
  delay(1000);
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
  }
  //WiFi.end(); // (3) This isn't valid. There is no wifi.end()

  // Maybe try manually switching the mode? Like WiFi.mode(WIFI_AP);
  WiFi.mode(WIFI_AP); // (2) This doesn't do it either, but no crash at least.
  WiFi.softAP(local_ssid, local_password); // This doesn't seem to work...
  if (VERBOSE) {Serial.print("\tHost IP Address: "); Serial.println(WiFi.softAPIP());}


  // Actually, it works if I just don't stop the server.
  // If I stop the server, something breaks.
  /*
  if (VERBOSE) {Serial.println("Starting server...");}

  server.reset();

  // Trying this...
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", "Well, it kind of worked.");
  });

  server.begin();
  if (VERBOSE) {Serial.println("Server started successfully!");}
  */
}

AsyncWebServer setup_server() {
  // I tried using this to setup the web server between stops
  // and starts. It did not work. idk WHY it didn't work, but when
  // the server stops (server.end()), it is hard to get it to start again.
  // This is a future problem.


  AsyncWebServer new_server(80);
  // Not sure if I can even initialize stuff like that here...

  // Home page
  new_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, html_replacer);
  });

  new_server.begin(); //Trying to start it here... see if it crashes...

  return new_server;
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


int value_tool(const char* cmd, const char* base_cmd, char* value){
  /* this will be capable of setting and getting values, like the IP address*/
  int k;
  int n;
  char buffer[MAX_STRING_LENGTH];
  char arg[MAX_STRING_LENGTH];

  int index = strlen(base_cmd);

  int cmp;
  
  if (strlen(cmd) <= index) {
    Serial.println("NACK");
    if (VERBOSE) {Serial.println("\tIt's not long enough, mate.");}
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
  
  if (cmd[index] == modifier__get) {
    Serial.println(value);
  }
  else if (cmd[index] == modifier__set) {
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
    print_error(base_cmd, WRONG_MODIFIER, RUN_ONLY);
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
  Serial.println(ACK);
  if (VERBOSE) {
    Serial.print("\tStatus: ");
    Serial.println(WiFi.status());
  }
  return 0;
}


int get_config(const char* api_key/*unused*/) {
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
    Serial.println(NACK);
    if (VERBOSE) {
      Serial.println("\tERROR: Cannot get config, not connected to WiFi");
    }
    return 1; //No Wifi error
  }

  strcat(temp_server_name, HTTP_TAG);
  strcat(temp_server_name, ip_addr);
  strcat(temp_server_name, server_folder);
  strcat(temp_server_name, get_data_ext);
  server_name = String(temp_server_name);

  //http_request_data = ("?api_key=" + apiKeyValue + "&device=" + device_id);
  http_request_data = ("?api_key=" + apiKeyValue + "&device=" + device_id);
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
    payload.trim();
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

  return http_response_code;
}


int post_stock() {
  WiFiClient client;
  HTTPClient http;

  String http_request_data;
  String server_name;

  int http_response_code;

  char temp_server_name[64] = "";

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(NACK);
    if (VERBOSE) {
      Serial.println("\tERROR: Cannot get config, not connected to WiFi");
    }
    return 1; //No Wifi error
  }

  strcat(temp_server_name, HTTP_TAG);
  strcat(temp_server_name, ip_addr);
  strcat(temp_server_name, server_folder);
  strcat(temp_server_name, post_data_ext);
  server_name = String(temp_server_name);
  //server_name = HTTP_TAG + ip_addr + folder + post_data_ext;

  http.begin(client, server_name);

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  http_request_data = "api_key=" + apiKeyValue + "&device=" + device_id + "&stock=" + stock + "&name=" + name;
  
  if (VERBOSE) {
    Serial.print("http_request_data: ");
    Serial.println(http_request_data);
  }

  http_response_code = http.POST(http_request_data);

  if (http_response_code > 0){
    //Serial.println(ACK);
    if (VERBOSE) {
      Serial.print("\tHTTP Response Code: ");
    }
    Serial.println(http_response_code);
  }
  else {
    Serial.println(NACK);
    if (VERBOSE) {
      Serial.print("\tError code: ");
      Serial.println(http_response_code);
    }
  }

  http.end();

  return http_response_code;
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
  int cmd_length;

  out = 77;

  mismatch = compare_char_array(cmd_header, cmd);

  if (mismatch) {
    Serial.print("Invalid command; must start with ");
    Serial.println(cmd_header);
    return 1;
  }

  // === VALUE COMMANDS ===
  if (compare_char_array(cmd, cmd__ssid) == 0) {
    out = value_tool(cmd, cmd__ssid, ssid);
    return out;
  }

  if (compare_char_array(cmd, cmd__pass) == 0) {
    out = value_tool(cmd, cmd__pass, password);
    return out;
  }

  if (compare_char_array(cmd, cmd__ip) == 0) {
    out = value_tool(cmd, cmd__ip, ip_addr);
    return out;
  }

  if (compare_char_array(cmd, cmd__id) == 0) {
    out = value_tool(cmd, cmd__id, device_id);
    return out;
  }

  if (compare_char_array(cmd, cmd__stock) == 0) {
    out = value_tool(cmd, cmd__stock, stock);
    return out;
  }

  if (compare_char_array(cmd, cmd__name) == 0) {
    out = value_tool(cmd, cmd__name, name);
    return out;
  }

  // === '!' COMMANDS ===
  if (compare_char_array(cmd, cmd__connect) == 0) {
    //Serial.println("Connecting...");
    cmd_length = strlen(cmd__connect);
    if (strlen(cmd) > cmd_length) {
      if (cmd[cmd_length] == modifier__run) {
        out = connect();
        return out;
      }
      else if (cmd[cmd_length] == modifier__get) {
        if (WiFi.status() == WL_CONNECTED) {
          //Serial.println("Connected");
          Serial.println(ACK);
          return 0;
        }
        else {
          //Serial.println("Disconnected");
          Serial.println(NACK);
          return 0;
        }
      }
      else {
        print_error(cmd__connect, WRONG_MODIFIER, GET_OR_RUN);
        return 1;
      }
    }
  }

  if (compare_char_array(cmd, cmd__disconnect) == 0) {
    cmd_length = strlen(cmd__disconnect);
    if (cmd[cmd_length] == modifier__run) {
      return disconnect();
    }
    else {
      print_error(cmd__disconnect, WRONG_MODIFIER, RUN_ONLY);
      return 1;
    }
  }

  if (compare_char_array(cmd, cmd__config) == 0) {
    cmd_length = strlen(cmd__config);
    if (cmd[cmd_length] == modifier__run) {
      return get_config(apiKeyValue.c_str());
    }
    else {
      print_error(cmd__config, WRONG_MODIFIER, RUN_ONLY);
      return 1;
    }
  }

  if (compare_char_array(cmd, cmd__send) == 0) {
    cmd_length = strlen(cmd__send);
    if (cmd[cmd_length] == modifier__run) {
      out = post_stock();
      return out;
    }
    else {
      print_error(cmd__send, WRONG_MODIFIER, RUN_ONLY);
      return 1;
    }
  }

  if (compare_char_array(cmd, cmd__access) == 0) {
    cmd_length = strlen(cmd__access);
    if (cmd[cmd_length] == modifier__run) {
      enter_access_mode();
      Serial.println(ACK);
      return 0;
    }
    else {
      print_error(cmd__access, WRONG_MODIFIER, RUN_ONLY);
      return 1;
    }
  }

  if (compare_char_array(cmd, cmd__ping) == 0) {
    cmd_length = strlen(cmd__ping);
    if (cmd[cmd_length] == modifier__run) {
      // Send NACK unless connected properly; might change this functionality later
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println(ACK);
      }
      else {
        Serial.println(NACK);
      }
      //Serial.println(ACK);
      return 0;
    }
    else {
      print_error(cmd__ping, WRONG_MODIFIER, RUN_ONLY);
      return 1;
    }
  }

  print_error(cmd, UNRECOGNIZED_CMD, 0);

  return out;
}


void fetch_wifi_memory(char* d_ssid, char* d_pw, char* d_ip) {
  // Fetch wifi connection parameters stored in EEPROM (actually RAM) memory
  // and update the corresponding char arrays.
  // d_ssid = destination to store SSID from memory
  // d_pw = destination to store password from memory
  // d_ip = destination to store IP address from memory

  int address = 0; //Arbitrary, keep it consistent

  struct {
    // TODO: Remove the +1 if it gets too big, because it's just a null terminator.
    // It can be assumed. Just load the raw data from memory and slice it up
    // as necessary. A project for another day.
    char m_ssid[MAX_SSID_LENGTH+1] = "";
    char m_pw[MAX_PASSWORD_LENGTH+1] = "";
    char m_ip[MAX_IP_LENGTH+1] = "";
  } wifi_data;

#ifdef VERBOSE
  Serial.println("Fetching wifi connection information from memory.");
#endif

  // Fetch memory information.
  EEPROM.get(address, wifi_data);

#ifdef VERBOSE
  Serial.print("\tSSID: ");
  Serial.println(wifi_data.m_ssid);
  Serial.print("\tPassword: ");
  Serial.println(wifi_data.m_pw);
  Serial.print("\tIP Address: ");
  Serial.println(wifi_data.m_ip);
#endif

  strcpy(d_ssid, wifi_data.m_ssid);
  strcpy(d_pw, wifi_data.m_pw);
  strcpy(d_ip, wifi_data.m_ip);

  return;
}


void update_wifi_memory(
  const char* new_ssid,
  const char* new_pw,
  const char* new_ip
) {
  // Function for updating stored WiFi connection information.
  // If saved connection information is out-of-date, then update memory.
  // Remember that memory writes should be limited, because memory has
  // a limited lifespan. That's why we actually check whether memory needs to be updated;
  // only write when necessary.

  int ssid_match;
  int pw_match;
  int ip_match;

  int address = 0; // Could be anything within the 512-byte limit defined in setup()
  
  struct {
    // TODO: Remove the +1 if it gets too big, because it's just a null terminator.
    // It can be assumed. Just load the raw data from memory and slice it up
    // as necessary. A project for another day.
    char m_ssid[MAX_SSID_LENGTH+1] = "";
    char m_pw[MAX_PASSWORD_LENGTH+1] = "";
    char m_ip[MAX_IP_LENGTH+1] = "";
  } wifi_data;

  // Fetch memory information.
  EEPROM.get(address, wifi_data);

  if (VERBOSE) {Serial.println("\tSaving wifi connection info to memory.");}

  // Check if data needs to be updated.
  ssid_match = strcmp(new_ssid, wifi_data.m_ssid);
  pw_match = strcmp(new_pw, wifi_data.m_pw);
  ip_match = strcmp(new_ip, wifi_data.m_ip);

  if (ssid_match != 0) {
    strcpy(wifi_data.m_ssid, new_ssid);
  }
  if (pw_match != 0) {
    strcpy(wifi_data.m_pw, new_pw);
  }
  if (ip_match != 0) {
    strcpy(wifi_data.m_ip, new_ip);
  }

  // If any of the info doesn't match, write back to memory.
  if ((ssid_match != 0) || (pw_match != 0) || (ip_match != 0)) {
    // Replace values in byte-array cache
    EEPROM.put(address, wifi_data);
    // Actually commit values to memory
    EEPROM.commit();
  }

  return;
}


// === MAIN OPERATION STARTS HERE ===
void setup() {
  // Declare some local variables
  IPAddress myIP;
  bool ap_result;
  int ram_address = 0;

  // Initialize variables
  switch_to_station = false;
  switch_to_access = false;

  strcat(local_ssid, device_id);

  // Begin UART serial communication
  Serial.begin(115200);

  // Commit 512 bytes of ESP8266 flash (for "EEPROM" emulation).
  // The esp8266 doesn't *actually* have an EEPROM, but it DOES have
  // flash memory that can be treated the same as an actual EEPROM.
  // This step loads the content of 512 bytes of flash into a 512-byte-array
  // cache in RAM.
  // EEPROM activity is stored in other functions.
  // See update_wifi_memory() and fetch_wifi_memory()
  EEPROM.begin(512);

  // Delay to give time for Serial to start
  delay(1000);

  // Update values from memory
  fetch_wifi_memory(ssid, password, ip_addr);

  // === START AP ===
  if (START_AS_AP) {
    if (VERBOSE) {Serial.print("Setting AP (Access Point)...");}
    ap_result = WiFi.softAP(local_ssid, local_password);
    if (ap_result != true) {
      if (VERBOSE) {Serial.println("Error! Something went wrong...");}
    }
    else {
      myIP = WiFi.softAPIP();

      if (VERBOSE){
        Serial.println("done!");
        Serial.print("\tWiFi network name: ");
        Serial.println(local_ssid);
        Serial.print("\tWiFi network password: ");
        Serial.println(local_password);
        Serial.print("\tHost IP Address: ");
        Serial.println(myIP);
      }

      // Initialize web server

      if (VERBOSE) {Serial.print("Starting server...");}

      //setup_server(server); //This caused a crash
      //AsyncWebServer server = setup_server(); // This doesn't crash, but... nothing happens on access.

      // Homepage
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html, html_replacer);
      });

      // Redirects
      server.on("/ssid", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasArg("fssid2")) {
          strcpy(ssid, request->arg("fssid2").c_str());
        }
        request->redirect("/");
      });

      server.on("/password", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasArg("fpass2")) {
          strcpy(password, request->arg("fpass2").c_str());
        }
        request->redirect("/");
      });

      server.on("/ip", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasArg("fipad2")) {
          strcpy(ip_addr, request->arg("fipad2").c_str());
        }
        request->redirect("/");
      });

      // Redirect and activate STATION mode
      server.on("/continue", HTTP_GET, [](AsyncWebServerRequest *request){
        switch_to_station = true;// Tells the device to exit setup mode.
        request->redirect("/");
      });

      server.begin();
      
      if (VERBOSE) {Serial.println("done!");Serial.println("Setup complete.\n");}
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  
  // Non-blocking, allows for other operations to continue.
  current_time = millis();

  if (current_time - previous_time >= interval) {
    if (switch_to_station) {
      // Make sure connection info is stored in memory
      update_wifi_memory(ssid, password, ip_addr);

      // Turn off the server and connect to a network
      enter_station_mode(ssid, password);

      switch_to_station = false;
    }
    /*
    else if (switch_to_access) {
      enter_access_mode(local_ssid, local_password);
      switch_to_access = false;
    }
    */
    // ^ This is being taken care of in the handler.
    else if (Serial.available() > 0) {
      // If there is serial data that has been received...
      String temp_command = Serial.readString();
      temp_command.trim(); // remove \r and \n
      status = handler(temp_command.c_str());
    }

    previous_time = current_time;
  }
}
