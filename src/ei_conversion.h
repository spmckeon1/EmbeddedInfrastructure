#pragma once

//
//  conversion.h
//  
//
//  Created by Stephen McKeon on 7/19/26.
//

#include <Arduino.h>
#include <ArduinoJson.h>

class Conversion {
public:
  static String jsonObjToJsonStr(const JsonDocument& doc);
  static bool jsonStrToJsonObj(const String& json, JsonDocument& doc);
  String fromStr(int from);
};
extern Conversion conv;
