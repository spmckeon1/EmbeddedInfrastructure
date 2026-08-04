
//#include <WiFi.h>
//#include <ezTime.h>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ezTime.h>

#include <ei_events.h>
#include <ei_scheduler.h>
#include <ei_appPolicy.h>
#include <ei_network.h>
#include <ei_storage.h>
#include <ei_utilities.h>
#include <ei_time.h>

EiTime eiTime;

/*-----  TIME EVENT LOOP  -----*/

bool EiTime::evtLoop() {
  static RunTime ntpSetTimer = {IntervalType::IT_SECOND, 2, -1};
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
    logInfo(LS, ET::TIME, "NTP synchronization complete.");
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
  logInfo(LS, ET::TIME, "Initiating for NTP synchronization...");
   updateNTP();
   return;
}

/*-----  SET THE TIMEZONE VIS POSIX  -----*/

bool EiTime::setTimeZone() {
  if(_tz.setPosix(_config.posixRule)) {
    logInfo(LS, ET::TIME, "Applied POSIX timezone: " + _config.posixRule);
    return true;
  }
  logError(LS, ET::TIME, "Unable to apply POSIX timezone: " + _config.posixRule);
  return false;
}

/*-----  TAKE CARE OFF ALL THE ACTIONS NECESSAY TO ENSURE IETIME HAS WHAT IT NEEDS TO RUN  -----*/

bool EiTime::setup() {
  TRACE();
  JsonDocument doc;
  _configFileName = appDirs.libCfgDir + "/ei_timeCfg.json";
  doc = createConfigJson(_config);
  storage.ensureFileExists(_configFileName, createConfigJson(_config), LN);         // Create the configuration file with defaults if it doesn't exist.
  if (!storage.readJsonFile(_configFileName.c_str(), doc, LN)) {                            // Read the configuration file.
    logError(LS, ET::TIME, "Failed to read: " + String(_configFileName));
    return false;
  }
  loadConfigFromJson(doc, _config);                                                 // Populate the working configuration.
  if (!validateConfiguration(_config)) {                                            // If validation fails...
    logInfo(LS, ET::TIME, "Invalid time configuration. Restoring defaults.");
    _config = TimeConfig{};                                                         // Restore defaults.
    Storage::WriteResult result = storage.writeJsonFile(_configFileName.c_str(),            // Rewrite configuration file.
                                                        createConfigJson(_config),
                                                        LN);
    if (result != Storage::WriteResult::Success) {
      logError(LS, ET::TIME, "Failed to write default configuration.");
      return false;
    }
  }
  logInfo(LS, ET::TIME, "EiTime setuo successfully completed.");
  return true;
}

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

/*-----  CREATE THE CONFIG JSON OBJECT FROM THE CFG CONTENTS  -----*/

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
    logError(LS, ET::TIME, "Failed to validate TimeConfig: "
                    "cfg.posixRule: " + cfg.posixRule
                    );
    return false;
  }
  return true;
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
  logInfo(LS, ET::TIME, "Saved boot time: " + String(bootTime) + " to " + appFnames.bootTime);  // log the actvity
  logInfo(LS, ET::TIME, "The boot process is now complete and took " + String(millis()) + "ms");
  logInfo(LS, ET::TIME, logging.dividerStr(FN, LN));
  logInfo(LS, ET::TIME, logging.dividerStr(FN, LN));
  eiEvents.notify(EiEvent::SystemReady); 
}

/*-----  READ THE BOOT TIME FILE AND RETURN IT  -----*/

time_t EiTime::getBootTime() const {
  JsonDocument doc;
  if (!storage.readJsonFile(appFnames.bootTime.c_str(), doc, __LINE__)) {
    logError(LS, ET::TIME, "Unable to read boot time.");
    return 0;
  }
  return static_cast<time_t>(doc["bootTime"] | 0);
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

/*-----  GET A DRRATION IN HUMAN READABLE FORMAT -----*/

String EiTime::formatDuration(uint32_t ms, DurFormat format) {
  const uint32_t days    = ms / 86400000UL;
  const uint32_t hours   = (ms / 3600000UL) % 24;
  const uint32_t minutes = (ms / 60000UL) % 60;
  const uint32_t seconds = (ms / 1000UL) % 60;

  switch (format)
  {
    case DurFormat::COMPACT:
      return  String(days) + "-" +
              Text::pad(String(hours),   '0', 2) + ":" +
              Text::pad(String(minutes), '0', 2) + ":" +
              Text::pad(String(seconds), '0', 2);

    case DurFormat::PRETTY:
      return  String(days) + " days, " +
              Text::pad(String(hours),   '0', 2) + ":" +
              Text::pad(String(minutes), '0', 2) + ":" +
              Text::pad(String(seconds), '0', 2);

    case DurFormat::VERBOSE:
      return  String(days)    + " Days " +
              String(hours)   + " Hours " +
              String(minutes) + " Minutes " +
              String(seconds) + " Seconds";
  }

  return "";
}

/*-----  PROCESS AN INCOMING MSG  -----*/

void EiTime::processMsg(const JsonDocument& doc) {
  
}
