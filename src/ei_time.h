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
//#include <ezTime.h>

struct TimeConfig {       // Default to U.S. Mountain Time. Applications may override as needed.
    String abbreviation = "MDT";
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
  TimeConfig _config;
  TimeState  _state;
//  TimeStats  _stats;
//  Timezone   _tz;
  

public:
  bool begin();

  bool isReady() const;
  bool posixRuleChanged() const;
  String getPosixRule() const;
  String getLogTimeStamp();
};

extern EiTime eiTime;
