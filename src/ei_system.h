#pragma once

#include <ei_types.h>
#include <ei_utilities.h>

struct SystemState {
  bool rebootPending = false;
  uint32_t rebootRequestedAt = 0;
  String rebootReason;
  uint32_t freeHeap;
  uint32_t lastFreeHeap;
};

struct SystemConfig {
  bool heapMonitorEnabled = true;
  uint16_t heapMonitorInterval = 60;    // Minutes
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
  void processExternalMsg(const JsonDocument& doc, Source source);             // msgs comming in libraries that receive outside commuications
  void processMsg(const JsonDocument& doc);

private:
private:
  enum class Service {
    Unknown,
    Network,
    Mqtt,
    Time,
    Storage,
    Web,
    Logging
  };
  
  void performReboot();
  SystemState _state;
  SystemConfig _config;
  
  static Service serviceFromString(const char* s);
  static const char* serviceToString(Service service);
  void checkHeap();

};

extern EiSystem eiSystem;

extern bool appHandleMsg(const JsonDocument& doc, Source source);

