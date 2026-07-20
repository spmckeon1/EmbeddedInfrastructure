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
}

namespace L {
    enum Level {
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };
}

namespace ET {          // Event Type / Category
    enum Type {
        GENERAL,

        // Infrastructure
        STORAGE,
        NETWORK,
        MQTT,
        WIFI,
        TIME,

        // Hardware
        GPS,
        ET_SERIAL,
        SENSOR,

        // System
        MEMORY,
        CONFIG,
        STARTUP,
        SHUTDOWN,

        // Application
        USER,
        COMMAND
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
