/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com  
*********/

// Load Wi-Fi library
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include <Adafruit_Sensor.h>
//#include <DHT.h>

#define SSID_HTML "SSID"
#define PASSWORD_HTML "PASSWORD"
#define IPADDRESS_HTML "IP"

// Replace with your network credentials
const char* local_ssid     = "Wherehouse Plate Unit";
const char* local_password = "mypassword";

char ssid[33];
char password[65];
char ip_address[17];

int mode = 0;

char temp_string_a[] = "";
char temp_string_b[] = "";

// Set web server port number to 80
//WiFiServer server(80);
AsyncWebServer server(80);

int stock = 0;

// Variable to store the HTTP request
String header;

bool switch_to_station;

String testText() {
  return String("How did you even get here?");
}

String strwrapper(const char * strin) {
  return String(strin);
}

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
    <form action="/submit" method="get">
      <div>
        <label for="fssid">Router SSID:</label><br>
        <input type="text" id="fssid" name="fssid" required>
      </div>
      <div>
        <label for="fpass">Router Password:</label><br>
        <input type="text" id="fpass" name="fpass" required>
      </div>
      <div>
        <label for="fipad">Server IP Address:</label><br>
        <input type="text" id="fipad" name="fipad" required>
      </div>
      <div>
        <br><br><input type="submit" class="button" value="Confirm">
      </div>
    </form>
  </body>
  </html>
)rawliteral";

const char submit_page_html[] PROGMEM = R"rawliteral(
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
    <h1>Wi-Fi info updated successfully!</h1>
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
      <a href="/"><button class="button">< Back</button>
      <a href="/continue"><button class="button">Next ></button>
    </div>
  </body>
  </html>
)rawliteral";

// IDK if the submit button works yet

// I could make multiple of these HTML rawliterals
// to simulate multiple webpages...


String html_replacer(const String& var) {
  // This is passed to the ESP Async Web Server in order
  // to replace %variable% with the actual value stored in memory.
  //
  // I'm using defined constants so that I don't mistype something.
  if (var == SSID_HTML) {
    return String(ssid);
  }
  else if (var == PASSWORD_HTML) {
    return String(password);
  }
  else if (var == IPADDRESS_HTML) {
    return String(ip_address);
  }
  else {
    return String();
  }
}

void switch_mode() {
  // Turn off the WiFi server and restart wifi in Station mode,
  // so that it can attempt to connect to another network.

  delay(1000);
  Serial.print("WiFi status: ");
  Serial.println(WiFi.status());
  if (WiFi.status() == WL_CONNECTED){
    WiFi.disconnect();
  }

  delay(2000);
  //WiFi.mode(WIFI_STA); //StackOverflow says this isn't necessary
  WiFi.begin(ssid, password); // Now we connect to the router!
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi failed :/");
  }
  return;
}


void setup() {
  switch_to_station = false;

  delay(2000);
  Serial.begin(115200);
  Serial.println("Serial port started successfully!");

  // Start AP (access point) for web server
  Serial.print("Setting AP (Access Point)...");
  boolean result = WiFi.softAP(local_ssid, local_password);

  if (result != true) {
    Serial.println("error! Something went wrong...");
  }
  else {
    IPAddress myIP = WiFi.softAPIP();

    Serial.println("done!");
    Serial.println("");
    Serial.print("WiFi netowrk name: ");
    Serial.println(local_ssid);
    Serial.print("WiFi network password: ");
    Serial.println(local_password);
    Serial.print("Host IP Address: ");
    Serial.println(myIP);

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html, html_replacer);
    });

    server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", testText().c_str());
    });

    // This is where you end up after you press the submit button
    // Maybe make a second webpage for this later?
    server.on("/submit", HTTP_GET, [](AsyncWebServerRequest *request){
      // Check if GET parameter exists (see README > GET, POST and FILE parameters)
      if((request->hasParam("fssid")) && (request->hasParam("fpass")) && (request->hasParam("fipad"))) {
        
        // TODO Define these string indices somewhere 
        int args = request->args();
        for (int i = 0; i < args; i++) {
          //char[] argname = request->argName(i).c_str();
          //char[] argval = request->arg(i).c_str();
          
          //Serial.print("Argname: ");
          //Serial.println(request->argName(i));
          //Serial.print("Argval: ");
          //Serial.println(request->arg(i));

          // This seems suspicious and inefficient, but this is senior
          // design, not a coding class. If it works, it works.
          if (request->argName(i) == "fssid") {
            strcpy(ssid, request->arg(i).c_str());
            //Serial.println(request->arg(i));
          }
          else if (request->argName(i) == "fpass") {
            strcpy(password, request->arg(i).c_str());
            //Serial.println(request->arg(i).c_str());
          }
          else if (request->argName(i) == "fipad") {
            strcpy(ip_address, request->arg(i).c_str());
            //Serial.println(request->arg(i).c_str());
          }
        }

        Serial.print("New Router SSID = ");
        Serial.println(ssid);
        Serial.print("New Router Password = ");
        Serial.println(password);
        Serial.print("New Target IP Address = ");
        Serial.println(ip_address);

        request->send_P(200, "text/html", submit_page_html, html_replacer);
      }
      else {
        request->send_P(200, "text/plain", "Error: Bad request");
      }
    });

    server.on("/continue", HTTP_GET, [](AsyncWebServerRequest *request){
      Serial.println("Attempting to switch modes...");
      switch_to_station = true;

      request->send_P(200, "text/plain", strwrapper("Proceeding.\n").c_str());
    });


    Serial.println("\nStarting Server...");
    server.begin();
    Serial.println("Server started successfully.");
  }
}

void loop(){
  // This is unused by the web server. 
  // The whole point of the Async Web Server is that it
  // doesn't require looping; it happens asynchronously.

  if (switch_to_station) {
    delay(2000);
    server.end();
    switch_mode();

    if (WiFi.status() == WL_CONNECTED) {
      switch_to_station = false;
      Serial.println("We switched it successfully!");
    }
    // Next test: try hosting the web server on this computer, then switching and connecting
    // to the web server, getting data, and printing it out.
  }
}
