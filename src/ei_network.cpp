//
//  network.cpp
//  
//
//  Created by Stephen McKeon on 7/18/26.
//

#include <Arduino.h>
#include <ei_conversion.h>
#include <ei_logging.h>
#include <ei_network.h>

Network network;

/*-------------------------  STARTUP NETWORK SUBSYSTEM  -------------------------*/

bool Network::startup() {
  logInfo("Starting Network subsystem");
  WiFi.onEvent(aWiFiEvent);                                   // Register WiFi event callbacks
  if (!checkHardware()) {                                     // Verify WiFi hardware is available
    network.logError("WiFi hardware not detected");
    return false;
  }
  if (!loadConfiguration()) {                                 // Load network configuration
    network.logError("Unable to load network configuration");
    return false;
  }
  if (!validateConfiguration()) {                             // Validate configuration
    network.logError("Invalid network configuration");
    return false;
  }
  network.logInfo("Network subsystem ready");
  return true;
}

/*---------------  USED ON BOOT TO CONNECT TO THE NET  ---------------*/

bool Network::connect(String& ssid, String& pwd, int from) {
  NetworkConfig _config;
  
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
//  bool success = wm.autoConnect(constDataPtr->accessPtName.c_str());
  bool success = wm.autoConnect(constDataPtr->accessPtName.c_str());

  if (!success) {
    network.logInfo("Failed to connect to SSID " + ssid + " and portal timed out. Initializing AP fallback state." + conv.fromStr(from));
    
    // Set your existing global status track variables exactly like your old code did
    WiFi.mode(WIFI_AP);
    WiFi.softAP(constDataPtr->accessPtName);
    accessPtIP = WiFi.softAPIP().toString();
    srvIPAddr = accessPtIP;
    connectedSSID = "accessPt";
    AP_createdTime = millis();
    
    network.logInfo("Access point name: " + constDataPtr->accessPtName + conv.fromStr(from));
    network.logInfo("AP IP address: " + accessPtIP + conv.fromStr(from));
    dividerStr(FN, LN);
    return false;
  }
  
  // 4. Connection Success! Update your global variables cleanly
  connectedSSID = WiFi.SSID();
  srvIPAddr = WiFi.localIP().toString();
  
  network.logInfo("WiFi Connection SUCCESSFUL! IP: " + srvIPAddr  + conv.fromStr(from));
  
  return true;
}

/*---------------  PUT THE RIGHT HEADERS INTO A NETWORK INFO LOG  ---------------*/

void Network::logInfo(const String& msg) {
    logging.msg(WHOAMI, T::SYSLOG, L::INFO, ET::NETWORK, msg);
}

/*---------------  PUT THE RIGHT HEADERS INTO A NETWORK ERROR LOG  ---------------*/

void Network::logError(const String& msg) {
    logging.msg(WHOAMI, T::SYSLOG, L::ERROR, ET::NETWORK, msg);
}
