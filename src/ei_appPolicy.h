#pragma once

// Module:
//     Application Policy
#include <ezTime.h>
#include <ei_types.h>

struct AppDirPolicy {
  String libCfgDir = "/libCfg";
  String dataDir   = "/libData";
  String logDir    = "/libLog";
  String appData   = "/appData";
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
    bool enabled      = false;
    String topic      = "default/lwt/topic";
    String onlineMsg  = "Online";
    String offlineMsg = "Offline";
    uint8_t qos       = 1;
    bool retain       = true;
};

struct MqttHeartbeatPolicy {
    bool enabled      = true;
    String topic      = "default/heartbeat";
    uint32_t interval = 60000;
    bool retain       = false;
};

extern AppDirPolicy appDirs;
extern AppFnamePolicy appFnames;
extern AppIDs appIDs;
extern MqttLwtPolicy appMqttLwtPolicy;
extern MqttHeartbeatPolicy mqttHbPolicy;
