#pragma once
//
//  network.hpp
//  
//
//  Created by Stephen McKeon on 7/18/26.
//

#include <ei_types.h>

struct NetworkCredentials {
    bool dirty = false;
    String ssid;
    String password;
};

struct NetworkState {
    String connectedSSID;
    String ipAddress;          // Current active IP
    String accessPtIP;      // IP when acting as an AP
    unsigned long apCreatedTime = 0;
};

class EiNetwork {

public:
  bool startup();

private:
  NetworkCredentials _config;
  NetworkState _state;;
  static constexpr const char* _configFileName = "ei_networkCfg.json";
  
  bool connect(String& ssid, String& pwd, int from);
  void logInfo(const String& msg);
  void logError(const String& msg);
  void aWiFiEvent(WiFiEvent_t event);
  bool readConfigFromDisk();
  JsonDocument createConfigJson(const NetworkCredentials& cfg) const;
  bool checkHardware();
  bool validateConfiguration();
};

extern EiNetwork network;
