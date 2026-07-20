//
//  conversion.c
//  
//
//  Created by Stephen McKeon on 7/19/26.
//

#include <Arduino.h>
#include <ei_conversion.h>

Conversion conv;

/*---------------  SERIALIZE A JSON DOC, AND RETURN IT AS A STRING  ---------------*/

String Conversion::jsonObjToJsonStr(const JsonDocument& doc) {
  String json;
  serializeJson(doc, json);
  return json;
}

bool Conversion::jsonStrToJsonObj(const String& json, JsonDocument& doc) {
  return deserializeJson(doc, json) == DeserializationError::Ok;
}

/*---------------  TAKE CARE OF LOGS WITH FROM IN THEM  ---------------*/

String Conversion::fromStr(int from) {
    return " [FROM: " + String(from) + "]";
}
