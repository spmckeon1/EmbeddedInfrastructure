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
}

namespace ET {          // Event Type / Category
  enum Type {
    GENERAL,

    STORAGE,        // Infrastructure
    NETWORK,
    MQTT,
    WIFI,
    TIME,

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
}
struct WhoAmI {                   // contains log msg file name, code line number, and function name
    const char* file;
    int line;
    const char* function;
};

// allows the user to fill in the WhoAmI struct
#define WHOAMI WhoAmI{__FILE__, \
                      __LINE__, \
                      __func__}

class Logging {
private:
  String doSerialMonLogEntry(String event, String functionName, int lineNo);
  String doJsonStrLogEntry(const WhoAmI& whoAmI, T::Type recordType, L::Level level, ET::Type eventType, const String& message);
  void sendToSyslog(String s);
  
public:

  void msg(const WhoAmI& whoAmI, T::Type recordType, L::Level level, ET::Type eventType, const String& message);
  void msg(const String& event, const String& functionName, int lineNo);
  String dividerStr(const String& function, int line);
};

extern Logging logging;
