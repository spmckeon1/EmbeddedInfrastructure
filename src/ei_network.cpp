//
//  network.cpp
//  
//
//  Created by Stephen McKeon on 7/18/26.
//

#include <Arduino.h>
#include "esp_mac.h" // Required for ESP-IDF MAC functions
#include <ei_appPolicy.h>
#include <ei_events.h>
#include <ei_scheduler.h>
#include <ei_conversion.h>
#include <ei_storage.h>
#include <ei_network.h>
#include <ei_web.h>

/*-----  CLASS CONSTRUCTOR  -----*/
// This wires your custom log bridge directly into WiFiManager before boot
EiNetwork::EiNetwork()
  : wmLoggerBridge(),
    wm(wmLoggerBridge)
{
    _configFileName = "/network_cfg.json";
}


EiNetwork network;

/*-----  SETUP THE NETWORK SUBSYSTEM  -----*/

                            
                            // Prepare the Network subsystem for startup.
bool EiNetwork::setup()     // This phase may not depend on services provided by other subsystems.
{
  wm.setDebugOutput(true);
  wm.setSaveConfigCallback(EiNetwork::saveConfigCallback);
  _configFileName = appDirs.libCfgDir + "/ei_networkCfg.json";
  WiFi.onEvent([this](WiFiEvent_t, WiFiEventInfo_t info) {            // Register WiFi event handlers.
    onWifiGotIP(info);
  }, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent([this](WiFiEvent_t, WiFiEventInfo_t info) {
    onWifiDisconnect(info);
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  return checkHardware();
  return true;
}

/*-------------------------  STARTUP NETWORK SUBSYSTEM  -------------------------*/

bool EiNetwork::startup()
{
  logInfo(LS, ET::NETWORK, "Initializing Network infrastructure...");
  storage.ensureFileExists(_configFileName,                           // Ensure the configuration file exists.
                         createConfigJson(_config), LN);
  if (!readConfigFromDisk()) {                                          // Load persisted configuration.
    logError(LS, ET::NETWORK, "Unable to load disk config layout.");
    return false;
  }
  if (!validateConfiguration()) {                                     // Validate configuration for logging purposes.
    logError(LS, ET::NETWORK,                                                  // An unprovisioned device is not a startup failure.
             "Network configuration is empty or unprovisioned.");
  }
  wm.setConfigPortalBlocking(false);                                      // 1. Prevent WiFiManager from halting your loop
  wm.setConnectTimeout(15);                                               // 2. Set how long it tries connecting to the router before opening the portal
  wm.setConfigPortalTimeout(180);                                         // 3. Set how long the portal stays open before automatically closing (3 minutes)
  wm.setClass("invert");                                                  // 4. Custom Styling
  wm.setCustomHeadElement(
    "<style>body{background-color:#121212; color:#ffffff;}</style>");
  logInfo(LS, ET::NETWORK, "Starting WiFiManager...");                             // Start the connection process.
  wm.autoConnect(appIDs.accessPointName);                                 // 5. Fire off the background connection/portal attempt, This returns instantly instead of waiting for a connection
  logInfo(LS, ET::NETWORK, "Network subsystem started.");
  return true;
}

/*-------------------------  NETWORK EVENT LOOP  -------------------------*/

bool EiNetwork::evtLoop() {
  static RunTime cfgWriteTimer = {IntervalType::IT_SECOND, 1, -1}; // declare the RunTime struct
  if(scheduler.isTimeToRun(cfgWriteTimer) && _config.dirty) {      // if it is time to run a _config.dirty check and _config.dirty is dirty then
    writeConfigToDisk();                                        // write the new _config to dsk.
  }
  wm.process();
  return false;
}

/*---------------  EVENT HANDLER: OBTAINED NETWORK IP  ---------------*/

void EiNetwork::onWifiGotIP(WiFiEventInfo_t info) {
  _state.sta.connected = true;
  _state.sta.ipAddress = WiFi.localIP().toString();
  _state.sta.ssid = WiFi.SSID();
  if (_config.ssid.isEmpty() || _config.password.isEmpty()) {
    _config.ssid = WiFi.SSID();
    _config.password = WiFi.psk(); // Pulls the cached network key/passphrase
    _config.dirty = true;
  }
  logInfo(LS, ET::NETWORK, "Event 7: Network connection verified. Active IP: " + _state.sta.ipAddress);
  eiEvents.notify(EiEvent::WifiConnected);
}

/*---------------  EVENT HANDLER: ROUTER DISCONNECTED  ---------------*/

void EiNetwork::onWifiDisconnect(WiFiEventInfo_t info) {
  uint8_t reason = info.wifi_sta_disconnected.reason;
  _state.sta.connected = false;
  _state.sta.ipAddress = "";
  _state.sta.ssid = "";
  logError(LS, ET::NETWORK, "Event 5: Router connection lost. Reason code: " + String(reason));
  eiEvents.notify(EiEvent::WifiDisconnected);
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
    logInfo(LS, ET::NETWORK, response);
}

/*-------------------------  LOAD NET CREDENTIALS FROM DISK  -------------------------*/

bool EiNetwork::readConfigFromDisk() {
  JsonDocument doc;
  if (!storage.readJsonFile(_configFileName.c_str(), doc, LN))
    return false;
  _config.ssid     = doc["ssid"].as<String>();
  _config.password = doc["password"].as<String>();
  return true;
}

/*-------------------------  WRITE THE _config SSID AND PWD TO DISK  -------------------------*/

Storage::WriteResult EiNetwork::writeConfigToDisk()
{
    JsonDocument doc;
    doc["ssid"]     = _config.ssid;
    doc["password"] = _config.password;

    Storage::WriteResult result =
        storage.writeJsonFile(_configFileName.c_str(), doc, LN);

    if (result == Storage::WriteResult::Success)
    {
        _config.dirty = false;
    }

    return result;
}
/*-----  CREATE THE CONFIG JSON OBJECT FROM THE cfg CONTENTS  -----*/

JsonDocument EiNetwork::createConfigJson(const NetworkConfig& cfg) const {
    JsonDocument doc;
    doc["ssid"] = cfg.ssid;
    doc["password"] = cfg.password;
    return doc;
}

/*-----  BOOT UP WIFI HARDWARE CHECK  -----*/

bool EiNetwork::checkHardware() {
  uint8_t mac_bytes[6];
  esp_err_t err = esp_efuse_mac_get_default(mac_bytes);               // Reads the factory-burned base MAC address directly from eFuse
  if (err != ESP_OK) {
    logError(LS, ET::NETWORK, "Wi-Fi hardware driver failed to initialize.");
    return false;
  }
  char mac_str[18];                                                     // Format bytes into a standard MAC string for validation
  snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_bytes[0], mac_bytes[1], mac_bytes[2], mac_bytes[3], mac_bytes[4], mac_bytes[5]);
  String mac = String(mac_str);
  if (mac == "00:00:00:00:00:00" || mac.length() == 0) {
    logError(LS, ET::NETWORK, "Wi-Fi hardware check failed: Invalid MAC address.");
    logInfo(LS, ET::NETWORK, "MAC address returned: '" + mac + "'");
    return false;
  }
  logInfo(LS, ET::NETWORK, "Wi-Fi hardware is verified and ready. MAC: " + mac);
  return true;
}

  /*-----  VALIDATE THE WIFI CONFIGURATION  -----*/

  bool EiNetwork::validateConfiguration() {
      if (_config.ssid.isEmpty()) {
        logError(LS, ET::NETWORK, "Network SSID is not configured.");
          return false;
      }
      // WPA2 passwords must be at least 8 characters.
      // Allow empty passwords in case the user intentionally connects
      // to an open network.
      if (!_config.password.isEmpty() && _config.password.length() < 8) {
        logError(LS, ET::NETWORK, "Wi-Fi password must be at least 8 characters.");
        return false;
      }

      return true;
  }

/*
void EiNetwork::saveConfigCallback() {
    // 1. Log that the user submitted data
    network.logInfo(__FUNCTION__, __LINE__, "New Wi-Fi credentials submitted via portal!");

    // 2. Fetch the newly entered credentials from WiFiManager and update _config
    network._config.ssid = network.wm.getWiFiSSID();
    network._config.password = network.wm.getWiFiPass();
    network._config.dirty = true;

    // 3. Generate your updated configuration JSON string
    JsonDocument doc = network.createConfigJson(network._config);
    String jsonStr;
    serializeJson(doc, jsonStr);

    // 4. Overwrite your file on disk using your existing storage system layout
  Storage::WriteResult result = storage.writeFile(network._configFileName.c_str(), jsonStr.c_str(), LN);
  if (result == Storage::WriteResult::Success) {
        network.logInfo(__FUNCTION__, __LINE__, "Successfully saved new network credentials to disk.");
    } else {
        network.logError(__FUNCTION__, __LINE__, "Failed writing new credentials to disk.");
    }
}
*/
/*-----  IS WIFI CONNECTED  -----*/

bool EiNetwork::isConnected() const {
    return _state.sta.connected;
}

/*-----  PUBLIC ROUTINE TO GET THE IN USE IP ADDRESS  -----*/

String EiNetwork::getIPAddress() const {
    return _state.sta.ipAddress;
}

/*-----  PUBLIC: ALLOW EXTERNAL AGENT TO SEND A NEW NETWORK WIFI CFG IN  -----*/

bool EiNetwork::configure(const NetworkConfig& cfg) {
  if (cfg.ssid.isEmpty()) {                             // Validate the configuration before accepting it.
    logError(LS, ET::NETWORK, "WiFi SSID may not be empty.");
    return false;
  }
  _config = cfg;
  _config.dirty = true;
  Storage::WriteResult result = writeConfigToDisk();
  if (result != Storage::WriteResult::Success) {
    logError(LS, ET::NETWORK, "Unable to save network configuration.");
    return false;
  }
  logInfo(LS, ET::NETWORK, "WiFi configuration updated.");
  return true;
}

/*-----  PUBLIC: ALLOW EXTERNAL AGENT TO SEND A NEW NETWORK WIFI JSON DOC IN  -----*/

bool EiNetwork::configureFromJson(const JsonDocument& doc) {
  NetworkConfig cfg = _config;
  if (doc["data"]["ssid"].is<String>())
    cfg.ssid = doc["data"]["ssid"].as<String>();
  if (doc["data"]["password"].is<String>())
    cfg.password = doc["data"]["password"].as<String>();
  return configure(cfg);
}

/*-----  PUBLIC: ALLOW EXTERNAL AGENT SEE THE NETWORK WIFI CFG  -----*/

const NetworkConfig& EiNetwork::config() const {
    return _config;
}



void EiNetwork::saveConfigCallback()
{
    logInfo(LS, ET::NETWORK, "New Wi-Fi credentials submitted via portal!");

    NetworkConfig cfg = network._config;
    network.updateConfigFromWiFiManager(cfg);

    if (network.configure(cfg))
    {
        logInfo(LS, ET::NETWORK, "Successfully saved new network credentials to disk.");
    }
    else
    {
        logError(LS, ET::NETWORK, "Failed writing new credentials to disk.");
    }
}

/*-----  ***  -----*/

void EiNetwork::updateConfigFromWiFiManager(NetworkConfig& cfg)
{
    cfg.ssid     = wm.getWiFiSSID();
    cfg.password = wm.getWiFiPass();
    cfg.dirty    = true;
}

/*-----  PROCESS AN INCOMING MSG  -----*/

void EiNetwork::processMsg(const JsonDocument& doc) {
  String route = doc["route"].as<String>();
  String command = doc["command"].as<String>();

  if (route == "network/wifi/cfg") {

    if (command == "SET") {
      JsonDocument response;
      response["owner"] = "library";
      response["route"] = "network/wifi/cfg";
      response["command"] = "RESULT";
      JsonObject data = response["data"].to<JsonObject>();
      if (configureFromJson(doc)) {
        data["success"] = true;
        data["message"] = "WiFi configuration saved.";
      } else {
        data["success"] = false;
        data["message"] = "WiFi configuration was not saved.";
      }
      web.webPubMsg(response);
      return;
    }
    if (command == "GET") {
      JsonDocument response = getWifiConfigMsg();
      web.webPubMsg(response);
      return;
    }

    logError(LS, ET::NETWORK, "Unknown command '" + command + "' for route '" + route + "'.");
    return;
  }

  logError(LS, ET::NETWORK, "Unknown Network route '" + route + "'.");
}

/*-----  PROCESS AN INCOMING MSG  -----*/

JsonDocument EiNetwork::getWifiConfigMsg() {
    JsonDocument response;

    response["owner"] = "library";
    response["route"] = "network/wifi/cfg";
    response["command"] = "GET";

    JsonObject data = response["data"].to<JsonObject>();

    data["ssid"] = _config.ssid;
    data["password"] = _config.password;

    return response;
}

