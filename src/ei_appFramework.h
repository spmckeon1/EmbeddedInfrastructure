#pragma once

#include <ArduinoJson.h>

//#include <ezTime.h>
#include <ei_types.h>

#if CONFIG_IDF_TARGET_ESP32
    constexpr uint8_t DEVICE_IS_RUNNING = 2;
#elif CONFIG_IDF_TARGET_ESP32C3
    constexpr uint8_t DEVICE_IS_RUNNING = 8;
#else
    #error "Unsupported ESP32 target"
#endif



struct AppDirPolicy {
  String libCfgDir = "/libCfg";
  String dataDir   = "/libData";
  String logDir    = "/libLog";
  String appData   = "/appData";
  String htmlDir   = "/html";
};

struct AppIDs {
  const char* appName;
  const char* sourceId;
  const char* pageId;
  const char* accessPointName;
  const char* pageTitle;
  const char* pageHeader;
  const char* uploadPage;
  const char* mqttTopic;
  String deviceId;
};

struct MqttLwtPolicy {
  bool enabled = false;
  String topic = "default/lwt/topic";
  String onlineMsg = "Online";
  String offlineMsg = "Offline";
  uint8_t qos = 1;
  bool retain = true;
};

struct MqttHeartbeatPolicy {
  bool enabled = true;
  uint32_t interval = 60000;
  uint32_t timeout  = 180000;  // Seconds before considered offline
};

struct AppFname {
    String bootTime;
    String deviceId;
};

struct AppLibraryConfig {
    uint8_t maxWebClients = 4;

    // future application-defined library values go here
};

class AppFramework
{
public:
  bool startup();

  void processMsg(const JsonDocument& doc);
  String buildJsonAppMqttMsg(const String& route, const String& command, const JsonObjectConst& data);
  
private:
  bool getDeviceId();
  void initSetupPage(const JsonDocument& doc);

};

extern AppFramework appFramework;


extern AppDirPolicy appDirs;
extern AppFname appFnames;
extern AppIDs appIDs;
extern MqttLwtPolicy appMqttLwtPolicy;
extern MqttHeartbeatPolicy mqttHbPolicy;
extern AppLibraryConfig appLibraryConfig;
