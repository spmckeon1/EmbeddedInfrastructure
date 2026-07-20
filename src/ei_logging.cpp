
#include <Arduino.h>
#include <ei_logging.h>
#include <ei_time.h>
#include <ei_conversion.h>

Logging logging;

/*-----  FORMAT LOG ENTRY FOR THE SERIAL MONITOR  -----*/
   
String Logging::doSerialMonLogEntry(String event, String functionName, int lineNo) {
  return eiTime.getLogTimeStamp()+                     //return the log entry
         ", Line#:" + String(lineNo)
         + ", FROM: " + functionName +
         ": " + event;
}

/*-----  FORMAT LOG ENTRY AS JSON STRING  -----*/

String Logging::doJsonStrLogEntry(const WhoAmI& whoAmI, T::Type recordType, L::Level level, ET::Type eventType, const String& message) {
  static uint32_t devSeq = 1;
  JsonDocument doc;

  doc["deviceSequence"] = devSeq++;
  doc["eventTime"]      = getLogTimeStamp();
  doc["component"]      = constData.appShortName;
  doc["function"]       = whoAmI.functionName;
  doc["line"]           = whoAmI.lineNo;
  doc["recordType"]     = T::toString(recordType);
  doc["level"]          = L::toString(level);
  doc["eventType"]      = ET::toString(eventType);
  doc["message"]        = message;

  return conv.jsonObjToJsonStr(doc);
}
/*-----  CUSTOMIZED SERIAL.PRINT  -----*/

void Logging::msg(const WhoAmI& whoAmI, T::Type recordType, L::Level level, ET::Type eventType, const String& message) {
  Serial.print(doSerialMonLogEntry(whoAmI, recordType, level, eventType, message));
  sendToSyslog(doJsonStrLogEntry(whoAmI, recordType, level, eventType, message));
}

/*-----  CUSTOMIZED SERIAL.PRINT  -----*/

void Logging::msg(const String& event, const String& functionName, int lineNo) {
  Serial.print(doSerialMonLogEntry(event, functionName, lineNo));
  sendToSyslog(doJsonStrLogEntry(event, functionName, lineNo));
}

/*-----  WRITE TO SYSLOG  -----*/

void Logging::sendToSyslog(String s) {
  switch (_dest) {
      case LogDestination::RamBuffer:
          sendToRamBuffer(logEntry);
          break;
      case LogDestination::MqttServer:
          sendToMqtt(logEntry);
          break;
  }}


/*-----  PUT A DIVIDER IN IN THE LOG  -----*/

String Logging::dividerStr(const String& function, int line) {
    return "----- " + function + ":" + String(line) +
           " -------------------------------------";
}
