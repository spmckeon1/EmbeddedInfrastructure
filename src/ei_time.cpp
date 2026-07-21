
//#include <WiFi.h>
//#include <ezTime.h>

#include <Arduino.h>
#include <ei_time.h>

EiTime eiTime;

/*-----  TIME EVENT LOOP  -----*/

bool EiTime::eventLoop() {
    return false;
}

/*---------------    STARTUP THE EZTIME TIME SERVICE  ---------------*/

bool EiTime::begin() {
  static uint32_t lastAttempt = 0;
  if (millis() - lastAttempt < 5000)                          // Only retry every 5 seconds.
    return false;

  lastAttempt = millis();
 if (timeStatus() != timeSet) {                               // STEP 1 - Wait for NTP synchronization.
    logging.info("Waiting for NTP synchronization...");
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
    logging.warn("Unable to resolve Olson timezone.");
    return false;
  }
  logging.warn("No timezone configured. Using default.");   // STEP 4 - Final fallback.
  _tz.setPosix(DEFAULT_POSIX_RULE);
  _config.posixRule = DEFAULT_POSIX_RULE;
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

    if (!myStorage.readJsonFile(_configFileName, doc))
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

    return myStorage.writeJsonFile(_configFileName, doc);
}

/*-----  CREATE THE CONFIG JSN OBJECT FROM MEMORY CONTENTS  -----*/

JsonDocument EiTime::createEiTimeCfgJson() {
    JsonDocument doc;
    doc["olsonName"] = _config.olsonName;
    doc["posixRule"] = _config.posixRule;
    return doc;
}
