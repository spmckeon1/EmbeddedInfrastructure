#pragma once

// -----------------------------------------------------------------------------
// EmbeddedInfrastructure
//
// Module:
//     Time
//
// Owns:
//
//     - Time synchronization
//     - Time formatting
//     - Uptime
//     - Time-related utilities
//
// Does NOT own:
//
//     - Scheduling
//     - Timers
//
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <ei_types.h>
#include <ei_configuration.h>

struct TimeConfig {       // Default to U.S. Mountain Time. Applications may override as needed.
    String olsonName = "America/Denver";
    String posixRule = "MST7MDT,M3.2.0,M11.1.0";
};

struct TimeState {
  bool timeValid;
  bool ready;
  String abbreviation;
  bool posixChanged;
  
};

class EiTime
{
private:
  static constexpr const char* _configFileName = "ei_timeCfg.json";
  TimeConfig _config;
  TimeState  _state;
  
  bool eventLoop();
  bool readConfigFromDisk();
  bool writeConfigToDisk();
  JsonDocument createEiTimeCfgJson() const;

public:
  bool begin();

  bool isReady() const;
  bool posixRuleChanged() const;
  String getPosixRule() const;
  String getLogTimeStamp();
};

extern EiTime eiTime;
