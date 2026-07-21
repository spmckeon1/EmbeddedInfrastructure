#pragma once

//
//  conversion.h
//  
//
//  Created by Stephen McKeon on 7/19/26.
//

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ei_types.h>

class Conversion {
public:
  static String jsonObjToJsonStr(const JsonDocument& doc);
  static bool jsonStrToJsonObj(const String& json, JsonDocument& doc);
  String fromStr(int from);
  const char* enumToTxt(int value, const char* const* names, int count);
  String enumToStr(int value, const char* const* names, int count);
};
extern Conversion conv;
