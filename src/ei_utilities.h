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

namespace Text {
  void getAppInfo(JsonDocument& doc, const char* filePath, const char* compileDate);
  String getFileName(const char* filePath);
  void GetAppName(JsonObject app, const String& fileName);
  void getAppVersion(JsonObject app, const String& fileName);
  String formatAppInfo(const JsonDocument& doc);
  String addRuntimeInfo(String banner);
  String pad(const String& s, char padCh, int width);
}
