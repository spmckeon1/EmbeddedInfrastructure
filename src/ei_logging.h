#pragma once

// -----------------------------------------------------------------------------
// EmbeddedInfrastructure
//
// Module:
//     Logging
//
// Owns:
//
//     - Log generation
//     - Log formatting
//     - Log routing
//     - Log destinations
//
// Does NOT own:
//
//     - Log presentation
//     - Log storage policy
//
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <ei_types.h>
#include <ei_mqtt.h>

#define FI __FILE__           // compiler get file name
#define FN __func__           // compiler get function name
#define LN __LINE__           // what line was the compiler on
#define LS __FILE__, __func__, __LINE__



void logDebug(const char* file,
              const char* function,
              int line,
              const char* eventType,
              const String& msg);

void logInfo(const char* file,
             const char* function,
             int line,
             const char* eventType,
             const String& msg);

void logWarn(const char* file,
             const char* function,
             int line,
             const char* eventType,
             const String& msg);

void logError(const char* file,
              const char* function,
              int line,
              const char* eventType,
              const String& msg);

namespace T {           // Type
  enum Type {
    SYSLOG,         // Diagnostic text
    TEMPERATURE,    // Temperature measurement
    METRIC,         // Numeric measurement
    GPS,            // GPS fix/position
    POWER,          // Electrical measurements
    NETWORK,        // Network statistics
    EVENT           // Structured application event
  };

  inline constexpr const char* typeNames[] = {
    "SYSLOG",
    "TEMPERATURE",
    "METRIC",
    "GPS",
    "POWER",
    "NETWORK",
    "EVENT"
  };
  const char* toTxt(Type type);
  String toStr(Type type);
}

namespace L {           // Level
  enum Level {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
  };
  inline constexpr const char* levelNames[] = {   // Keep this table in the same order as the Level enum.
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
  };
  const char* toTxt(Level level);
  String toStr(Level level);
}

namespace ET
{
  inline constexpr const char GENERAL[]   = "GENERAL";

  inline constexpr const char LOGGING[]   = "LOGGING";
  inline constexpr const char STORAGE[]   = "STORAGE";
  inline constexpr const char NETWORK[]   = "NETWORK";
  inline constexpr const char MQTT[]      = "MQTT";
  inline constexpr const char WIFI[]      = "WIFI";
  inline constexpr const char TIME[]      = "TIME";
  inline constexpr const char OTA[]       = "OTA";
  inline constexpr const char WEB[]       = "WEB";
  inline constexpr const char SYSTEM[]    = "SYSTEM";

  inline constexpr const char SENSOR[]    = "SENSOR";

  inline constexpr const char MEMORY[]    = "MEMORY";
  inline constexpr const char CONFIG[]    = "CONFIG";
  inline constexpr const char STARTUP[]   = "STARTUP";
  inline constexpr const char SHUTDOWN[]  = "SHUTDOWN";

  inline constexpr const char USER[]      = "USER";
  inline constexpr const char COMMAND[]   = "COMMAND";
}

struct LoggingConfig {
  Topic topic{"/ei/to/nr/logs"};
  QoS qos = QOS0;
  Retain retain = FORGET;
};

enum class LogDestination {
  RamBuffer,
  MqttServer
};

class Logging {
private:

  LoggingConfig _config;
  LogDestination _dest = LogDestination::RamBuffer;
  
  static constexpr size_t MAX_PENDING_LOGS = 100;
  String _pendingLogQueue[MAX_PENDING_LOGS];
  uint16_t _queueHead = 0;
  uint16_t _queueTail = 0;
  uint16_t _queueCount = 0;
  bool enqueuePendingLog(const String& jsonLog);
  bool dequeuePendingLog(String& jsonLog);
  bool pendingLogsEmpty() const;
  bool pendingLogsFull() const;
//  bool flushPendingLogs();
  bool flushOnePendingLog();
  
  String formatSerialLogEntry(const char* file,
                              const char* function,
                              int lineNum,
                              const String& msg);
  String formatJsonStrLogEntry(const char* file,
                               const char* function,
                               int lineNum,
                               T::Type recordType,
                               L::Level level,
                               const char* eventType,
                               const String& message);
  bool sendToNodeRedLogging(const String& logEntry);
  const char* baseFileName(const char* file);
  

public:

  uint8_t registerEventType(const char* name);
  const char* getEventTypeName(int typeId);
  bool evtLoop();
  void msg(
           const char* file,
           const char* function,
           int lineNum,
           T::Type recordType,
           L::Level level,
           const char* eventType,
           const String& message
           );
  String dividerStr(const String& function, int line);
  void startup();
  void setDestination(LogDestination destination);
  const char* destinationToString(LogDestination dest) const;

};

extern Logging logging;


