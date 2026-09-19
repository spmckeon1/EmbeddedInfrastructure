#pragma once

// -----------------------------------------------------------------------------
// EmbeddedInfrastructure
//
// Module:
//     Utilities
//
// Owns:
//
//     - General-purpose helper functions that have no natural home elsewhere
//
// Goal:
//
//     - Keep this module as small as practical.
//     - Move functionality into a more appropriate module whenever possible.
//
// -----------------------------------------------------------------------------

#include <Arduino.h>
#include <ArduinoJson.h>

#include <ei_types.h>

namespace AppInfo {
  void GetAppName(JsonObject app, const String& fileName);
  void getAppInfo(JsonDocument& doc, const char* filePath, const char* compileDate);
  String formatAppInfo(const JsonDocument& doc);
  void getAppVersion(JsonObject app, const String& fileName);
  String addRuntimeInfo(String banner);
}

namespace Json {
  String jsonToString(const JsonDocument& doc);
  void missingField(const char* eventType, const String& field, const String& json);
}

namespace Text {
  String stripPath(const char* filePath);
  String pad(const String& s, char padCh, int width);
  String sourceToStr(Source source);

}

namespace GPIO {
  bool readPin(uint8_t pin, bool lastState, uint8_t debounceTime);
}

namespace MATH {
unsigned long suli(unsigned long minuend, unsigned long subtrahend);
}

namespace TEMP {
float applyHysteresisF(float newTemp, float currentTemp, float hysteresis);
}

