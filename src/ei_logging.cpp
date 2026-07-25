
#include <Arduino.h>
#include <ei_types.h>
#include <ei_logging.h>
#include <ei_storage.h>
#include <ei_time.h>
#include <ei_conversion.h>

Logging logging;

/*-----  STARTUP THE LOGGING SERVICE  -----*/

void Logging::startup() {
  Serial.begin(115200);                                                         // set the serial port speed
  dividerStr(FN, LN);
  dividerStr(FN, LN);
  logInfo(FN, LN, "Starting boot process." );
  logInfo(FN, LN, "Serial started at 115200");
  logInfo(FN, LN, "Starting the Logging service");
}

/*-----  FORMAT LOG ENTRY FOR THE SERIAL MONITOR  -----*/
   
String Logging::formatSerialLogEntry(const char* file,
                                     const char* function,
                                     int lineNum,
                                     const String& msg)
{
  return eiTime.getLogTimeStamp()
      + ", "
      + file
      + ":"
      + String(lineNum)
      + ", "
      + function
      + "(): "
      + msg
   ;
}

/*-----  FORMAT LOG ENTRY AS JSON STRING  -----*/

String Logging::formatJsonStrLogEntry(const char* file,
                                      const char* function,
                                      int lineNum,
                                      T::Type recordType,
                                      L::Level level,
                                      ET::Type eventType,
                                      const String& message)
{
  static uint32_t devSeq = 1;
  JsonDocument doc;

  doc["deviceSequence"] = devSeq++;
  doc["eventTime"]      = eiTime.getLogTimeStamp();
  doc["component"]      = appConsts.appShortName;
  doc["file"]           = file;
  doc["function"]       = function;
  doc["line"]           = lineNum;
  doc["recordType"]     = T::toStr(recordType);
  doc["level"]          = L::toStr(level);
  doc["eventType"]      = ET::toStr(eventType);
  doc["message"]        = message;

  return conv.jsonObjToJsonStr(doc);
}
/*-----  PRINT THE LOG MSG TO SERIAL AND SEND IT TO sendToNodeRedLogging()  -----*/

void Logging::msg(
    const char* file,
    const char* function,
    int lineNum,
    T::Type recordType,
    L::Level level,
    ET::Type eventType,
    const String& message)
{
  Serial.println(formatSerialLogEntry(file,
                                    function,
                                    lineNum,
                                    message));
  sendToNodeRedLogging(formatJsonStrLogEntry(file,
                                             function,
                                             lineNum,
                                             recordType,
                                             level,
                                             eventType,
                                             message));
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

/*---------------  PUT THE RIGHT HEADERS INTO A STORAGE INFO LOG  ---------------*/

void Logging::logInfo(const char* function,
                      int lineNum,
                      const String& msg)
{
    logging.msg(__FILE__,
                function,
                lineNum,
                T::SYSLOG,
                L::INFO,
                ET::LOGGING,
                msg);
}

/*---------------  PUT THE RIGHT HEADERS INTO A STORAGE ERROR LOG  ---------------*/

void Logging::logError(const char* function,
                      int lineNum,
                      const String& msg)
{
    logging.msg(__FILE__,
                function,
                lineNum,
                T::SYSLOG,
                L::ERROR,
                ET::LOGGING,
                msg);
}
