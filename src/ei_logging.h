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

#define FN __func__           // compiler get function name
#define LN __LINE__               // what line was the compiler on

enum class LogDestination {
    RamBuffer,
    MqttServer
};

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

namespace ET {          // Event Type / Category
  enum Type {
    GENERAL,

    STORAGE,        // Infrastructure
    NETWORK,
    MQTT,
    WIFI,
    TIME,
    LOGGING,

    GPS,            // Hardware
    ET_SERIAL,
    SENSOR,

    MEMORY,           // System
    CONFIG,
    STARTUP,
    SHUTDOWN,

    USER,             // Application
    COMMAND
  };

  inline constexpr const char* eventTypeNames[] = {     // Keep this table in the same order as the Type enum.
    "GENERAL",

    "STORAGE",
    "NETWORK",
    "MQTT",
    "WIFI",
    "TIME",

    "GPS",
    "SERIAL",
    "SENSOR",

    "MEMORY",
    "CONFIG",
    "STARTUP",
    "SHUTDOWN",

    "USER",
    "COMMAND"
  };
  const char* toTxt(Type type);
  String toStr(Type type);
}


class Logging {
private:
  LogDestination _dest = LogDestination::RamBuffer;
  String formatSerialLogEntry(const char* file,
                              const char* function,
                              int lineNum,
                              const String& msg);
  String formatJsonStrLogEntry(const char* file,
                               const char* function,
                               int lineNum,
                               T::Type recordType,
                               L::Level level,
                               ET::Type eventType,
                               const String& message);
  void sendToNodeRedLogging(String logEntry);
  void logInfo(const char* function,
               int lineNum,
               const String& msg);
  void logError(const char* function,
                int lineNum,
                const String& msg);

public:

  void msg(
           const char* file,
           const char* function,
           int lineNum,
           T::Type recordType,
           L::Level level,
           ET::Type eventType,
           const String& message
           );
  String dividerStr(const String& function, int line);
  void startup();
  void setDestination(LogDestination destination);

};

extern Logging logging;
