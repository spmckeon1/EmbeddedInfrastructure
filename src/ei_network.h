#pragma once
//
//  network.hpp
//  
//
//  Created by Stephen McKeon on 7/18/26.
//

#include <WiFi.h>
#include <esp_wifi.h>

#include <ei_appFramework.h>
#include <ei_logging.h>
#include <ei_storage.h>
#include <ei_types.h>

class Logging; // Tells the compiler that the Logging class exists elsewhere

enum class NetworkMode {
    CONNECTING,
    CONNECTED,
    PROVISIONING
};

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


class EiNetwork {

public:
  EiNetwork(); // 💡 FIXED: Missing constructor signature added here
  bool setup();
  bool startup();
  bool evtLoop();
  bool isConnected() const;
  String getIPAddress() const;
  const NetworkConfig& config() const;
  bool configureFromJson(const JsonDocument& doc);
  bool configure(const NetworkConfig& cfg);
  void processMsg(const JsonDocument& doc);
  JsonDocument getWifiConfigMsg();

private:
  NetworkConfig _config;
  NetworkState _state;;
  String _configFileName;
  bool _provisioning = false;
  static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15UL * 1000UL;
  static constexpr uint32_t PROVISIONING_TIMEOUT_MS = 5UL * 60UL * 1000UL;
  NetworkMode _mode = NetworkMode::CONNECTING;
  uint32_t _connectStartTime = 0;
  uint32_t _provisioningStartTime = 0;

  void aWiFiEvent(WiFiEvent_t event);
  void onWifiGotIP(WiFiEventInfo_t info);
  void onWifiDisconnect(WiFiEventInfo_t info);
  
  bool readConfigFromDisk();
  JsonDocument createConfigJson(const NetworkConfig& cfg) const;
  bool checkHardware();
  bool validateConfiguration();
  Storage::WriteResult writeConfigToDisk();
  void startProvisioningAP();
  void stopProvisioningAP();
  
};

extern EiNetwork network;


