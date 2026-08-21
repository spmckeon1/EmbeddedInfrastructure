
#include <Arduino.h>
#include <ei_appPolicy.h>
#include <ei_ds18b20.h>
#include <ei_storage.h>
#include <ei_mqtt.h>
#include <ei_network.h>
#include <ei_time.h>
#include <ei_scheduler.h>
#include <ei_system.h>
#include <ei_web.h>

EiSystem eiSystem;

/*-----  EISYSTEMS EVENT LOOP  -----*/

void EiSystem::evtLoop() {
  // Timers owned by EiSystem
  static RunTime rebootTimer = {IntervalType::IT_SECOND, 5, -1};
  static RunTime heapTimer   = {IntervalType::IT_MINUTE, _config.heapMonitorInterval, -1};

  // Cooperative scheduler state
  static constexpr uint8_t NUM_SUBSYSTEMS = 5;
  static uint8_t nextSubsystem = 0;

  // ----- Service EiSystem -----
  if (_state.rebootPending && scheduler.isTimeToRun(rebootTimer)) {
    performReboot();
  }
  if (_config.heapMonitorEnabled &&
      scheduler.isTimeToRun(heapTimer)) {
    checkHeap();
  }

  // ----- Service one EmbeddedInfrastructure subsystem -----
  switch (nextSubsystem) {
    case 0:
      nextSubsystem = 1;
      logging.evtLoop();
      break;
    case 1:
      nextSubsystem = 2;
      storage.evtLoop();
      break;
    case 2:
      nextSubsystem = 3;
      network.evtLoop();
      break;
    case 3:
      nextSubsystem = 4;
      eiTime.evtLoop();
      break;
    case 4:
      nextSubsystem = 5;
      ds18b20.evtLoop();
      break;
    case 5:
    default:
      nextSubsystem = 0;
      mqtt.evtLoop();
      break;
  }
}

bool EiSystem::bootStrap() {
    logging.startup();                              // Logging destinations/config - MUST BE FIRST TO ALLOW LOGGING TO WORK
    if(!storage.startup()) return false;            // Filesystem available
    storage.createDirIfNotExist(appDirs.libCfgDir);
    storage.createDirIfNotExist(appDirs.dataDir);
    storage.createDirIfNotExist(appDirs.logDir);
    storage.createDirIfNotExist(appDirs.appData);
    storage.createDirIfNotExist(appDirs.htmlDir);
    return true;
}

bool EiSystem::setup() {
  // Logging - no setup() needed
  // Storage - no setup() needed
  if(!network.setup()) return false;
  if(!eiTime.setup()) return false;
  if(!mqtt.setup()) return false;
  if(!web.setup()) return false;
  if(!ds18b20.setup()) return false;

  return true;
}

bool EiSystem::startup() {
  if(!network.startup()) return false;      // Load credentials, initialize WiFi state
  if(!mqtt.startup()) return false;
//  if(!web.startup()) return false;
  if(!ds18b20.startup()) return false;
  return true;
}



/*-----  PROCESS AN INCOMING LIBRARY MSG  -----*/

void EiSystem::processLibraryMsg(const JsonDocument& doc) {
  TRACE();
  String route = doc["route"].as<String>();
  int separator = route.indexOf('/');
  if (separator <= 0) {
    logError(LS, ET::SYSTEM, "Library message has invalid route '" + route + "'.");
    return;
  }
  String service = route.substring(0, separator);
  if (service == "network") {
    network.processMsg(doc);
    return;
  }
  if (service == "mqtt") {
    mqtt.processMsg(doc);
    return;
  }
  if (service == "time") {
    eiTime.processMsg(doc);
    return;
  }
  if (service == "storage") {
    storage.processMsg(doc);
    return;
  }
  if (service == "web") {
    web.processMsg(doc);
    return;
  }
  if (service == "system") {
    eiSystem.processMsg(doc);
      return;
  }
  logError(
    LS,
    ET::SYSTEM,
    "Received library message for unknown service '" +
    service + "' from route '" + route + "'."
  );
}

/*-----  ROUTE AN OUTBOUND LIBRARY MSG  -----*/

void EiSystem::routeOutboundMsg(const JsonDocument& doc) {
    TRACE();

    const char* receiver = doc["receiver"] | "";

    if (strcmp(receiver, "web") == 0) {
        web.webPubMsg(doc);
        return;
    }

//    if (strcmp(receiver, "mqtt") == 0) {
//        mqtt.processMsg(doc);
//        return;
//    }

    logError(
        LS,
        ET::SYSTEM,
        "Outbound message has unknown receiver '" +
        String(receiver) +
        "'."
    );
}

/*-----  PROCESS AN INCOMING SYSTEM MSG  -----*/

void EiSystem::processMsg(const JsonDocument& doc) {
  TRACE();
  String route = doc["route"].as<String>();
  String command = doc["command"].as<String>();
  if (route == "system/reboot") {
    if (command == "SET") {
        requestReboot("Web Setup requested reboot.");
        return;
    }
    logError(
        LS,
        ET::SYSTEM,
        "Unknown system command '" + command +
        "' from route '" + route + "'."
    );
    return;
  }
  logError(LS, ET::SYSTEM, "Unknown system route '" + route + "'.");
}

/*-----  TRANSLATE FROM TEXT TO ENUM  -----*/

EiSystem::Service EiSystem::serviceFromString(const char* s) {
  if (strcmp(s, "network") == 0)
    return Service::Network;
  if (strcmp(s, "mqtt") == 0)
    return Service::Mqtt;
  if (strcmp(s, "time") == 0)
    return Service::Time;
  if (strcmp(s, "storage") == 0)
    return Service::Storage;
  if (strcmp(s, "web") == 0)
    return Service::Web;
  if (strcmp(s, "logging") == 0)
    return Service::Logging;
  return Service::Unknown;
}


/*-----  TRANSLATE FROM ENUM TO TEXT   -----*/

const char* EiSystem::serviceToString(Service service) {
  switch (service) {
    case Service::Network:
      return "network";
    case Service::Mqtt:
      return "mqtt";
    case Service::Time:
      return "time";
    case Service::Storage:
      return "storage";
    case Service::Web:
      return "web";
    case Service::Logging:
      return "logging";
    default:
      return "unknown";
  }
}

/*-----  CALL FOR A REBOOT   -----*/

void EiSystem::requestReboot(const String& reason, bool immediate) {
  if (reason.isEmpty()) {
    logError(LS, ET::SYSTEM, "A reboot request without a reason is not allowed.");
    return;
  }
  logInfo(LS, ET::SYSTEM, "Reboot requested: " + reason);
  _state.rebootReason = reason;
  if (immediate) {
    performReboot();
    return;
  }
  _state.rebootPending = true;
  _state.rebootRequestedAt = millis();
}

/*-----  REBOOT THE ESP   -----*/

void EiSystem::performReboot() {
  logInfo(LS, ET::SYSTEM, "Rebooting.");
  // Notify subsystems.
  // Flush logs.
  // Disconnect MQTT.
  // Close files.
  Serial.flush();
  ESP.restart();
}

/*-----  CHECK THE HEAP   -----*/

void EiSystem::checkHeap() {
  _state.lastFreeHeap = _state.freeHeap;
  _state.freeHeap  = ESP.getFreeHeap();
  logInfo(LS, ET::SYSTEM, "Free heap: " + String(_state.freeHeap) + " bytes (previous: " + String(_state.lastFreeHeap) + ").");
}

/*-----  PUBLIC: GET THE HEAP   -----*/

void EiSystem::getFreeHeap() {
  checkHeap();
}

/*-----  PUBLIC: SET HEAP MONITORING ENABLED   -----*/

void EiSystem::enableHeapMonitor(bool enabled) {
    _config.heapMonitorEnabled = enabled;
}

/*-----  PUBLIC: SET HEAP CHECK INTERVAL   -----*/

void EiSystem::setHeapMonitorInterval(uint16_t minutes)
{
    _config.heapMonitorInterval = minutes;
}

/*-----  PUBLIC: SET HEAP MONITORING ENABLED   -----*/

void EiSystem::processExternalMsg(const JsonDocument& doc, Source source) {
  TRACE();
  String msg;
  serializeJson(doc, msg);

  logInfo(
      LS,
      ET::SYSTEM,
      "External message received: " + msg
  );

  
  
  String owner = doc["owner"].as<String>();
  if (owner == "library") {
    processLibraryMsg(doc);
    return;
  }
  if (owner == "application") {
    if (!appHandleMsg(doc, source)) {
      String msg;
      serializeJson(doc, msg);
      logInfo(LS, ET::WEB, "Received Web message: " + msg);
      logError(LS, ET::MQTT, "Application did not handle external command '" + msg + "'.");
    }
    return;
  }  logError(LS, ET::MQTT, "Received external message with unknown owner '" + owner + "'.");
}
