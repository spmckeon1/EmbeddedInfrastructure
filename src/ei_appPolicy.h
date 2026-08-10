#pragma once

//#include <ezTime.h>
#include <ei_types.h>
#include <ei_mqtt.h>

struct AppDirPolicy {
  String libCfgDir = "/libCfg";
  String dataDir   = "/libData";
  String logDir    = "/libLog";
  String appData   = "/appData";
  String htmlDir   = "/html";
};

struct AppFnamePolicy {
    String bootTime;
};

struct AppIDs {
  const char* appName;
  const char* shortName;
  const char* accessPointName;
  const char* pageTitle;
  const char* pageHeader;
  const char* uploadPage;
};

struct MqttLwtPolicy {
  bool enabled = false;
  Topic topic{"default/lwt/topic"};
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

extern AppDirPolicy appDirs;
extern AppFnamePolicy appFnames;
extern AppIDs appIDs;
extern MqttLwtPolicy appMqttLwtPolicy;
extern MqttHeartbeatPolicy mqttHbPolicy;
