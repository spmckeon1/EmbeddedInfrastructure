
#include <Arduino.h>
#include <ei_types.h>
#include <ei_logging.h>
#include <ei_storage.h>
#include <ei_time.h>
#include <ei_conversion.h>

Logging logging;

/*-----  FORMAT LOG ENTRY FOR THE SERIAL MONITOR  -----*/
   
String Logging::formatSerialLogEntry(const WhoAmI& whoAmI, const String& msg) {
  return eiTime.getLogTimeStamp()+                     //return the log entry
         ", Line#:" + String(whoAmI.line)
         + ", FROM: " + whoAmI.function +
         ": " + msg;
}

/*-----  FORMAT LOG ENTRY AS JSON STRING  -----*/

String Logging::formatJsonStrLogEntry(const WhoAmI& whoAmI, T::Type recordType, L::Level level, ET::Type eventType, const String& message) {
  static uint32_t devSeq = 1;
  JsonDocument doc;

  doc["deviceSequence"] = devSeq++;
  doc["eventTime"]      = eiTime.getLogTimeStamp();
  doc["component"]      = appConsts.appShortName;
  doc["function"]       = whoAmI.function;
  doc["line"]           = whoAmI.line;
  doc["recordType"]     = T::toStr(recordType);
  doc["level"]          = L::toStr(level);
  doc["eventType"]      = ET::toStr(eventType);
  doc["message"]        = message;

  return conv.jsonObjToJsonStr(doc);
}
/*-----  CUSTOMIZED SERIAL.PRINT  -----*/     // Return the canonical timestamp for log records.
                                              // Before time synchronization this is milliseconds since boot.
                                              // After synchronization it becomes the configured date/time format.

void Logging::msg(const WhoAmI& whoAmI, T::Type recordType, L::Level level, ET::Type eventType, const String& message) {
  Serial.print(formatSerialLogEntry(whoAmI, message));
  sendToNodeRedLogging(formatJsonStrLogEntry(whoAmI, recordType, level, eventType, message));
}

/*-----  WRITE TO SYSLOG  -----*/

void Logging::sendToNodeRedLogging(String logEntry) {
  switch (_dest) {
      case LogDestination::RamBuffer:
          storage.writeLog(logEntry);
          break;
      case LogDestination::MqttServer:
//          sendToMqtt(logEntry);
          break;
  }}


/*-----  PUT A DIVIDER IN IN THE LOG  -----*/

// THIS IS INTENDED TO PUT A FORMATED DIVIDER LINE IN A LOG (EVENT) FILE

String Logging::dividerStr(const String& function, int line) {
    return "----- " + function + ":" + String(line) +
           " -------------------------------------";
}


const char* T::toTxt(Type type)  {
    return conv.enumToTxt(
        type,
        typeNames,
        sizeof(typeNames) / sizeof(typeNames[0]));
}

String T::toStr(Type type) {
    return conv.enumToStr(
        type,
        typeNames,
        sizeof(typeNames) / sizeof(typeNames[0]));
}

const char* L::toTxt(Level level) {
    return conv.enumToTxt(
        level,
        levelNames,
        sizeof(levelNames) / sizeof(levelNames[0]));
}

String L::toStr(Level level) {
    return conv.enumToStr(
        level,
        levelNames,
        sizeof(levelNames) / sizeof(levelNames[0]));
}

const char* ET::toTxt(Type type) {
    return conv.enumToTxt(
        type,
        eventTypeNames,
        sizeof(eventTypeNames) / sizeof(eventTypeNames[0]));
}

String ET::toStr(Type type) {
    return conv.enumToStr(
        type,
        eventTypeNames,
        sizeof(eventTypeNames) / sizeof(eventTypeNames[0]));
}

/*-----  PUBLIC INTERFACE TO CHANGE LOGGING DESTINATION  -----*/

void Logging::setDestination(LogDestination destination) {
    _dest = destination;
}
