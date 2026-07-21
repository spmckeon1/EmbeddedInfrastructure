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

/*---------------  CONVERT A ENUM TO IT'S TEXTVALUE STORED IN A CONSTEXPR ARRAY  ---------------*/

const char* Conversion::enumToTxt(int value, const char* const* names, int count) {
    if (value < 0 || value >= count)
        return "UNKNOWN";
    return names[value];
}

/*---------------  CONVERT A ENUM STRING  ---------------*/

String Conversion::enumToStr(int value, const char* const* names, int count) {
  return String(enumToTxt(value, names, count));
}
