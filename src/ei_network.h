#pragma once
//
//  network.hpp
//  
//
//  Created by Stephen McKeon on 7/18/26.
//

#include <WiFi.h>
#include <WiFiManager.h>  // 💡 Fixes: 'WiFiManager' / 'wm' was not declared in this scope
#include <esp_wifi.h>

#include <ei_appPolicy.h>
#include <ei_logging.h>
#include <ei_storage.h>
#include <ei_types.h>

class Logging; // Tells the compiler that the Logging class exists elsewhere

/*
struct NetworkCredentials {
    bool dirty = false;
    String ssid = "";
    String password = "";
};
*/

struct NetworkConfig {
    bool dirty = false;
    String ssid;
    String password;
    wifi_power_t txPower = WIFI_POWER_8_5dBm;
};

struct StationState
{
    bool   connected = false;
    String ssid;
    String ipAddress;
};

struct AccessPointState
{
    bool   active = false;
    String ipAddress;
    uint32_t createdTime = 0;
};

struct NetworkState
{
    StationState     sta;
    AccessPointState ap;
};


class WiFiManagerLogBridge : public Print {             // 1. The custom stream bridge that intercepts character arrays
private:
    String buffer;
public:
  WiFiManagerLogBridge() { buffer.reserve(128); }

  size_t write(uint8_t c) override {
    if (c == '\n') {
      flushBuffer();
    } else if (c != '\r') {
      buffer += (char)c;
    }
    return 1;
  }

  size_t write(const uint8_t *buffer, size_t size) override {
      size_t n = 0;
      while (size--) { n += write(*buffer++); }
      return n;
  }

private:
    void flushBuffer() {
        if (buffer.length() > 0) {
          buffer.trim();
          // Routes directly to your custom logging class setup
          logging.msg(
                       "WiFiManager.h",
                       "autoConnect",
                       0,
                       T::SYSLOG,
                       L::INFO,
                       ET::NETWORK, // Only keep this one event type parameter!
                       buffer
                       );
          buffer = "";
        }
    }
};

class EiNetwork {

public:
  EiNetwork(); // 💡 FIXED: Missing constructor signature added here
  bool setup();
  bool startup();
  bool evtLoop();
  bool isConnected() const;
  String getIPAddress() const;

private:
  NetworkConfig _config;
  NetworkState _state;;
  // ORDER MATTERS HERE: wmLoggerBridge must be declared BEFORE wm
  // so that it initializes first in memory.
  WiFiManagerLogBridge wmLoggerBridge;
  WiFiManager wm;
  String _configFileName;

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
  Storage::WriteResult writeConfigToDisk();
  JsonDocument createConfigJson(const NetworkConfig& cfg) const;
  bool checkHardware();
  bool validateConfiguration();
  static void saveConfigCallback();
};

extern EiNetwork network;


