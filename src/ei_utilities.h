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

#include <ei_types.h>

enum class Source {
  NOT_YET_SET = -1,
  WEB,
  NODE_RED,
  APP_STARTUP
};

namespace AppInfo {
  void GetAppName(JsonObject app, const String& fileName);
  void getAppInfo(JsonDocument& doc, const char* filePath, const char* compileDate);
  String formatAppInfo(const JsonDocument& doc);
  void getAppVersion(JsonObject app, const String& fileName);
  String addRuntimeInfo(String banner);
}

namespace Json {
  String jsonToString(const JsonDocument& doc);
}

namespace Text {
  String stripPath(const char* filePath);
  String pad(const String& s, char padCh, int width);
  String sourceToStr(Source source);

}
