#pragma once
//
//  network.hpp
//  
//
//  Created by Stephen McKeon on 7/18/26.
//

#include <WiFi.h>
#include <WiFiManager.h>  // 💡 Fixes: 'WiFiManager' / 'wm' was not declared in this scope
#include <ei_appPolicy.h>
#include <esp_wifi.h>
#include <ei_types.h>

struct NetworkCredentials {
    bool dirty = false;
    String ssid = "";
    String password = "";
};

struct NetworkState {
    String connectedSSID;
    String ipAddress;                 // Current active IP
    String accessPtIP;                // IP when acting as an AP
    unsigned long apCreatedTime = 0;
};

class EiNetwork {

public:
  bool setup();
  bool startup();
  bool evtLoop();

private:
  NetworkCredentials _config;
  NetworkState _state;;
  WiFiManager wm;                                                     // 2. Initialize WiFiManager
  String _configFileName;

  bool connect(String& ssid, String& pwd, int from);
  void initAsyncPortal(const char* apName);
  void aWiFiEvent(WiFiEvent_t event);
  void onWifiGotIP(WiFiEventInfo_t info);
  void onWifiDisconnect(WiFiEventInfo_t info);
  
  void logInfo(const char* function,
               int lineNum,
               const String& msg);
  void logError(const char* function,
                int lineNum,
                const String& msg);
  bool readConfigFromDisk();
  JsonDocument createConfigJson(const NetworkCredentials& cfg) const;
  bool checkHardware();
  bool validateConfiguration();
};

extern EiNetwork network;
