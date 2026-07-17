
#include <ArduinoTrace.h>
#include <WiFi.h>
#include <WiFiManager.h>  // 💡 Fixes: 'WiFiManager' / 'wm' was not declared in this scope
#include "esp_wifi.h"     // 💡 Fixes: 'esp_wifi_set_max_tx_power' was not declared in this scope

#include <mySD.h>
#include <myMQTT.h>
#include <commonItems_ESP32.h>
#include <myText.h>
#include <myWiFi.h>

//#ifndef ACCESS_POINT_NAME
//    #define ACCESS_POINT_NAME "ESP32_ACCESS_POINT"
//#endif

/*
* WiFi Events
0  ARDUINO_EVENT_WIFI_READY                 < ESP32 WiFi ready
1  ARDUINO_EVENT_WIFI_SCAN_DONE             < ESP32 finish scanning AP
2  ARDUINO_EVENT_WIFI_STA_START             < ESP32 station start
3  ARDUINO_EVENT_WIFI_STA_STOP              < ESP32 station stop
4  ARDUINO_EVENT_WIFI_STA_CONNECTED         < ESP32 station connected to AP
5  ARDUINO_EVENT_WIFI_STA_DISCONNECTED      < ESP32 station disconnected from AP
6  ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE   < the auth mode of AP connected by ESP32 station changed
7  ARDUINO_EVENT_WIFI_STA_GOT_IP            < ESP32 station got IP from connected AP
8  ARDUINO_EVENT_WIFI_STA_LOST_IP           < ESP32 station lost IP and the IP is reset to 0
9  ARDUINO_EVENT_WPS_ER_SUCCESS             < ESP32 station wps succeeds in enrollee mode
10 ARDUINO_EVENT_WPS_ER_FAILED              < ESP32 station wps fails in enrollee mode
11 ARDUINO_EVENT_WPS_ER_TIMEOUT             < ESP32 station wps timeout in enrollee mode
12 ARDUINO_EVENT_WPS_ER_PIN                 < ESP32 station wps pin code in enrollee mode
13 ARDUINO_EVENT_WIFI_AP_START              < ESP32 soft-AP start
14 ARDUINO_EVENT_WIFI_AP_STOP               < ESP32 soft-AP stop
15 ARDUINO_EVENT_WIFI_AP_STACONNECTED       < a station connected to ESP32 soft-AP
16 ARDUINO_EVENT_WIFI_AP_STADISCONNECTED    < a station disconnected from ESP32 soft-AP
17 ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED      < ESP32 soft-AP assign an IP to a connected station
18 ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED     < Receive probe request packet in soft-AP interface
19 ARDUINO_EVENT_WIFI_AP_GOT_IP6            < ESP32 ap interface v6IP addr is preferred
19 ARDUINO_EVENT_WIFI_STA_GOT_IP6           < ESP32 station interface v6IP addr is preferred
20 ARDUINO_EVENT_ETH_START                  < ESP32 ethernet start
21 ARDUINO_EVENT_ETH_STOP                   < ESP32 ethernet stop
22 ARDUINO_EVENT_ETH_CONNECTED              < ESP32 ethernet phy link up
23 ARDUINO_EVENT_ETH_DISCONNECTED           < ESP32 ethernet phy link down
24 ARDUINO_EVENT_ETH_GOT_IP                 < ESP32 ethernet got IP from connected AP
19 ARDUINO_EVENT_ETH_GOT_IP6                < ESP32 ethernet interface v6IP addr is preferred
25 ARDUINO_EVENT_MAX
*/

const String netInfoFname = appDir + "/netInfo.txt";                            // name of the net information file
Ticker wifiReconnectTimer;
//time_t accessPtConnectTime = 0;                                                // time the access point was created
time_t AP_createdTime = 0;                                                      // time the access point was created

/*---------------  CALLED WHENEVER A WIFI EVENT OCCURS  ---------------*/

void aWiFiEvent(WiFiEvent_t event) {
  bool handled = false;
  String response = "[WiFi-event] event:" + String(event) + " - ";
  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
      response += "WiFi interface ready";
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      response += "Completed scan for access points";
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      response += "WiFi client started";
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      response += "WiFi clients stopped";
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      response += "Connected to access point";
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      response += "Disconnected from WiFi access point";
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      response += "Authentication mode of access point has changed";
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      handled = true;
      mySP(response + "Obtained IP address: " + WiFi.localIP().toString() + "\n", FN, LN);
      onWifiConnect(event);
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      response += "Lost IP address and IP address is reset to 0";
      break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
      response += "WiFi Protected Setup (WPS): succeeded in enrollee mode";
      break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
      response += "WiFi Protected Setup (WPS): failed in enrollee mode";
        break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
      response += "WiFi Protected Setup (WPS): timeout in enrollee mode";
      break;
    case ARDUINO_EVENT_WPS_ER_PIN:
      response += "WiFi Protected Setup (WPS): pin code in enrollee mode";
      break;
    case ARDUINO_EVENT_WIFI_AP_START: {
      IPAddress IP = WiFi.softAPIP();                                               // get a default ip address
      accessPtIP = IP.toString();                                                   // save the ip address of this access point
      srvIPAddr = accessPtIP;
//      connectedSSID = "accessPt";                                                   // remember we are in the access point mode
      mySP("AP IP address: " + accessPtIP + "\n", FN, LN);                          // tell the world the device s coming up as an access point
      mySP(response += "WiFi access point started\n", FN, LN);
      handled = true;
      break; }
    case ARDUINO_EVENT_WIFI_AP_STOP:
      response += "WiFi access point  stopped";
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      response += "Client connected";
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      response += "Client disconnected";
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      response += "Assigned IP address to client";
      break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
      response += "Received probe request";
      break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
      response += "AP IPv6 is preferred";
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
      response += "STA IPv6 is preferred";
      break;
    case ARDUINO_EVENT_ETH_GOT_IP6:
      response += "Ethernet IPv6 is preferred";
      break;
    case ARDUINO_EVENT_ETH_START:
      response += "Ethernet started";
      break;
    case ARDUINO_EVENT_ETH_STOP:
      response += "Ethernet stopped";
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      response += "Ethernet connected";
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      response += "Ethernet disconnected";
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      response += "Obtained IP address";
      break;
    default: response += "Received an unknown WiFi event ID. Event: " + String(event) + ".";
      break;
  }
    if(!handled) mySP(response + "\n", FN, LN);
}

void logLoginDataStructValues(LogInData_Ptr data) {
  mySP("NetInfo file contents:"
        "\n     data->ssid = " + data->ssid +
        "\n     data->ssidPwd = ••••••••••••"                                   // + data->ssidPwd +
        "\n     data->tzAbbrv = " + data->tzAbbrv +
        "\n     data->olsenTzName = " + data->olsenTzName +
        "\n", FN, LN);
}

/*---------------   LogInData STRUCT TO JSON STRING  ---------------*/

void LogInDataToStr(String &jsonOutput, LogInData_Ptr data, bool showPass) {
  // Allocate our clean ArduinoJson v7 document
  JsonDocument doc;

  // Map the struct fields using your pointer assignments
  doc["ssid"]  = data->ssid;
  doc["pass"]  = data->ssidPwd;
  doc["tz"]    = data->tzAbbrv;
  doc["olson"] = data->olsenTzName;
  doc["posix"] = data->posixTzRule;

  // Serialize the object directly into a safe local buffer (No fragmentation)
  char buffer[256];
  serializeJson(doc, buffer, sizeof(buffer));
  
  // Drop the finished text directly into your passed-in variable
  jsonOutput = buffer;
  
  // Optional debugging log matching your original structure
  if(showPass) {
    mySP("loginData JSON = " + jsonOutput + "\n", FN, LN);
  }
}

/*---------------    READ NETINFO FILE FROM DISK   ---------------*/

void readNetInfoFromDisk(String fn) {
  String s = myFileSys.readFile(fn.c_str(), true);
  
  // Clean up any trailing hidden whitespace or line breaks
  s.trim();

  // --- CRITICAL SELF-HEALING INJECTION ---
  // If the file fails to open OR if it is completely blank (0 bytes)
  if(s.indexOf("ERROR 100:") != -1 || s.length() == 0) {
    if(s.length() == 0) {
      mySP("The file '" + fn + "' was empty. Re-initializing with structural JSON defaults...\n", FN, LN);
    } else {
      mySP("ERROR 100: The file '" + fn + "' could not be opened.\n", FN, LN);
    }
    writeNetInfoToDisk(fn);     // Rewrites the file using your unified JSON library helper
    return;                     // Stop execution pass here so the unpopulated code exits cleanly
  }
  
  // Existing JSON Parsing Logic Continues Safely...
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, s);

  // If someone manually edited the file and broke the JSON formatting strings
  if (error) {
    mySP("ERROR: Failed to parse network JSON config! Error: " + String(error.c_str()) + "\n", FN, LN);
    mySP("Rewriting corrupt file layout with defaults...\n", FN, LN);
    writeNetInfoToDisk(fn);
    return;
  }

  // Assign the saved elements straight to your global struct safely by key name
  logInD1.ssid        = doc["ssid"].as<String>();
  logInD1.ssidPwd     = doc["pass"].as<String>();
  logInD1.tzAbbrv     = doc["tz"].as<String>();
  logInD1.olsenTzName = doc["olson"].as<String>();
  
  if (doc["posix"].is<String>()) {
    logInD1.posixTzRule = doc["posix"].as<String>();
  } else {
    logInD1.posixTzRule = "CST6CDT,M3.2.0,M11.1.0";
  }

  if(false) logLoginDataStructValues(&logInD1);
}

/*---------------  WRITE NET INFO TO DISK (JSON VERSION)  ---------------*/

int writeNetInfoToDisk(String fn) {
  // 1. Pack the current global state variables into JSON keys
  JsonDocument doc;
  doc["ssid"]  = logInD1.ssid;
  doc["pass"]  = logInD1.ssidPwd;
  doc["tz"]    = logInD1.tzAbbrv;
  doc["olson"] = logInD1.olsenTzName;
  doc["posix"] = logInD1.posixTzRule; // Saves your active POSIX rule!

  // 2. Serialize the JSON document directly into a clean String variable
  String jsonOutput;
  serializeJson(doc, jsonOutput);

  // 3. Hand the string over to your standard centralized library function
  int bytesWritten = myFileSys.writeFile(fn.c_str(), jsonOutput.c_str(), LN);
  
  if(bytesWritten > 0) {
    mySP("Successfully wrote network JSON config to " + fn + "\n", FN, LN);
  }
  
  // Return the byte count to completely satisfy the 'int' signature rule
  return bytesWritten;
}

/*---------------  THE SERVER JUST GOT AN IP ADDRESS  ---------------*/

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
// TBD...
}

/*---------------  SET UP THE WIFI SYSTEM  ---------------*/

void wifi_setup() {
  WiFi.onEvent(aWiFiEvent);
  mySP(pdl(FN, LN), "-1", -1);                                                  // print a dashed line in the log
  mySP("wifi_setup(), Connecting to WiFi\n", FN, LN);                         // print a dashed line in the log
}

/*----------------------------------------------------------------------------------------------------------*/

/*---------------  RUNS WHEN A WIFI CONNECTION IS MADE  ---------------*/

void onWifiConnect(WiFiEvent_t event) {
//  accessPtConnectTime = 0;                                                      // connected to the desired wifi sysztem so set this to zero
  AP_createdTime = 0;
  connectedSSID = WiFi.SSID();                                                  // remember the ssid the server is connected to
  srvIPAddr = WiFi.localIP().toString();                                        // save the ip address the server has received
  mySP(pdl(FN, LN), "-1", -1);                                                  // print a dashed line in the log
  startWebServer();                                                             // start the web server
  if(connectedSSID != "accessPt") {                                             // if the server is not set up as an access point
    startEzTime = true;
    setUpEZTime(logInD1.olsenTzName);                                           // and attempt to reset it
    startMQTT = true;
    doAppWiFiConnected();
  }
}

/*---------------  WHEN THE WIFI CONNECTION IS LOST  ---------------*/

void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  mySP("Disconnected from Wi-Fi.\n", FN, LN);
  aWiFiEvent(event);
  wifiReconnectTimer.once(2, connectToWiFi, LN);                                    // attempt to connect to the wifi again
}

/*---------------  CONNECT TO WIFI  ---------------*/

void connectToWiFi(int calledBy) {
  mySP("Connecting to Wi-Fi...CALLED BY LINE#: " + String(calledBy) + "\n", FN, LN);
  connToWiFi(logInD1.ssid, logInD1.ssidPwd, LN);
}

/*---------------  USED ON BOOT TO CONNECT TO THE NET  ---------------*/

bool connToWiFi(String ssid, String pwd, int from) {
  mySP("Connecting to SSID " + ssid + ", Length = " + String(ssid.length()) + "\n", FN, from);
  
  // 1. CRITICAL ESP32-C3 PATRICK: Cap radio TX power to completely eliminate brownout crashes
  WiFi.mode(WIFI_STA);
  esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
  
  // 2. Initialize WiFiManager
  WiFiManager wm;
  
  // If the credentials are empty or the router connection fails, wait 3 minutes (180s)
  // for a user to connect to the AP portal before timing out and trying again.
  wm.setConfigPortalTimeout(180);
  
  // Make the configuration page look like your sleek dark mode theme
  wm.setClass("invert");
  wm.setCustomHeadElement("<style>.c{background-color:#1a1a1a;} body{background-color:#121212; color:#ffffff;} button,input[type='submit']{background-color:#337ab7; color:white;}</style>");
  
  // 3. Fire the automated connection and fallback routine
  // Pass your custom Access Point name from your existing constant data pointer
  mySP("Handing connection control to the automated WiFiManager pipeline...\n", FN, from);
  bool success = wm.autoConnect(constDataPtr->accessPtName.c_str());
  
  if (!success) {
    mySP("Failed to connect to SSID " + ssid + " and portal timed out. Initializing AP fallback state.\n", FN, from);
    
    // Set your existing global status track variables exactly like your old code did
    WiFi.mode(WIFI_AP);
    WiFi.softAP(constDataPtr->accessPtName);
    accessPtIP = WiFi.softAPIP().toString();
    srvIPAddr = accessPtIP;
    connectedSSID = "accessPt";
    AP_createdTime = millis();
    
    mySP("Access point name: " + constDataPtr->accessPtName + "\n", FN, from);
    mySP("AP IP address: " + accessPtIP + "\n", FN, from);
    pdl(FN, from);
    return false;
  }
  
  // 4. Connection Success! Update your global variables cleanly
  connectedSSID = WiFi.SSID();
  srvIPAddr = WiFi.localIP().toString();
  
  mySP("\n", "-1", from);
  mySP("WiFi Connection SUCCESSFUL! IP: " + srvIPAddr + "\n", FN, from);
  
  return true;
}

/*---------------  IS IP ADDR DEFINED  ---------------*/

String isIpDefined(String ip) {
  if(ip.length() > 0) {
    return ip;
  } else return "Not in use";
}

