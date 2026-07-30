
#include <Arduino.h>
#include <ei_types.h>
#include <ei_appPolicy.h>
#include <ei_logging.h>
#include <ei_mqtt.h>
#include <ei_storage.h>
#include <ei_time.h>
#include <ei_conversion.h>

Logging logging;

/*-----  LOGGING EVENT LOOP  -----*/

bool Logging::evtLoop() {
  flushOnePendingLog();

  return flushOnePendingLog();
}

/*-----  STARTUP THE LOGGING SERVICE  -----*/

void Logging::startup() {
  Serial.begin(115200);                                                         // set the serial port speed
  logInfo(FN, LN, dividerStr(FN, LN));
  logInfo(FN, LN, dividerStr(FN, LN));
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
                                      const char* eventType,
                                      const String& message)
{
  static uint32_t devSeq = 1;
  JsonDocument doc;

  doc["deviceSequence"] = devSeq++;
  doc["eventTime"]      = eiTime.getLogTimeStamp();
  doc["component"]      = appIDs.shortName;
  doc["file"]           = file;
  doc["function"]       = function;
  doc["line"]           = lineNum;
  doc["recordType"]     = T::toStr(recordType);
  doc["level"]          = L::toStr(level);
  doc["eventType"]      = eventType;
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
    const char* eventType,
    const String& message)
{
  file = baseFileName(file);
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

bool Logging::sendToNodeRedLogging(const String& logEntry)
{
    switch (_dest)
    {
        case LogDestination::RamBuffer:
          return enqueuePendingLog(logEntry);
        case LogDestination::MqttServer:
          return mqtt.mqttPubMsg(_config.topic,
                                   _config.qos,
                                   _config.retain,
                                   logEntry,
                                   LN);
    }

    return false;
}

/*-----  PUT A DIVIDER IN IN THE LOG  -----*/

String Logging::dividerStr(const String& function, int line) {
    return "----- " + function + ":" + String(line) + " -------------------------------------";
}


/*-----  CONCERT THE T ENUM TO CHAR  -----*/

const char* T::toTxt(Type type)  {
    return conv.enumToTxt(
        type,
        typeNames,
        sizeof(typeNames) / sizeof(typeNames[0]));
}

/*-----  CONCERT THE T ENUM TO STRING  -----*/

String T::toStr(Type type) {
    return conv.enumToStr(
        type,
        typeNames,
        sizeof(typeNames) / sizeof(typeNames[0]));
}

/*-----  CONCERT THE L ENUM TO CHAR  -----*/

const char* L::toTxt(Level level) {
    return conv.enumToTxt(
        level,
        levelNames,
        sizeof(levelNames) / sizeof(levelNames[0]));
}

/*-----  CONCERT THE L ENUM TO STRING  -----*/

String L::toStr(Level level) {
    return conv.enumToStr(
        level,
        levelNames,
        sizeof(levelNames) / sizeof(levelNames[0]));
}

/*-----  PUBLIC INTERFACE TO CHANGE LOGGING DESTINATION  -----*/

void Logging::setDestination(LogDestination destination) {
  _dest = destination;
  logInfo(FN, LN, "Logging destination has been chaged to: " + String(destinationToString(_dest)));
}

/*-----  GET THE PRINTABLE VALUE OF _dest  -----*/

const char* Logging::destinationToString(LogDestination dest) const
{
    switch (dest)
    {
        case LogDestination::RamBuffer: return "RamBuffer";
        case LogDestination::MqttServer: return "MqttServer";
        default:                        return "Unknown";
    }
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

/*-----  PUT THE RIGHT HEADERS INTO A STORAGE ERROR LOG  -----*/

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

/*-----  SHORTEN THE FQN TO THE DESIRED COMPONENT(S)  -----*/

const char* Logging::baseFileName(const char* file) {
    const char* p = strrchr(file, '/');
    if (p)
        return p + 1;
    p = strrchr(file, '\\');   // Windows support
    if (p)
        return p + 1;
    return file;
}

//  RAM LOG PROCESSES

/*-----  ARE THE PENDING LOGS EMPTY  -----*/

bool Logging::pendingLogsEmpty() const {
    return _queueCount == 0;
}

/*-----  ARE THE PENDING LOGS FULL  -----*/

bool Logging::pendingLogsFull() const {
    return _queueCount >= MAX_PENDING_LOGS;
}

/*-----  ADD THE NEW LOG AT THE TAIL OF THE QUEUE  -----*/

bool Logging::enqueuePendingLog(const String& jsonLog) {
  if (pendingLogsFull())
    return false;
  _pendingLogQueue[_queueTail] = jsonLog;
  _queueTail = (_queueTail + 1) % MAX_PENDING_LOGS;
  _queueCount++;
  return true;
}

/*-----  REMOVE LOGS FROM THE HEAD TO THE TAIL  -----*/

bool Logging::dequeuePendingLog(String& jsonLog) {
  if (pendingLogsEmpty())
    return false;
  jsonLog = _pendingLogQueue[_queueHead];             // Transfer ownership of the oldest pending log
  _pendingLogQueue[_queueHead].clear();               // Release the queue's copy
  _queueHead = (_queueHead + 1) % MAX_PENDING_LOGS;   // Advance to the next entry
  _queueCount--;                                      // One less entry remains
  return true;
}

/*-----  SEND THE QUEUED LOGS TO NODE-RED 1 AT A TIME  -----*

bool Logging::flushPendingLogs() {
  String jsonLog;
  while (dequeuePendingLog(jsonLog)) {
    if (!sendToNodeRedLogging(jsonLog)) {
      enqueuePendingLog(jsonLog);// Connection dropped again. Put this log back and stop trying.
      return false;
    }
  }
  return true;
}

 /*-----  SEND THE 1 QUEUED LOG TO sendToNodeRedLogging()  -----*/

 bool Logging::flushOnePendingLog() {
  if (pendingLogsEmpty())
    return false;
  if (!mqtt.connected())
    return false;
  String jsonLog;
  if (!dequeuePendingLog(jsonLog))
    return false;
  if (!sendToNodeRedLogging(jsonLog)) {
    enqueuePendingLog(jsonLog);
    return false;
  }
  return true;
}
