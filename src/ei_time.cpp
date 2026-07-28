
//#include <WiFi.h>
//#include <ezTime.h>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ezTime.h>

#include <ei_scheduler.h>
#include <ei_appPolicy.h>
#include <ei_network.h>
#include <ei_storage.h>
#include <ei_time.h>

EiTime eiTime;

/*-----  TIME EVENT LOOP  -----*/

bool EiTime::evtLoop() {
  static RunTime ntpSetTimer = {IntervalType::IT_SECOND, 5, -1};
  static bool ntpSyncAnnounced = false;

  if (!network.isConnected())
    return false;

  if (timeStatus() != timeSet) {
    if (scheduler.isTimeToRun(ntpSetTimer))
      syncTime();

    ntpSyncAnnounced = false;
    return false;
  }

  if (!ntpSyncAnnounced) {
    logInfo(FN, LN, "NTP synchronization complete.");
    ntpSyncAnnounced = true;
  }

  if (_tz.getPosix() != _config.posixRule) {
    setTimeZone();
    saveBootTime();
    return false;
  }

  if (_pending.pending) {
    // applyPendingConfiguration();
    return false;
  }

  return false;
}


/*-----  CALL FOR NTP SYNC  -----*/

void EiTime::syncTime() {
  logInfo(FN, LN, "Initiating for NTP synchronization...");
   updateNTP();
   return;
}

/*-----  SET THE TIMEZONE VIS POSIX  -----*/

bool EiTime::setTimeZone() {
  if(_tz.setPosix(_config.posixRule)) {
    logInfo(FN, LN, "Applied POSIX timezone: " + _config.posixRule);
    return true;
  }
  logError(FN, LN, "Unable to apply POSIX timezone: " + _config.posixRule);
  return false;
}

/*-----  TAKE CARE OFF ALL THE ACTIONS NECESSAY TO ENSURE IETIME HAS WHAT IT NEEDS TO RUN  -----*/

bool EiTime::setup() {
  JsonDocument doc;
  _configFileName = appDirs.libCfgDir + "/ei_timeCfg.json";
  doc = createConfigJson(_config);

  storage.ensureFileExists(_configFileName, createConfigJson(_config), LN);         // Create the configuration file with defaults if it doesn't exist.
  if (!storage.readJsonFile(_configFileName.c_str(), doc, LN)) {                            // Read the configuration file.
    logError(FN, LN, "Failed to read: " + String(_configFileName));
    return false;
  }
  loadConfigFromJson(doc, _config);                                                 // Populate the working configuration.
  if (!validateConfiguration(_config)) {                                            // If validation fails...
    logInfo(FN, LN, "Invalid time configuration. Restoring defaults.");

    _config = TimeConfig{};                                                         // Restore defaults.

    Storage::WriteResult result = storage.writeJsonFile(_configFileName.c_str(),            // Rewrite configuration file.
                                                        createConfigJson(_config),
                                                        LN);
    if (result != Storage::WriteResult::Success) {
      logError(FN, LN, "Failed to write default configuration.");
      return false;
    }
  }
  logInfo(FN, LN, "EiTime setuo successfully completed.");
  return true;
}

/*---------------    STARTUP THE EZTIME TIME SERVICE  ---------------*/




/*-----  READ THE EITIME CONFIGURATION FILE  -----*/

bool EiTime::readConfigFromDisk() {
    JsonDocument doc;
    if (!storage.readJsonFile(_configFileName.c_str(), doc, LN))
        return false;

    _config.posixRule = doc["posixRule"] | _config.posixRule;

    return true;
}

/*-----  WRITE THE EITIME CONFIGURATION FILE  -----*/

Storage::WriteResult EiTime::writeConfigToDisk() {
    JsonDocument doc;
    doc["posixRule"] = _config.posixRule;
    return storage.writeJsonFile(_configFileName.c_str(), doc, LN);
}

/*-----  CREATE THE CONFIG JS)N OBJECT FROM THE cfg CONTENTS  -----*/

JsonDocument EiTime::createConfigJson(const TimeConfig& cfg) const {
  JsonDocument doc;
  doc["posixRule"] = cfg.posixRule;
  return doc;
}

/*-----  PUT THE JSON OBJECT READ FROM DISK INTO A SUPPLIED TimeConfig STRUCT   -----*/

void EiTime::loadConfigFromJson(const JsonDocument& doc, TimeConfig& cfg) const {
    if (doc["posixRule"].is<String>())
        cfg.posixRule = doc["posixRule"].as<String>();
}

/*-----  VALIDATE AS MUCH AS POSSIBLE THE JSIN OBJECT READ FRM DISK  -----*/

bool EiTime::validateConfiguration(const TimeConfig& cfg) {
  if (cfg.posixRule.isEmpty()) {
    logError(FN, LN, "Failed to validate TimeConfig: "
                    "cfg.posixRule: " + cfg.posixRule
                    );
    return false;
  }
  return true;
}

/*-----  PUT THE RIGHT HEADERS INTO A LOG MSG  -----*/

void EiTime::logInfo(const char* function,
                      int lineNum,
                      const String& msg)
{
    logging.msg(__FILE__,
                function,
                lineNum,
                T::SYSLOG,
                L::INFO,
                ET::TIME,
                msg);
}

/*-----  PUT THE RIGHT HEADERS INTO A LOG MSG  -----*/

void EiTime::logError(const char* function,
                      int lineNum,
                      const String& msg)
{
    logging.msg(__FILE__,
                function,
                lineNum,
                T::SYSLOG,
                L::ERROR,
                ET::TIME,
                msg);
}


time_t EiTime::now()
{
    return _tz.now();
}

uint8_t EiTime::second()
{
    return _tz.second();
}

uint8_t EiTime::minute()
{
    return _tz.minute();
}

uint8_t EiTime::hour()
{
    return _tz.hour();
}

uint8_t EiTime::hour12()
{
    return _tz.hourFormat12();
}

uint8_t EiTime::day()
{
    return _tz.day();
}

uint8_t EiTime::weekday()
{
    return _tz.weekday();
}

uint8_t EiTime::month()
{
    return _tz.month();
}

uint16_t EiTime::year()
{
    return _tz.year();
}

uint16_t EiTime::millisecond()
{
    return millis() % 1000;
}


/*-----  COMPUTE THE CORRECT UNIX BOOT TIME AND GET IT WRITTEN TO DISK  -----*/

void EiTime::saveBootTime() {
  JsonDocument doc;
  time_t bootTime = _tz.now() - (millis() / 1000);                                       // get the actual boot time
  doc["bootTime"] = bootTime;                                                             // put the n=boot time inti a json object
  storage.writeJsonFile(appFnames.bootTime.c_str(), doc, LN);                             // write it to disk
  logInfo(FN, LN, "Saved boot time: " + String(bootTime) + " to " + appFnames.bootTime);  // log the actvity
}

/*-----    GET LOG TIME STAMP  -----*/

String EiTime::getLogTimeStamp() {
  if(_tz.getTimezoneName() == "" || _tz.getTimezoneName() == "UTC") {
//  if(timeStatus() != timeSet) {
    return String(millis()) + "ms";               // put the milliseconds into 't'
  } else {
    return formatLogTime();
  }
}


/*-----    GET THE STANDARD TIME STRING FOR LOGS  -----*/

String EiTime::formatLogTime() {
    return _tz.dateTime(LOG_TIME_FORMAT);
}

/*-----    ALLOW A PUBLIC AGENT TO CHANGE SEND A POSIX STRING TO IETIME  -----*/

bool EiTime::setPosixRule(const String& rule) {
    if (!_tz.setPosix(rule))
        return false;
    _config.posixRule = rule;
    _config.dirty = true;
    writeConfigToDisk();
    return true;
}
