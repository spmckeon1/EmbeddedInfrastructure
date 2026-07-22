
//#include <WiFi.h>
//#include <ezTime.h>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ei_storage.h>
#include <ei_time.h>

EiTime eiTime;

/*-----  TIME EVENT LOOP  -----*/

bool EiTime::eventLoop() {
    return false;
}

/*-----  TAKE CARE OFF ALL THE ACTIONS NECESSAY TO ENSURE IETIME HAS WHAT IT NEEDS TO RUN  -----*/

bool EiTime::setup() {
//  TimeConfig config;
  JsonDocument doc;
  storage.ensureFileExists(_configFileName, createConfigJson(_config), LN);         // Create the configuration file with defaults if it doesn't exist.
  if (!storage.readJsonFile(_configFileName, doc, LN)) {                            // Read the configuration file.
    eiTime.logError("Failed to read: " + String(_configFileName));
    return false;
  }
  loadConfigFromJson(doc, _config);                                                 // Populate the working configuration.
  if (!validateConfiguration(_config)) {                                            // if validate fails then
    _config = TimeConfig{};                                                         // set it to the defauts
    storage.writeJsonFile(_configFileName, createConfigJson(_config), LN);          // and write it to disk
  }
  return true;
}

/*---------------    STARTUP THE EZTIME TIME SERVICE  ---------------*/

bool EiTime::begin() {
  static uint32_t lastAttempt = 0;
  if (millis() - lastAttempt < 5000)                          // Only retry every 5 seconds.
    return false;

  lastAttempt = millis();
 if (timeStatus() != timeSet) {                               // STEP 1 - Wait for NTP synchronization.
   eiTime.logInfo("Waiting for NTP synchronization...");
    updateNTP();
    return false;
  }
  _state.timeValid = true;
  if (_config.posixRule.length()) {                           // STEP 2 - Fast path: use cached POSIX rule.
    if (_tz.setPosix(_config.posixRule)) {
      _state.ready = true;
      _state.abbreviation = _tz.dateTime("T");
      return true;
    }
  }
  if (_config.olsonName.length()) {                           // STEP 3 - Resolve from Olson database.
    if (_tz.setLocation(_config.olsonName)) {
        String newRule = _tz.getPosix();
        if (newRule != _config.posixRule) {
          _config.posixRule = newRule;
          _state.posixChanged = true;
        }
        _state.abbreviation = _tz.dateTime("T");
        _state.ready = true;
        return true;
    }
    eiTime.logError("Unable to resolve Olson timezone.");
    return false;
  }
  eiTime.logError("No timezone configured. Using default.");   // STEP 4 - Final fallback.
  _tz.setPosix(_config.posixRule);
  _config.posixRule = _config.posixRule;
  _state.posixChanged = true;
  _state.abbreviation = _tz.dateTime("T");
  _state.ready = true;
  return true;
}

/*---------------    GET LOG TIME STAMP  ---------------*/

String EiTime::getLogTimeStamp() {
  return String(millis()) + "ms";               // put the milliseconds into 't'
}


/*-----  READ THE EITIME CONFIGURATION FILE  -----*/

bool EiTime::readConfigFromDisk() {
    JsonDocument doc;
    if (!storage.readJsonFile(_configFileName, doc, LN))
        return false;

    _config.olsonName = doc["olsonName"] | _config.olsonName;
    _config.posixRule = doc["posixRule"] | _config.posixRule;

    return true;
}

/*-----  WRITE THE EITIME CONFIGURATION FILE  -----*/

bool EiTime::writeConfigToDisk() {
    JsonDocument doc;

    doc["olsonName"] = _config.olsonName;
    doc["posixRule"] = _config.posixRule;

    return storage.writeJsonFile(_configFileName, doc, LN);
}

/*-----  CREATE THE CONFIG JS)N OBJECT FROM THE cfg CONTENTS  -----*/

JsonDocument EiTime::createConfigJson(const TimeConfig& cfg) const {
    JsonDocument doc;
    doc["olsonName"] = cfg.olsonName;
    doc["posixRule"] = cfg.posixRule;
    return doc;
}

/*-----  PUT THE JSON OBJECT READ FROM DISK INTO A SUPPLIED TimeConfig STRUCT   -----*/

void EiTime::loadConfigFromJson(const JsonDocument& doc, TimeConfig& cfg) const {
    if (doc["olsonName"].is<String>())
        cfg.olsonName = doc["olsonName"].as<String>();
    if (doc["posixRule"].is<String>())
        cfg.posixRule = doc["posixRule"].as<String>();
}

/*-----  VALIDATE AS MUCH AS POSSIBLE THE JSIN OBJECT READ FRM DISK  -----*/

bool EiTime::validateConfiguration(const TimeConfig& cfg) {
  if (cfg.olsonName.isEmpty() || cfg.posixRule.isEmpty()) {
    eiTime.logError("Failed to validate TimeConfig: "
                    "cfg.posixRule: " + cfg.posixRule +
                    "cfg.olsonName: " + cfg.olsonName
                    );
    return false;
  }
  return true;
}

/*-----  PUT THE RIGHT HEADERS INTO A LOG MSG  -----*/

void EiTime::logInfo(const String& msg) {
    logging.msg(WHOAMI, T::SYSLOG, L::INFO, ET::TIME, msg);
}

/*-----  PUT THE RIGHT HEADERS INTO A LOG MSG  -----*/

void EiTime::logError(const String& msg) {
    logging.msg(WHOAMI, T::SYSLOG, L::ERROR, ET::TIME, msg);
}


