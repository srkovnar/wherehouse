/*
  Based partly on the Client code from :
  https://randomnerdtutorials.com/esp8266-nodemcu-client-server-wi-fi/

  Also used
  https://forum.arduino.cc/t/concatenate-char/376886/2

  https://reference.arduino.cc/reference/en/language/functions/communication/serial/read/

*/

#include <Arduino.h>
#include <EEPROM.h>
#include <Hash.h>
#include <WiFiClient.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>



#define FW_VERSION "0.1.0" // Readable with WF+VRSN?


#define DOTDELAY 2000 //milliseconds
//#define VERBOSE 1
#define VERBOSE 0 //If 0, don't print anything unnecessary

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

char ssid[MAX_SSID_LENGTH+1];
char password[MAX_PASSWORD_LENGTH+1];
char server_ip[MAX_IP_LENGTH+1];

const char* server_folder = "/demo"; // The folder where all the php and html files are kept, inside htdocs
const char* data_ext = "/esp-data.php"; // Unused
const char* post_data_ext = "/post-esp-data.php";
const char* get_data_ext = "/get-esp-data.php";

String apiKeyValue = "tpmat5ab3j7f9";
char api_key[32] = "tpmat5ab3j7f9";

char device_id[9] = "1";

char stock[MAX_STRING_LENGTH] = "0";
char name[MAX_STRING_LENGTH] = "Widgets"; // Unused


//For connecting to database
const char* dbname = "esp3866_db";
const char* username = "root";
const char* dbpassword = "";

bool switch_to_station; // We're not in station mode, and we should be in access mode.
bool switch_to_access; // We're in access mode, and we should be in station mode.

int status; //Status returned from handling command

// Not all of these are implemented yet... vvv
// Flags (stored in g_status)
const int F__SETUP   = (1 << 0); // If 1, done with setup (unimplemented)
const int F__READY   = (1 << 1); // 0 = still in access point mode. 1 = normal operation (station mode).
const int F__ERROR   = (1 << 2); // Unrecoverable error
const int F__SW2STAT = (1 << 3); // Switch to station mode
const int F__SW2AP   = (1 << 4); // Switch to access point mode
const int F__SERVERUP= (1 << 5); // Raise once server is started

int g_status;


// === COMMAND MATCH STRINGS ===
const char modifier__get = '?';
const char modifier__set = '=';
const char modifier__run = '!';
const char separator = ':';

// Read/Write
const char* cmd__ip     = "WF+IPAD";    // Hub IP Address
const char* cmd__ssid   = "WF+SSID";  // Wi-Fi SSID
const char* cmd__pass   = "WF+PWRD";    // Wi-Fi Password
const char* cmd__id     = "WF+DVID";    // Device ID

const char* cmd__stock  = "WF+STCK"; // Amount of item in stock
const char* cmd__name   = "WF+NAME";  // Name of item

const char* cmd__connect      = "WF+CONN";  // Connect to network
const char* cmd__disconnect   = "WF+DCON";  // Disconnect from network

const char* cmd__send   = "WF+SEND";  //! Send data
const char* cmd__config = "WF+CNFG";   //! Get configuration (item name, weight, tare)

const char* cmd__access  = "WF+APMD"; //! Go back to Access Point mode

// Miscellaneous
const char* cmd__ping   = "WF+PING"; //!
const char* cmd__version= "WF+VRSN"; //?

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
      <form action="/submitall" method="get">
        <br>
        <label for="fssid">SSID:</label><br>
        <input type="text" id="fssid" name="fssid"><br>
        <label for="fpass">Password:</label><br>
        <input type="text" id="fpass" name="fpass"><br>
        <label for="fipad">Server IP Address:</label><br>
        <input type="text" id="fipad" name="fipad"><br>
        <br><input type="submit" class="button" value="Update Settings"><br>
      </form>
    </div>
    <br><a href="/continue"><button class="button">Done</button><br>
    <br><a href="/scan"><button class="button">Scan for networks</button><br>
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
    return String(server_ip);
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
    Serial.print(ACK);
    Serial.print(":");
    Serial.println(value);
    return 0;
  }
  else if (cmd[index] == modifier__set) {
    strcpy(value, arg);
    cmp = strcmp(value, arg);
    if (cmp == 0) {
      Serial.print(ACK);
      Serial.print(":");
      Serial.println(value);
      return 0;
    }
    else {
      Serial.print(NACK);
      Serial.print(":");
      Serial.println(value);
      return 1;
    }
  }
  else if (cmd[index] == modifier__run) {
    print_error(base_cmd, WRONG_MODIFIER, GET_OR_SET);
    return 2;
  }
  else {
    print_error(base_cmd, WRONG_MODIFIER, ANY_MODIFIER);
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
    return 1;
  }
  else {
    if (VERBOSE) {
      Serial.println("");
      Serial.println("Connected to WiFi!");
      // Print IP (Not sure what this does)
      Serial.println("");
      Serial.println(WiFi.localIP()); //This is the IP of the ESP8266
      update_wifi_memory(ssid, password, "");
    }
    return 0;
  }
}


int disconnect() {
  
  
  if (VERBOSE) {Serial.print("Disconnecting...");}
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
  }
  if (VERBOSE) {Serial.println("done!");}


  if (VERBOSE) {Serial.print("Now waiting for connection to end...");}

  int done;
  int tries = 0;
  do {
    wl_status_t wfstat = WiFi.status();
    if (wfstat == WL_CONNECTED) {
      done = 0;
    }
    else if (tries > 10) {
      done = 2;
    }
    else {
      done = 1;
    }
    tries++;
    delay(10);
  } while (done == 0);

  if (VERBOSE) {Serial.println("done!");}
  if (VERBOSE) {Serial.print("WiFi status = "); Serial.println(WiFi.status());}

  if (done == 1) {
    return 0; //success (ACK)
  }
  else {
    return 1; //failure (NACK)
  }
  
  // if (WiFi.status() != WL_CONNECTED) {
  //   if (VERBOSE) {Serial.println("Disconnected successfully.");}
  //   return 0;
  // }
  // else {
  //   if (VERBOSE) {Serial.println("Unsuccessful disconnect.");}
  //   return 1;
  // }
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
  strcat(temp_server_name, server_ip);
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
    Serial.print(ACK);
    Serial.print(":");
    Serial.println(payload);// Should add the http response code to this in the future, TODO
  }
  else {
    Serial.print(NACK);
    Serial.print(separator);
    Serial.println(http_response_code);
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
  strcat(temp_server_name, server_ip);
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

  if (http_response_code > 0) {
    Serial.print(ACK);
  }
  else {
    Serial.print(NACK);
  }
  Serial.print(separator);
  Serial.println(http_response_code);

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
    Serial.println(NACK);
    return 1;
  }

  // The comparisons below should be done with arrays of pointers
  // and nested for loops. Start with the full list of commands,
  // then go character-by-character eliminating possibilities.

  // Just to get to the WF+CONN command, you need to first go through 7
  // if statements, each of which also contain a whole function which loops through
  // the whole command string. So, instead of looping through the command string
  // over and over again, you should just do it once and narrow down possibilities
  // as you go.

  // === VALUE COMMANDS ===
  if (compare_char_array(cmd, cmd__ssid) == 0) {
    out = value_tool(cmd, cmd__ssid, ssid);
    update_wifi_memory(ssid, password, server_ip);
    return out;
  }

  if (compare_char_array(cmd, cmd__pass) == 0) {
    out = value_tool(cmd, cmd__pass, password);
    update_wifi_memory(ssid, password, server_ip);
    return out;
  }

  if (compare_char_array(cmd, cmd__ip) == 0) {
    out = value_tool(cmd, cmd__ip, server_ip);
    update_wifi_memory(ssid, password, server_ip);
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
        if (out == 0) {
          Serial.println(ACK);
        }
        else {
          Serial.println(NACK);
        }
        return out;
      }
      else if (cmd[cmd_length] == modifier__get) {
        if (WiFi.status() == WL_CONNECTED) {
          Serial.print(ACK);
          Serial.print(":");
          Serial.println("Connected");
          return 0;
        }
        else {
          Serial.print(NACK);
          Serial.print(":");
          Serial.println("Disconnected");
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
      out = disconnect();
      if (WiFi.status() != WL_CONNECTED) {
        Serial.print(ACK);
      }
      else {
        Serial.print(NACK);
      }
        Serial.print(separator);
        Serial.println(WiFi.status());
      return out;
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
      g_status &= ~F__READY; // Lower READY flag
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
      // if (WiFi.status() == WL_CONNECTED) {
      //   Serial.println(ACK);
      // }
      // else {
      //   Serial.println(NACK);
      // }
      if (g_status & F__READY) {
        Serial.print(ACK);
        Serial.print(separator);
        Serial.println(g_status);
      }
      else {
        Serial.print(NACK);
        Serial.print(separator);
        Serial.println(g_status);
      }
      return 0;
    }
    else {
      print_error(cmd__ping, WRONG_MODIFIER, RUN_ONLY);
      return 1;
    }
  }

  if (compare_char_array(cmd, cmd__version) == 0) {
    cmd_length = 7;
    if (cmd[cmd_length] == modifier__get) {
      Serial.print(ACK);
      Serial.print(separator);
      Serial.println(FW_VERSION);
      return 0;
    }
    else {
      print_error(cmd__version, WRONG_MODIFIER, GET_ONLY);
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

  if (VERBOSE) {Serial.println("Fetching wifi connection information from memory.");}

  // Fetch memory information.
  EEPROM.get(address, wifi_data);

  if (VERBOSE) {
    Serial.print("\tSSID: ");
    Serial.println(wifi_data.m_ssid);
    Serial.print("\tPassword: ");
    Serial.println(wifi_data.m_pw);
    Serial.print("\tIP Address: ");
    Serial.println(wifi_data.m_ip);
  }

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

  bool update_ssid = false;
  bool update_pw = false;
  bool update_ip = false;

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

  if ((strlen(new_ssid) > 0) && (ssid_match != 0)) {
    strcpy(wifi_data.m_ssid, new_ssid);
    update_ssid = true;
  }
  if ((strlen(new_pw) > 0) && (pw_match != 0)) {
    strcpy(wifi_data.m_pw, new_pw);
    update_pw = true;
  }
  if ((strlen(new_ip) > 0) && (ip_match != 0)) {
    strcpy(wifi_data.m_ip, new_ip);
    update_ip = true;
  }

  // If any of the info doesn't match, write back to memory.
  if ((update_ssid) || (update_pw) || (update_ip)) {
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

  bool start_as_ap = 0;

  g_status = 0;

  // Initialize variables
  switch_to_station = false;
  switch_to_access = false;

  strcat(local_ssid, device_id);

  // Begin UART serial communication
  Serial.begin(115200);
  // Serial.begin(19200);
  delay(3000); // Wait to finish plugging in...

  // Commit 512 bytes of ESP8266 flash (for "EEPROM" emulation).
  // The esp8266 doesn't *actually* have an EEPROM, but it DOES have
  // flash memory that can be treated the same as an actual EEPROM.
  // This step loads the content of 512 bytes of flash into a 512-byte-array
  // cache in RAM.
  // EEPROM activity is stored in other functions.
  // See update_wifi_memory() and fetch_wifi_memory()
  EEPROM.begin(512);
  //EEPROM.begin(128);
  if (VERBOSE) {Serial.println("EEPROM initialized.");}
  delay(500);

  // Update values from memory
  fetch_wifi_memory(ssid, password, server_ip);
  delay(500);
  if (VERBOSE) {Serial.print("Attempting to connect with saved WiFi parameters...");}
  int try_to_connect = connect();

  if (try_to_connect == 1) { // If failed to connect
    if (VERBOSE) {Serial.println("failed.");}
    start_as_ap = true;
    try_to_connect = disconnect();
    g_status &= ~F__READY;
  }
  else {
    if (VERBOSE) {Serial.println("success!");}
    start_as_ap = false;
    g_status |= F__READY;
  }



  // === START AP ===
  if (start_as_ap) {
    if (VERBOSE) {Serial.print("Setting AP (Access Point)...");}
    ap_result = WiFi.softAP(local_ssid, local_password);
    // if (ap_result != true) {
    //   if (VERBOSE) {Serial.println("Error! Something went wrong...");}
    // }
    // else {
    if (ap_result == true) {
      myIP = WiFi.softAPIP();
      // if (VERBOSE){
      //   Serial.println("done!");
      //   Serial.print("\tWiFi network name: ");
      //   Serial.println(local_ssid);
      //   Serial.print("\tWiFi network password: ");
      //   Serial.println(local_password);
      //   Serial.print("\tHost IP Address: ");
      //   Serial.println(myIP);
      // }
      if (VERBOSE) {Serial.println("done!");}
    }
    else {
      g_status |= F__ERROR;
    }
  }

  // NOTE: Start the server even if we're in station mode,
  // because this is the only time we can start it. If we ever go back
  // into AP mode, then it needs to be already started.

  // Initialize web server
  if (VERBOSE) {Serial.print("Setting up server...");}

  //setup_server(server); //This caused a crash
  //AsyncWebServer server = setup_server(); // This doesn't crash, but... nothing happens on access.

  // Homepage
  if (VERBOSE) {Serial.print("1, ");}
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, html_replacer);
  });

  if (VERBOSE) {Serial.print("2, ");}
  // Redirects
  server.on("/ssid", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("fssid2")) {
      strcpy(ssid, request->arg("fssid2").c_str());
    }
    request->redirect("/");
  });

  if (VERBOSE) {Serial.print("3, ");}
  server.on("/password", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("fpass2")) {
      strcpy(password, request->arg("fpass2").c_str());
    }
    request->redirect("/");
  });

  if (VERBOSE) {Serial.print("4, ");}
  server.on("/ip", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("fipad2")) {
      strcpy(server_ip, request->arg("fipad2").c_str());
    }
    request->redirect("/");
  });

  if (VERBOSE) {Serial.print("5, ");}
  server.on("/submitall", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->arg("fssid").length() > 0) {
      strcpy(ssid, request->arg("fssid").c_str());
    }
    if (request->arg("fpass").length() > 0) {
      strcpy(password, request->arg("fpass").c_str());
    }
    if (request->arg("fipad").length() > 0) {
      strcpy(server_ip, request->arg("fipad").c_str());
    }
    request->redirect("/");
  });


  // Get list of available networks (UNFINISHED)
  //First request will return 0 results unless you start scan from somewhere else (loop/setup)
  //Do not request more often than 3-5 seconds

  /*
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "[";
    int n = WiFi.scanComplete();
    if(n == -2){
      WiFi.scanNetworks(true);
    } else if(n){
      for (int i = 0; i < n; ++i){
        if(i) json += ",";
        json += "{";
        json += "\"rssi\":"+String(WiFi.RSSI(i));
        json += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
        json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
        json += ",\"channel\":"+String(WiFi.channel(i));
        json += ",\"secure\":"+String(WiFi.encryptionType(i));
        json += ",\"hidden\":"+String(WiFi.isHidden(i)?"true":"false");
        json += "}";
      }
      WiFi.scanDelete();
      if(WiFi.scanComplete() == -2){
        WiFi.scanNetworks(true);
      }
    }
    json += "]";
    request->send(200, "application/json", json);
    json = String();
  });
  */
  
  if (VERBOSE) {Serial.print("6, ");}
  // Redirect and activate STATION mode
  server.on("/continue", HTTP_GET, [](AsyncWebServerRequest *request){
    // switch_to_station = true;// Tells the device to exit setup mode.
    g_status |= F__SW2STAT; // Enable SW2STAT bit
    request->redirect("/");
  });

  if (VERBOSE) {Serial.println("done!");}
  if (VERBOSE) {Serial.print("Starting server...");}
  server.begin();
  if (VERBOSE) {Serial.println("done!");}
  g_status |= F__SERVERUP;
      
  //if (VERBOSE) {Serial.println("done!");Serial.println("Setup complete.\n");}
}

void loop() {
  // put your main code here, to run repeatedly:
  
  // Non-blocking, allows for other operations to continue.
  current_time = millis();

  if (current_time - previous_time >= interval) {
    if (g_status & F__SW2STAT) {
    //if (switch_to_station) {
      // Make sure connection info is stored in memory
      update_wifi_memory(ssid, password, server_ip);

      // Turn off the server and connect to a network
      enter_station_mode(ssid, password);

      // switch_to_station = false;
      g_status &= ~F__SW2STAT;
      
      g_status |= F__READY;
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
      int result = handler(temp_command.c_str()); // Result is currently unused.
    }

    previous_time = current_time;
  }
}
