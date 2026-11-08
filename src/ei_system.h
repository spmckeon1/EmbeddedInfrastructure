#pragma once

#include "config.h"

#include <ei_types.h>
#include <ei_scheduler.h>
#include <ei_utilities.h>

// EI MQTT message topics
#define EI_MQTT_TOPIC_SUBSCRIPTION   "nr/to/ei/#"
#define EI_MQTT_INBOUND_GLOBAL    "nr/to/ei"
#define EI_MQTT_OUTBOUND_GLOBAL "ei/to/nr"

#ifdef BUILD_TEST
  #define INITIAL_HEAP_INTERVAL 5
#elif defined(BUILD_PRODUCTION)
  #define INITIAL_HEAP_INTERVAL 60
#endif

struct SystemState {
  bool rebootPending = false;
  uint32_t rebootRequestedAt = 0;
  String rebootReason;
  uint32_t freeHeap;
  uint32_t lastFreeHeap;
};

struct SystemConfig {
  bool heapMonitorEnabled = true;
  RunTime heapTimer;
  uint16_t heapMonitorInterval = INITIAL_HEAP_INTERVAL;  // Minutes
};

class EiSystem {
public:
  void evtLoop();
  bool bootStrap();
  bool setup();
  bool startup();
  void processLibraryMsg(const JsonDocument& doc);
  void routeOutboundMsg(const JsonDocument& doc);
  void requestReboot(const String& reason, bool immediate = false);
  void getFreeHeap();
  void enableHeapMonitor(bool enabled);
  void setHeapMonitorInterval(uint16_t minutes);
  uint16_t getHeapMonitorInterval() const;
  bool getHeapMonitorEnabled() const;
  void processExternalMsg(const JsonDocument& doc);             // msgs comming in libraries that receive outside commuications
//  void processMsg(const JsonDocument& doc);

private:
private:
  enum class Service {
    Unknown,
    Network,
    Mqtt,
    Time,
    Storage,
    Web,
    Logging,
    appFramework
  };
  
  void performReboot();
  SystemState _state;
  SystemConfig _config;
  String _rebootReasonFname;
  String _configFileName;
  
  void doGetGlobalCfg();
  void hdlRebootReq(const JsonDocument& doc);
  void hdlGlobalReq(const JsonDocument& doc);
  void processMsg(const JsonDocument& doc);
  static Service serviceFromString(const char* s);
  static const char* serviceToString(Service service);
  void checkHeap();
  bool loadConfig();
  bool saveConfig();

};

extern EiSystem eiSystem;

extern bool appHandleMsg(const JsonDocument& doc);
extern void processTimeActive();

