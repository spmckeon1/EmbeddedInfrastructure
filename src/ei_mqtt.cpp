//
//  ei_mqtt.c
//  
//
//  Created by Stephen McKeon on 7/26/26.
//
#include <ArduinoTrace.h>
#include <ei_appPolicy.h>
#include <ei_logging.h>
#include <ei_network.h>
#include <ei_storage.h>
#include <ei_scheduler.h>
#include <ei_mqtt.h>

EiMqtt mqtt;

/*-----  MQTT EVENT LOOP  -----*/

bool EiMqtt::evtLoop() {
  RunTime loopTimer = {IntervalType::IT_SECOND, 5, -1};
  if(!_state.connected && scheduler.isTimeToRun(loopTimer)) {
    connect();
  }
  sendHeartbeat();
  // add event capture here
  return false;
}

/*-----  START THE MQTT SYSTEM  -----*/

bool EiMqtt::startup() {
  if(network.isConnected() && !_state.connected) {
    connect();
  }
  return true;
}

/*-----  MAKE SURE THE MQTT LIBRARY IS READY TO GO  -----*/

bool EiMqtt::setup() {
  JsonDocument doc;
  configToJson(doc);
  _configFileName = appDirs.libCfgDir + "/ei_mqttCfg.json";
  logInfo(FN, LN, "Starting MQTT subsystem...");
  Storage::EnsureFileResult result =                                        // Ensure the configuration file exists.
            storage.ensureFileExists(_configFileName.c_str(), doc, LN);
  switch (result) {
    case Storage::EnsureFileResult::Created:
      logInfo(FN, LN, "Created default MQTT configuration file.");
      break;
    case Storage::EnsureFileResult::AlreadyExists:
      break;                                                                  // Nothing to do
    case Storage::EnsureFileResult::Error:
      logError(FN, LN, "Unable to ensure MQTT configuration file exists.");
      return false;
  }
  if(!readCfgFromDisk()) {
    logError(FN, LN, "Unable to read MQTT configuration.");
    return false;
  }
  applyConfiguration();
  
  _client.onConnect([this](bool sessionPresent) { onMqttConnect(sessionPresent);});   // Register MQTT callbacks.
  _client.onDisconnect([this](AsyncMqttClientDisconnectReason reason) { onMqttDisconnect(reason);});
  _client.onSubscribe([this](uint16_t packetId, uint8_t qos) { onMqttSubscribe(packetId, qos);});
  _client.onUnsubscribe([this](uint16_t packetId) { onMqttUnsubscribe(packetId);});
  _client.onMessage([this](char* topic,
                           char* payload,
                           AsyncMqttClientMessageProperties properties,
                           size_t len,
                           size_t index,
                           size_t total) {
      onMqttMessage(topic, payload, properties, len, index, total);
  });
  _client.onPublish([this](uint16_t packetId) {
    onMqttPublish(packetId);
  });
  configureLastWill();                                                      // Configure the Last Will and Testament.
  logInfo(FN, LN, "MQTT subsystem initialized.");
  return true;
}

/*-----  APPLY THE CINFIGURATION DATA TO THE AsyncMqttClient LIBRARY  -----*/

void EiMqtt::applyConfiguration() {
    _client.setServer(_config.host.c_str(), _config.port);
    _client.setCredentials(_config.username.c_str(),
                           _config.password.c_str());
    _client.setKeepAlive(10);
}

/*-----  STARTUP THE MQTT SERVUCE  -----*/

void EiMqtt::connect() {
  logInfo(FN, LN,
      "Applying MQTT configuration: " +
      _config.host + ":" + String(_config.port));
  _state.connected = false;
  logInfo(FN, LN, "Connecting to MQTT broker " + _config.host + ":" + String(_config.port));
  _client.connect();
}

/*-----  CONFIGURE THE LAST WILL AND TESTMENT  -----*/

void EiMqtt::configureLastWill() {
  if (!appMqttLwtPolicy.enabled) {
    logInfo(FN, LN, "Last Will & Testament disabled.");
    return;
  }
  _client.setWill(
                  appMqttLwtPolicy.topic.c_str(),
                  appMqttLwtPolicy.qos,
                  appMqttLwtPolicy.retain,
                  appMqttLwtPolicy.offlineMsg.c_str()
  );
  logInfo(FN, LN, "Registered Last Will & Testament protect lane on topic: " + appMqttLwtPolicy.topic);
}

/*-----  CONFIGURE THE HEARTBEAT  -----*/

void EiMqtt::sendHeartbeat() {
  static RunTime hbTimer = {IntervalType::IT_MINUTE, 1, -1};
  if (!mqttHbPolicy.enabled)                                  // if heartbeats are not desired
    return;                                                   // just return
  static uint32_t lastHeartbeat = 0;
  if(!scheduler.isTimeToRun(hbTimer)) return;                 // if it is not time to send a heartbeat then just return
  mqttPubMsg(mqttHbPolicy.topic, 0, false, "ping", LN);       // send a heartbeat to node red
}

/*-----  _config DATA TO A JSON DOC  -----*/

void EiMqtt::configToJson(JsonDocument& doc) const
{
    doc["host"]   = _config.host;
    doc["port"]     = _config.port;
    doc["username"] = _config.username;
    doc["password"] = _config.password;
}

/*-----  JSON DOC TO _config DATA  -----*/

bool EiMqtt::jsonToConfig(const JsonDocument& doc)
{
    _config.host   = doc["host"]   | "";
    _config.port     = doc["port"]     | 1883;
    _config.username = doc["username"] | "";
    _config.password = doc["password"] | "";

    return true;
}

/*-----  JSON DOC TO _config DATA  -----*/

bool EiMqtt::readCfgFromDisk() {
  JsonDocument doc;
  if (!storage.readJsonFile(_configFileName.c_str(), doc, LN)) {
    logError(FN, LN, "Unable to read MQTT configuration.");
    return false;
  }
  return jsonToConfig(doc);
}

/*-----  WRITE THE MQTT CONFIG DATA TO DISK  -----*/

bool EiMqtt::writeCfgToDisk() {
  JsonDocument doc;

  doc["host"]     = _config.host;
  doc["port"]     = _config.port;
  doc["username"] = _config.username;
  doc["password"] = _config.password;

  if (storage.writeJsonFile(_configFileName.c_str(), doc, LN) != Storage::WriteResult::Success) {
    logError(FN, LN, "Unable to write MQTT configuration.");
    return false;
  }
  _config.dirty = false;
  logInfo(FN, LN, "MQTT configuration written to disk.");
  return true;
}

/*-----  PUBLISH A MQTT MSG TO NODE RED  -----*/

bool EiMqtt::mqttPubMsg(const String& topic, uint8_t qos, boolean retain, const char* message, int from) {
  if(topic.isEmpty()) return false;
  return _client.publish(topic.c_str(), qos, retain, message);
}

// The helper function that catches standard Arduino Strings and handles them safely
bool EiMqtt::mqttPubMsg(const String& topic, uint8_t qos, boolean retain, const String& message, int from) {
  return mqttPubMsg(topic, qos, retain, message.c_str(), from);
}

/*-----  ACTIONS TO TAKE ON CONNECTING TO THE MQTT SERVER  -----*/

void EiMqtt::onMqttConnect(bool sessionPresent) {
  _state.connected = true;
  logInfo(FN, LN, "Received MQTT connection notice. Session " +
          String(sessionPresent ? "is" : "is not") + " present.");
  
  
//  publishOnlineStatus();
  
//  scheduleReconnectComplete();
  
//  eiEvents.dispatchMqttConnected(sessionPresent);
  
}

/*---------------  ON MQTT CONNECT  ---------------*/

void onMqttConnect(bool sessionPresent) {
  
}

/*---------------  ON MQTT DISCONNECT  ---------------*/

void EiMqtt::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    _state.connected = false;

    logInfo(FN, LN,
               "MQTT disconnected. Reason: " +
               disconnectReasonToString(reason));
}

/*---------------  ON MQTT SUBSCRIBE  ---------------*/

void EiMqtt::onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  static int lastPacketID = 0;                                                  // MQTT starts packet IDs at 1
  static time_t lastMs = 0;
  if(lastPacketID == packetId && millis() - lastMs < 50) return;                // don't handle duplicates
  logInfo(FN, LN, "Subscribe acknowledged.  packetId: " +
          String(int(packetId)) + ", qos: " + String(qos));
  lastMs = millis();
  lastPacketID = int(packetId);
}

/*---------------  ON MQTT UNSUBSCRIBE  ---------------*/

void EiMqtt::onMqttUnsubscribe(uint16_t packetId) {
  logInfo(FN, LN, "Unsubscribe acknowledged.  packetId: " + String(packetId));
}

/*---------------  ON MQTT MESSAGE  ---------------*/

void EiMqtt::onMqttMessage(char* topic,
                           char* payload,
                           AsyncMqttClientMessageProperties properties,
                           size_t len,
                           size_t index,
                           size_t total)
{
  
}

/*---------------  ON MQTT PUBLISH  ---------------*/

void EiMqtt::onMqttPublish(uint16_t packetId) {
  if(false) {                                                                 // debug
    logInfo(FN, LN, "Publish acknowledged, packetId: " + String(packetId));   // log the action
  }
}

/*-----  ALLOW THE EXTERNAL CONFIGURATION OF THE MqttConfig STRUCT DATA  -----*/

bool EiMqtt::configure(const MqttConfig& cfg) {
  if (cfg.host.isEmpty()) {
    logError(FN, LN, "MQTT host may not be empty.");
    return false;
  }
  if (cfg.port == 0) {
    logError(FN, LN, "Invalid MQTT port.");
    return false;
  }
  _config = cfg;
  _config.dirty = true;
  if (!writeCfgToDisk())
      return false;
  applyConfiguration();
  return true;
}

/*-----  DUMP THE CONTENTS OF _config TO THE LOGS  -----*/

void EiMqtt::dumpConfiguration() const {
  JsonDocument doc;
  configToJson(doc);
  String json;
  serializeJsonPretty(doc, json);
  logInfo(FN, LN, json);
}

/*-----  PUT THE DISCONNECT REASON INTO A STRING  -----*/

String EiMqtt::disconnectReasonToString(AsyncMqttClientDisconnectReason reason) const {
  switch (reason) {
  case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
    return "TCP disconnected";
  case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:
    return "Broker unavailable";
  // ...
  default:
    return "Unknown";
  }
  
}/*-----  LOG INFO LOG MSG HELPER FUNCTION  -----*/

void EiMqtt::logInfo(const char* function, int lineNum, const String& message) const {
  logging.msg(__FILE__,
              function,
              lineNum,
              T::EVENT,
              L::INFO,
              ET::MQTT,
              message);
  }

/*-----  LOG ERROR LOG MSG HELPER FUNCTION  -----*/

void EiMqtt::logError(const char* function, int lineNum, const String& message) const {
  logging.msg(__FILE__,
              function,
              lineNum,
              T::EVENT,
              L::ERROR,
              ET::MQTT,
              message);
}

