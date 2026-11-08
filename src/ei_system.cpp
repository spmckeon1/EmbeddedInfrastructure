

#include <Arduino.h>

#include <ei_system.h>

#include <ei_appFramework.h>

#ifdef SENSOR_USES_DS18B20
  #include <ei_ds18b20.h>
#elif defined(SENSOR_USES_DHT)
  #include <ei_dht.h>
#endif

#include <ei_events.h>
#include <ei_storage.h>
#include <ei_mqtt.h>
#include <ei_network.h>
#include <ei_time.h>
#include <ei_storage.h>
#include <ei_time.h>
#include <ei_web.h>

EiSystem eiSystem;

/*-----  EISYSTEMS EVENT LOOP  -----*/

void EiSystem::evtLoop() {
// Timers owned by EiSystem
//  static RunTime heapTimer   = {IntervalType::IT_MINUTE, _config.heapMonitorInterval, -1};

// Cooperative scheduler state
//  static constexpr uint8_t NUM_SUBSYSTEMS = 5;
  static uint8_t nextSubsystem = 0;

  // ----- Service EiSystem -----
  if (_state.rebootPending && millis() - _state.rebootRequestedAt >= 5000) {
    performReboot();
  }
  if (_config.heapMonitorEnabled && scheduler.isTimeToRun(_config.heapTimer, FN)) {
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
#ifdef SENSOR_USES_DS18B20
      ds18b20.evtLoop();
#elif defined(SENSOR_USES_DHT)
      dht.evtLoop();
#endif
      break;
    case 5:
      nextSubsystem = 6;
      mqtt.evtLoop();
      break;
    case 6:
    default:
      nextSubsystem = 0;
      web.evtLoop();
      break;
  }
}

bool EiSystem::bootStrap() {
  _rebootReasonFname = appDirs.dataDir + "/rebootReason.json";
  logging.startup();                                            // Logging destinations/config - MUST BE FIRST TO ALLOW LOGGING TO WORK
  if(!storage.startup()) return false;                          // Filesystem available
  
  JsonDocument doc;
  if (storage.exists(_rebootReasonFname)) {
    if (storage.readJsonFile(_rebootReasonFname.c_str(), doc, LN, true)) {
      String reason = doc["reason"].as<String>();
      logInfo(LS, ET::SYSTEM, "Reason for the last reboot was: " + reason);
      storage.deleteFile(_rebootReasonFname.c_str(), LN);
    } else {
      logError(LS, ET::SYSTEM, "Unable to read reboot reason file.");
    }
  } else {
    logInfo(LS, ET::SYSTEM, "No known reason for the previous reboot.");
  }
  
  storage.createDirIfNotExist(appDirs.libCfgDir);
  storage.createDirIfNotExist(appDirs.dataDir);
  storage.createDirIfNotExist(appDirs.logDir);
  storage.createDirIfNotExist(appDirs.appData);
  storage.createDirIfNotExist(appDirs.htmlDir);
  _configFileName = appDirs.libCfgDir + "/ei_systemCfg.json";
  if (!loadConfig()) {
    logError(LS, ET::SYSTEM, "Failed to initialize system configuration.");
    return false;
  }
  mqtt.addToSubCount(1);
  return true;
}

bool EiSystem::setup() {
  if(!logging.setup()) return false;
  // Storage - no setup() needed
  if(!network.setup()) return false;
  if(!eiTime.setup()) return false;
  if(!mqtt.setup()) return false;
  if(!web.setup()) return false;
#ifdef SENSOR_USES_DS18B20
  if(!ds18b20.setup()) return false;
#elif defined(SENSOR_USES_DHT)
  if(!dht.setup()) return false;
#endif
  _config.heapTimer = RunTime{IntervalType::IT_MINUTE, _config.heapMonitorInterval,  -1};
  mqtt.addSubscription("EI Global", EI_MQTT_TOPIC_SUBSCRIPTION, QOS2);      // MUST BE LAST ITEM IN SETUP()
  return true;
}

bool EiSystem::startup() {
  if(!network.startup()) return false;      // Load credentials, initialize WiFi state
  if(!appFramework.startup()) return false;
  if(!mqtt.startup()) return false;
#ifdef SENSOR_USES_DS18B20
    if (!ds18b20.startup()) return false;
#elif defined(SENSOR_USES_DHT)
    if (!dht.startup()) return false;
#endif
  eiEvents.on(EiEvent::TimeActive, processTimeActive);
  return true;
}



/*-----  PROCESS AN INCOMING LIBRARY MSG  -----*/

void EiSystem::processLibraryMsg(const JsonDocument& doc) {
  String route = doc["route"].as<String>();
  int separator = route.indexOf('/');
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
  if (service == "appFramework") {
    appFramework.processMsg(doc);
    return;
  }
  if (service == "system") {
    eiSystem.processMsg(doc);
      return;
  }
  if (service == "web") {
      web.processMsg(doc);
      return;
  }
  logError(LS, ET::SYSTEM, "Received library message for unknown service '" + service + "' from route '" + route + "'.");
}

/*-----  ROUTE AN OUTBOUND LIBRARY MSG  -----*/

void EiSystem::routeOutboundMsg(const JsonDocument& doc) {
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

/*-----  DO GET GLOBAL CONFIGURATION  -----*/

void EiSystem::doGetGlobalCfg() {
  const NetworkConfig& netCfg = network.config();
  const MqttConfig& mqttCfg = mqtt.config();
  const TimeConfig& timeCfg = eiTime.config();
  
  JsonDocument cfgDoc;
  JsonObject cfg = cfgDoc.to<JsonObject>();

  JsonObject networkCfg   = cfg["network"].to<JsonObject>();
  networkCfg["ssid"]      = netCfg.ssid;
  networkCfg["password"]  = netCfg.password;

  JsonObject mqttCfgObj   = cfg["mqtt"].to<JsonObject>();
  mqttCfgObj["host"]      = mqttCfg.host;
  mqttCfgObj["port"]      = mqttCfg.port;
  mqttCfgObj["brokerUser"]  = mqttCfg.brokerUser;
  mqttCfgObj["brokerPwd"]  = mqttCfg.brokerPwd;

  JsonObject timeCfgObj   = cfg["time"].to<JsonObject>();
  timeCfgObj["posixRule"]  = timeCfg.posixRule;
  timeCfgObj["olsonName"] = timeCfg.olsonName;
  
  String payload = appFramework.buildJsonAppMqttMsg("configuration/global/cfg", "PUT", cfg);
  logInfo(LS, ET::SYSTEM, "Global configuration update requested and sent.");
  mqtt.mqttPubMsg(EI_MQTT_OUTBOUND_GLOBAL, QOS2, FORGET, payload, LN);
}

/*-----  HANDLE THE REQUEST FOR A REBOOT  -----*/

void EiSystem::hdlRebootReq(const JsonDocument& doc) {
  String route = doc["route"].as<String>();
  String command = doc["command"].as<String>();
  if (command == "SET") {
    String reason = doc["data"]["reason"].as<String>();
    requestReboot(reason);
    return;
  }
  logError(LS,
    ET::SYSTEM, "Unknown system command '" + command + "' from route '" + route + "'.");
  return;
}

/*-----  HANDLE THE REQUEST FOR A GLOBAL REQUEST  -----*/

void EiSystem::hdlGlobalReq(const JsonDocument& doc) {
  String route = doc["route"].as<String>();
  String command = doc["command"].as<String>();
  if(command == "getConfig") {
    doGetGlobalCfg();
    return;
  }
  logError(LS,
    ET::SYSTEM, "Unknown system command '" + command + "' from route '" + route + "'.");
  return;
}

/*-----  PROCESS AN INCOMING SYSTEM MSG  -----*/

void EiSystem::processMsg(const JsonDocument& doc) {
  String route = doc["route"].as<String>();
  if (route == "system/reboot") {
    hdlRebootReq(doc);
    return;
  }
  if(route == "system/global") {
    hdlGlobalReq(doc);
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
  JsonDocument doc;
  doc["reason"] = reason;
  Storage::WriteResult result = storage.writeJsonFile(_rebootReasonFname.c_str(), doc, LN);
  if (result != Storage::WriteResult::Success) {
    logError(LS, ET::SYSTEM, "Unable to save reboot reason. Reboot request denied.");
    return;
  }
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

void EiSystem::setHeapMonitorInterval(uint16_t minutes) {
  _config.heapMonitorInterval = minutes;
  _config.heapTimer.intvToRun = minutes;
  logInfo(LS, ET::SYSTEM, "Heap monitor interval set to " + String(minutes) + ".");
  saveConfig();
}

/*-----  PUBLIC: GET HEAP CHECK INTERVAL   -----*/

uint16_t EiSystem::getHeapMonitorInterval() const {
    return _config.heapMonitorInterval;
}

/*-----  PUBLIC: GET THE STATE OF HEAP MONITOR ENABLED   -----*/

bool EiSystem::getHeapMonitorEnabled() const {
    return _config.heapMonitorEnabled;
}

/*-----  PUBLIC: SET HEAP MONITORING ENABLED   -----*/

void EiSystem::processExternalMsg(const JsonDocument& doc) {
  String msg;
  serializeJson(doc, msg);

//  DUMP("External message received: " + msg);
//  logInfo(LS, ET::SYSTEM, "External message received: " + msg);
  
  String owner = doc["owner"].as<String>();
  if (owner == "library") {
    processLibraryMsg(doc);
    return;
  }
  if (owner == "application") {
    if (!appHandleMsg(doc)) {
      String msg;
      serializeJson(doc, msg);
      logInfo(LS, ET::WEB, "Received Web message: " + msg);
      logError(LS, ET::MQTT, "Application did not handle external command '" + msg + "'.");
    }
    return;
  }  logError(LS, ET::MQTT, "Received external message with unknown owner '" + owner + "'.");
}

/*-----  GLOBAL: WHEN TIME BECINES ACTIVE   -----*/

void processTimeActive() {
    if (eiTime.isTimeActive()) {
      eiTime.saveBootTime();
    }
}

/*-----  LOAD SYSTEM CONFIGURATION  -----*/

bool EiSystem::loadConfig() {
  JsonDocument doc;
  if (storage.exists(_configFileName)) {
    if (storage.readJsonFile(_configFileName.c_str(), doc, LN, true)) {
      if (doc["heapMonitorInterval"].is<uint16_t>() && doc["heapMonitorInterval"].as<uint16_t>() > 0) {
        _config.heapMonitorInterval = doc["heapMonitorInterval"].as<uint16_t>();
        logInfo(LS, ET::SYSTEM, "System configuration loaded.");
        return true;
      }
      logError(LS, ET::SYSTEM, "System configuration is invalid; restoring current settings.");
    } else {
      logError(LS, ET::SYSTEM, "Unable to read system configuration; restoring current settings.");
    }
  } else {
    logInfo(LS, ET::SYSTEM, "System configuration not found; creating defaults from current settings.");
  }
  if (!saveConfig()) {
    return false;
  }
  return true;
}


/*-----  SAVE SYSTEM CONFIGURATION  -----*/

bool EiSystem::saveConfig() {
  JsonDocument doc;
  doc["heapMonitorInterval"] = _config.heapMonitorInterval;

  Storage::WriteResult result =
      storage.writeJsonFile(_configFileName.c_str(), doc, LN);

  if (result != Storage::WriteResult::Success) {
    logError(LS, ET::SYSTEM, "Unable to save system configuration.");
    return false;
  }

  return true;
}
