//
//  network.cpp
//  
//
//  Created by Stephen McKeon on 7/18/26.
//

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>  // 💡 Fixes: 'WiFiManager' / 'wm' was not declared in this scope
#include <esp_wifi.h>
#include <ei_conversion.h>
#include <ei_logging.h>
#include <ei_storage.h>
#include <ei_network.h>

EiNetwork network;

/*-------------------------  STARTUP NETWORK SUBSYSTEM  -------------------------*/

bool EiNetwork::startup() {
  logInfo("Starting Network subsystem");
  WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
      this->aWiFiEvent(event);
  });


//  WiFi.onEvent(network.aWiFiEvent);                                   // Register WiFi event callbacks
  if (!checkHardware()) {                                     // Verify WiFi hardware is available
    logError("WiFi hardware not detected");
    return false;
  }
  if (!readConfigFromDisk()) {                                 // Load network configuration
    logError("Unable to load network configuration");
    return false;
  }
  if (!validateConfiguration()) {                             // Validate configuration
    logError("Invalid network configuration");
    return false;
  }
  network.logInfo("Network subsystem ready");
  return true;
}

/*---------------  USED ON BOOT TO CONNECT TO THE NET  ---------------*/

bool EiNetwork::connect(String& ssid, String& pwd, int from) {
  network.logInfo("Connecting to SSID " + ssid + ", Length = " + String(ssid.length()) + conv.fromStr(from));
  
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
  network.logInfo("Handing connection control to the automated WiFiManager pipeline..." + conv.fromStr(from));
  bool success = wm.autoConnect(appConsts.accessPtName.c_str());

  if (!success) {
    network.logInfo("Failed to connect to SSID " + ssid + " and portal timed out. Initializing AP fallback state." + conv.fromStr(from));
    
    // Set your existing global status track variables exactly like your old code did
    WiFi.mode(WIFI_AP);
    WiFi.softAP(appConsts.accessPtName);
    _state.accessPtIP = WiFi.softAPIP().toString();
 //   _state.srvIPAddr = accessPtIP;
    _state.connectedSSID = "accessPt";
    _state.apCreatedTime = millis();
    
    network.logInfo("Access point name: " + appConsts.accessPtName + conv.fromStr(from));
    network.logInfo("AP IP address: " + _state.accessPtIP + conv.fromStr(from));
    logging.dividerStr(FN, LN);
    return false;
  }
  
  // 4. Connection Success! Update your global variables cleanly
  _state.connectedSSID = WiFi.SSID();
  _state.ipAddress = WiFi.localIP().toString();
  
  network.logInfo("WiFi Connection SUCCESSFUL! IP: " + _state.ipAddress  + conv.fromStr(from));
  
  return true;
}

/*---------------  PUT THE RIGHT HEADERS INTO A NETWORK INFO LOG  ---------------*/

void EiNetwork::logInfo(const String& msg) {
    logging.msg(WHOAMI, T::SYSLOG, L::INFO, ET::NETWORK, msg);
}

/*---------------  PUT THE RIGHT HEADERS INTO A NETWORK ERROR LOG  ---------------*/

void EiNetwork::logError(const String& msg) {
    logging.msg(WHOAMI, T::SYSLOG, L::ERROR, ET::NETWORK, msg);
}

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

/*---------------  CALLED WHENEVER A WIFI EVENT OCCURS  ---------------*/

void EiNetwork::aWiFiEvent(WiFiEvent_t event) {
  String response = "Event:" + String(event) + ": ";
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
      response += "Obtained IP address: " + WiFi.localIP().toString();
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
      response += "WiFi access point started, AP IP address: " + WiFi.softAPIP().toString();
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
    default: response += "Received an unknown WiFi event.";
  }
    network.logInfo(response);
}

/*-------------------------  LOAD NET CREDENTIALS FROM DISK  -------------------------*/

bool EiNetwork::readConfigFromDisk() {
  JsonDocument doc;
  if (!storage.readJsonFile(_configFileName, doc, LN))
    return false;
  _config.ssid = doc["ssid"] | _config.ssid;
  _config.password = doc["password"] | _config.password;
  return true;
}

/*-----  CREATE THE CONFIG JSON OBJECT FROM THE cfg CONTENTS  -----*/

JsonDocument EiNetwork::createConfigJson(const NetworkCredentials& cfg) const {
    JsonDocument doc;
    doc["ssid"] = cfg.ssid;
    doc["password"] = cfg.password;
    return doc;
}

  /*-----  BOOT UP WIFI HARDWARE CHECK  -----*/

  bool EiNetwork::checkHardware() {
      // 1. Verify that the underlying ESP-IDF Wi-Fi driver is initialized.
      // If it isn't init yet, we attempt to set a basic mode to trigger it.
      WiFi.mode(WIFI_STA);
      
      wifi_mode_t currentMode;
      esp_err_t err = esp_wifi_get_mode(&currentMode);
      
      if (err == ESP_ERR_WIFI_NOT_INIT) {
        logError("Wi-Fi hardware driver failed to initialize.");
          return false;
      }
      
      // 2. Query the physical Wi-Fi radio's burnt-in MAC address.
      // If the radio hardware is faulty or unreadable, this returns an empty or zeroed string.
      String mac = WiFi.macAddress();
      if (mac == "00:00:00:00:00:00" || mac.length() == 0) {
        logError("Wi-Fi hardware check failed: Invalid MAC address.");
          return false;
      }
      
    network.logInfo("Wi-Fi hardware is verified and ready. MAC: " + mac);
      return true;
  }

  /*-----  VALIDATE THE WIFI CONFIGURATION  -----*/

  bool EiNetwork::validateConfiguration() {
      if (_config.ssid.isEmpty()) {
        logError("Network SSID is not configured.");
          return false;
      }
      // WPA2 passwords must be at least 8 characters.
      // Allow empty passwords in case the user intentionally connects
      // to an open network.
      if (!_config.password.isEmpty() && _config.password.length() < 8) {
        logError("Wi-Fi password must be at least 8 characters.");
        return false;
      }

      return true;
  }
