#pragma once

#include <ei_types.h>

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
  void handleMsg(const JsonDocument& doc);
  void requestReboot(const String& reason, bool immediate = false);
  void getFreeHeap();
  void enableHeapMonitor(bool enabled);
  void setHeapMonitorInterval(uint16_t minutes);

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
