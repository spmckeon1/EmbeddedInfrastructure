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
#include <ei_system.h>
#include <ei_mqtt.h>

EiMqtt mqtt;

/*-----  MQTT EVENT LOOP  -----*/

bool EiMqtt::evtLoop() {
  if (!_state.operational)                                      // if mqtt is not operational do nothing
      return false;
  
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
  logInfo(LS, ET::MQTT, "Starting MQTT subsystem...");
  Storage::EnsureFileResult result =                                        // Ensure the configuration file exists.
            storage.ensureFileExists(_configFileName.c_str(), doc, LN);
  switch (result) {
    case Storage::EnsureFileResult::Created:
      logInfo(LS, ET::MQTT, "Created default MQTT configuration file.");
      break;
    case Storage::EnsureFileResult::AlreadyExists:
      break;                                                                  // Nothing to do
    case Storage::EnsureFileResult::Error:
      logError(LS, ET::MQTT, "Unable to ensure MQTT configuration file exists.");
      return false;
  }
  if(!readCfgFromDisk()) {
    logError(LS, ET::MQTT, "Unable to read MQTT configuration.");
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
  logInfo(LS, ET::MQTT, "MQTT subsystem initialized.");
  return true;
}

/*-----  APPLY THE CINFIGURATION DATA TO THE AsyncMqttClient LIBRARY  -----*/

void EiMqtt::applyConfiguration() {
  _client.setServer(_config.host.c_str(), _config.port);
  _client.setCredentials(_config.brokerUser.c_str(),
                         _config.brokerPwd.c_str());
  _client.setKeepAlive(10);
}

/*-----  STARTUP THE MQTT SERVUCE  -----*/

void EiMqtt::connect() {
  logInfo(LS, ET::MQTT, "Applying MQTT configuration: " + _config.host + ":" + String(_config.port));
  _state.connected = false;
  logInfo(LS, ET::MQTT, "Connecting to MQTT broker " + _config.host + ":" + String(_config.port));
  _client.connect();
}

/*-----  CONFIGURE THE LAST WILL AND TESTMENT  -----*/

void EiMqtt::configureLastWill() {
  if (!appMqttLwtPolicy.enabled) {
    logInfo(LS, ET::MQTT, "Last Will & Testament disabled.");
    return;
  }
  _client.setWill(
                  appMqttLwtPolicy.topic.c_str(),
                  appMqttLwtPolicy.qos,
                  appMqttLwtPolicy.retain,
                  appMqttLwtPolicy.offlineMsg.c_str()
  );
  logInfo(LS, ET::MQTT, "Registered Last Will & Testament protect lane on topic: " + appMqttLwtPolicy.topic);
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
    doc["brokerUser"] = _config.brokerUser;
    doc["brokerPwd"] = _config.brokerPwd;
}

/*-----  JSON DOC TO _config DATA  -----*/

bool EiMqtt::jsonToConfig(const JsonDocument& doc)
{
    _config.host   = doc["host"]   | "";
    _config.port     = doc["port"]     | 1883;
    _config.brokerUser = doc["brokerUser"] | "";
    _config.brokerPwd = doc["brokerPwd"] | "";

    return true;
}

/*-----  JSON DOC TO _config DATA  -----*/

bool EiMqtt::readCfgFromDisk() {
  JsonDocument doc;
  if (!storage.readJsonFile(_configFileName.c_str(), doc, LN)) {
    logError(LS, ET::MQTT, "Unable to read MQTT configuration.");
    return false;
  }
  return jsonToConfig(doc);
}

/*-----  WRITE THE MQTT CONFIG DATA TO DISK  -----*/

bool EiMqtt::writeCfgToDisk() {
  JsonDocument doc;

  doc["host"]     = _config.host;
  doc["port"]     = _config.port;
  doc["brokerUser"] = _config.brokerUser;
  doc["brokerPwd"] = _config.brokerPwd;

  if (storage.writeJsonFile(_configFileName.c_str(), doc, LN) != Storage::WriteResult::Success) {
    logError(LS, ET::MQTT, "Unable to write MQTT configuration.");
    return false;
  }
  _config.dirty = false;
  logInfo(LS, ET::MQTT, "MQTT configuration written to disk.");
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
  eiEvents.notify(EiEvent::MqttConnected);
  logging.setDestination(LogDestination::MqttServer);
  logInfo(LS, ET::MQTT, "Received MQTT connection notice. Session " +
          String(sessionPresent ? "is" : "is not") + " present.");
  addSubscriptions();
}

/*---------------  ON MQTT DISCONNECT  ---------------*/

void EiMqtt::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  _state.connected = false;
  eiEvents.notify(EiEvent::MqttDisconnected);
  logging.setDestination(LogDestination::RamBuffer);
  logInfo(LS, ET::MQTT,
          "MQTT disconnected. Reason: " +
          disconnectReasonToString(reason));
}

/*---------------  ON MQTT SUBSCRIBE  ---------------*/

void EiMqtt::onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  static int lastPacketID = 0;                                                  // MQTT starts packet IDs at 1
  static time_t lastMs = 0;
  if(lastPacketID == packetId && millis() - lastMs < 50) return;                // don't handle duplicates
  logInfo(LS, ET::MQTT, "Subscribe acknowledged.  packetId: " +
          String(int(packetId)) + ", qos: " + String(qos));
  lastMs = millis();
  lastPacketID = int(packetId);
}

/*---------------  ON MQTT UNSUBSCRIBE  ---------------*/

void EiMqtt::onMqttUnsubscribe(uint16_t packetId) {
  logInfo(LS, ET::MQTT, "Unsubscribe acknowledged.  packetId: " + String(packetId));
}

/*---------------  PROCESS THE NEWLY ARRIVES MQTT MSG  ---------------*/

void EiMqtt::processInboundMsg(const JsonDocument& doc) {
  String owner = doc["owner"].as<String>();                     // Determine the owner of the message.
  if (owner == "library") {                                     // Route library-owned messages.
    eiSystem.handleMsg(doc);
    return;
  }
  if (owner == "application") {                                 // Route application-owned messages.
    appHandleMsg(doc);
    return;
  }
  logError(LS, ET::MQTT, "Received MQTT message with unknown owner '" + owner + "'.");
}
/*---------------  HELPER TO LOG ERRORS ON M=MQTT MSG RECEIPT  ---------------*/

void EiMqtt::missingField(const String& field, const String& json) {
    logError(LS, ET::MQTT,
             "MQTT message missing required field '" +
             field + "'. Received: " + json);
}

/*---------------  ON MQTT MESSAGE  ---------------*/

void EiMqtt::onMqttMessage(char* topic,
                           char* payload,
                           AsyncMqttClientMessageProperties properties,
                           size_t len,
                           size_t index,
                           size_t total)
{
  String json(payload, len);                                          // Build the incoming JSON string.
  JsonDocument doc;                                                   // Parse the JSON.
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
      logError(LS, ET::MQTT, "Received invalid JSON. Error: " + String(err.c_str()) + ". Message: " + json);
      return;
  }
  if (!doc["source"].is<const char*>()) {                               // Validate the required message fields.
    missingField("source", json);
    return;
  }
  if (!doc["scope"].is<const char*>()) {
    missingField("scope", json);
    return;
  }
  if (!doc["service"].is<const char*>()) {
    missingField("service", json);
    return;
  }
  if (!doc["command"].is<const char*>()) {
    missingField("command", json);
    return;
  }
  if (!doc["data"].is<JsonObject>()) {
    missingField("data", json);
    return;
  }
  processInboundMsg(doc);
}


/*---------------  ON MQTT PUBLISH  ---------------*/

void EiMqtt::onMqttPublish(uint16_t packetId) {
  if(false) {                                                                 // debug
    logInfo(LS, ET::MQTT, "Publish acknowledged, packetId: " + String(packetId));   // log the action
  }
}

/*-----  ALLOW THE EXTERNAL CONFIGURATION OF THE MqttConfig STRUCT DATA  -----*/

bool EiMqtt::configure(const MqttConfig& cfg) {
  if (cfg.host.isEmpty()) {                                // Validate the configuration before accepting it.
    logError(LS, ET::MQTT, "MQTT host may not be empty.");
    return false;
  }
  if (cfg.port == 0) {
    logError(LS, ET::MQTT, "Invalid MQTT port.");
    return false;
  }
  _config = cfg;
  _config.dirty = true;
  if(!writeCfgToDisk()) {
    logError(LS, ET::MQTT, "Unable to save MQTT configuration.");
    return false;
  }
  return true;
}
/*-----  ALLOW THE EXTERNAL AGENT TO SEE THE CONFIGURATION OF THE MqttConfig STRUCT DATA  -----*/

const MqttConfig& EiMqtt::config() const {
    return _config;
}

/*-----  DUMP THE CONTENTS OF _config TO THE LOGS  -----*/

void EiMqtt::dumpConfiguration() const {
  JsonDocument doc;
  configToJson(doc);
  String json;
  serializeJsonPretty(doc, json);
  logInfo(LS, ET::MQTT, json);
}

/*-----  PUT THE DISCONNECT REASON INTO A STRING  -----*/

String EiMqtt::disconnectReasonToString(AsyncMqttClientDisconnectReason reason) const {
  switch (reason) {
  case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
    return "TCP disconnected";
  case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:
    return "MQTT Host unavailable";
  // ...
  default:
    return "Unknown";
  }
  
}

/*-----  SET THE MAXIMUM NUMBER OF MQTT SUBSCRIPTIONS  -----*/

bool EiMqtt::setMaxSubCnt(uint16_t maxCnt) {
  if (_subscriptions != nullptr) {
    logError(LS, ET::MQTT, "Subscription table already allocated.");
    return false;
  }
  if (maxCnt == 0) {
    logError(LS, ET::MQTT, "Maximum subscription count must be greater than zero.");
    return false;
  }
  _subscriptions = new (std::nothrow) MqttSubscription[maxCnt];
  if (_subscriptions == nullptr) {
    logError(LS, ET::MQTT, "Unable to allocate MQTT subscription table. "
             "MQTT subsystem disabled.");
    _state.operational = false;
    return false;
  }
  _maxSubCnt = maxCnt;
  _subCnt    = 0;
  return true;
}

/*-----  ADD REQUESTED SUBSCRIPTIONS  -----*/

bool EiMqtt::addAppMQTTSubscriptions(const String& name, const String& topic, uint8_t qos) {
  if (_subscriptions == nullptr) {
    logError(LS, ET::MQTT, "Call setMaxSubCnt() before addSubscription().");
    return false;
  }
  if (_subCnt >= _maxSubCnt) {
    logError(LS, ET::MQTT, "Maximum subscription count exceeded.");
    return false;
  }
  _subscriptions[_subCnt].name  = name;
  _subscriptions[_subCnt].topic = topic;
  _subscriptions[_subCnt].qos   = qos;
  _subCnt++;
  return true;
}

/*-----  ADD THE APP DESIRED SUBSCRIPTIONS  -----*/

bool EiMqtt::addSubscriptions() {
  if (!_state.operational)
    return false;
  if (!_state.connected)
    return false;
  for (uint16_t i = 0; i < _subCnt; i++) {
    _client.subscribe(_subscriptions[i].topic.c_str(),
                      _subscriptions[i].qos);
  }
  return true;
}

/*-----  PUBLIC IS THE MQTT SERVICE CONNECTED  -----*/

bool EiMqtt::connected() const {
  return _state.connected;
}

void EiMqtt::dumpConfig() const
{
    Serial.println();
    Serial.println("========== MQTT Configuration ==========");
    Serial.printf("dirty    : %s\n", _config.dirty ? "true" : "false");
    Serial.printf("host     : '%s'\n", _config.host.c_str());
    Serial.printf("port     : %u\n", _config.port);
    Serial.printf("brokerUser : '%s'\n", _config.brokerUser.c_str());
    Serial.printf("brokerPwd : '%s'\n", _config.brokerPwd.c_str());
    Serial.println("========================================");
    Serial.println();
}

/*-----  PUBLIC: ALLOW EXTERNAL AGENT TO SEND A NEW MQTT CFG IN  -----*/

bool EiMqtt::configureFromJson(const JsonDocument& doc) {
    MqttConfig cfg = _config;
    if (doc["mqttServer"].is<String>())
      cfg.host = doc["mqttServer"].as<String>();
    if (doc["mqttPort"].is<int>())
        cfg.port = doc["mqttPort"].as<int>();
    if (doc["mqttUser"].is<String>())
        cfg.brokerUser = doc["mqttUser"].as<String>();
    if (doc["mqttPass"].is<String>())
        cfg.brokerPwd = doc["mqttPass"].as<String>();
    return configure(cfg);
}

/*-----  TRANSLATE BETWEEN OWNER STRING AND OWNER ENUM  -----*/

EiMqtt::Owner EiMqtt::ownerFromString(const char* s) {
    if (strcmp(s, "library") == 0)
        return Owner::Library;
    if (strcmp(s, "application") == 0)
        return Owner::Application;
    return Owner::Unknown;
}

/*-----  TRANSLATE BETWEEN OWNER ENUM AND OWNER STRING  -----*/

const char* EiMqtt::ownerToString(Owner owner) {
    switch (owner) {
    case Owner::Library:
        return "library";
    case Owner::Application:
        return "application";
    default:
        return "unknown";
    }
}

/*-----  PROCESS AN INCOMING MSG  -----*/

void EiMqtt::processMsg(const JsonDocument& doc) {
  
}
