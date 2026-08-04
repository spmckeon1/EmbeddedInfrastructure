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
#include <ArduinoJson.h>
#include <ezTime.h>
#include <ei_types.h>
#include <ei_storage.h>

enum class DurFormat {
    COMPACT,
    PRETTY,
    VERBOSE
};

struct TimeConfig {       // Default to U.S. Mountain Time. Applications may override as needed.
  bool dirty = true;
  String posixRule = "MST7MDT,M3.2.0,M11.1.0";
};

struct PendingTimeConfig {
    bool   pending = false;
    String posixRule;
};

class EiTime {
public:
  bool setup();
  bool evtLoop();
  uint8_t  second();
  uint8_t  minute();
  uint8_t  hour();
  uint8_t  hour12();

  uint8_t  day();
  uint8_t  weekday();
  uint8_t  month();
  uint16_t year();

  uint16_t millisecond();

  bool isReady() const;
  bool posixRuleChanged() const;
  String getPosixRule() const;
  String getLogTimeStamp();
  time_t now();
  bool setPosixRule(const String& rule);
  String formatDuration(uint32_t ms, DurFormat format);
  time_t getBootTime() const;
  void processMsg(const JsonDocument& doc);

private:
  String _configFileName;
//  static constexpr const char* _configFileName = "/ei_timeCfg.json";
  TimeConfig _config;
  PendingTimeConfig _pending;
  Timezone _tz;                           // the ezTime time zone struct
  
  void syncTime();
  bool setTimeZone();
  bool readConfigFromDisk();
  Storage::WriteResult writeConfigToDisk();
  JsonDocument createConfigJson(const TimeConfig& cfg) const;
  void loadConfigFromJson(const JsonDocument& doc, TimeConfig& cfg) const;
  bool validateConfiguration(const TimeConfig& cfg);
  void saveBootTime();
  String formatLogTime();
  
  static constexpr const char* LOG_TIME_FORMAT =
      "Y-m-d~ H:i:s.v-T";

  static constexpr const char* DISPLAY_TIME_FORMAT =
      "g:i:s A";

  static constexpr const char* DISPLAY_DATE_FORMAT =
      "l, F j, Y";

  String formatLogTime() const;

  
};

extern EiTime eiTime;
