
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ei_time.h>
#include <ei_network.h>
#include <ei_utilities.h>

namespace Text
{

/*-----  ASSEMBLE THE DESIRED INFORMATION  -----*/

  void getAppInfo(JsonDocument& doc, const char* filePath, const char* compileDate) {
    JsonObject app = doc["application"].to<JsonObject>();
    String fileName = getFileName(filePath);
    GetAppName(app, fileName);
    getAppVersion(app, fileName);
    app["author"]   = "Stephen McKeon";
    app["compiled"] = compileDate;
    app["source"]   = filePath;
  }

/*-----  GET THE FILE NAME -----*/

String getFileName(const char* filePath) {
  String path(filePath);
  int pos = path.lastIndexOf('/');
#ifdef _WIN32
  int pos2 = path.lastIndexOf('\\');
  if (pos2 > pos)
    pos = pos2;
#endif
  if (pos >= 0)
    return path.substring(pos + 1);
  return path;
}

/*-----  FILL IN THE APPLICATION NAE -----*/

void GetAppName(JsonObject app, const String& fileName) {
  int pos = fileName.lastIndexOf("_v");
  
  if (pos >= 0)
    app["name"] = fileName.substring(0, pos);
  else
    app["name"] = fileName;
}

/*-----  FILL IN THE APPLICATION VERSION -----*/

void getAppVersion(JsonObject app, const String& fileName) {
  int start = fileName.lastIndexOf("_v");
  
  if (start < 0)
  {
    app["version"] = "";
    return;
  }
  
  int end = fileName.lastIndexOf('.');
  if (end < 0)
    end = fileName.length();
  
  app["version"] = fileName.substring(start + 2, end);
}

  /*-----  PUT THE DOC DATA I TE STANDARD FORMAT AND RETURN THE DATA IN STRING -----*/

String formatAppInfo(const JsonDocument& doc) {
  JsonObjectConst app = doc["application"];
  String result;
  if (!app["name"].isNull() && !app["version"].isNull())
    result += app["name"].as<String>() + " Version: " + app["version"].as<String>();
  if (!app["author"].isNull())
    result += "\nby " + app["author"].as<String>();
  if (!app["compiled"].isNull())
    result += "\nCompiled: " + app["compiled"].as<String>();
  if (!app["source"].isNull())
    result += "\n" + app["source"].as<String>();
  return result;
}

/*-----  GET RUN TIME AND IP ADDRESS INFO -----*/

String addRuntimeInfo(String banner) {
  time_t bootTime = eiTime.getBootTime();
  if (bootTime != 0) {
    unsigned long elapsedMs =
        static_cast<unsigned long>(eiTime.now() - bootTime) * 1000UL;
    banner += "\nUptime: " + eiTime.formatDuration(elapsedMs, DurFormat::VERBOSE);
  }
  else {
    banner += "\nUptime: Unknown";
  }
  banner += "\nIP address: ";
  banner += network.getIPAddress();
  return banner;
}

  /*---------------  PAD A STRING---------------*/

String pad(const String& s, char padCh, int width) {
  String result = s;
  int len = result.length();
  if (len < width) {
    for (int i = 0; i < width - len; i++) {
      result = padCh + result;
    }
  }
  return result;
}

}


