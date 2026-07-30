#pragma once

// -----------------------------------------------------------------------------
// EmbeddedInfrastructure
//
// Module:
//     MQTT
//
// Owns:
//
//     - MQTT client connection
//     - MQTT subscriptions
//     - MQTT publications
//     - Incoming message routing
//
// Not Responsible For:
//
//     - Network connectivity
//     - Device business logic
//     - Configuration ownership
//
// -----------------------------------------------------------------------------

#include <ArduinoJson.h>
#include <AsyncMqttClient.h>
#include "ei_events.h"

struct MqttConfig {
  String host;
  uint16_t port = 1883;
  String username;
  String password;
  bool dirty = false;
};

struct MqttState {
  bool operational = true;
  bool connected = false;
};

struct MqttStats {
    uint32_t messagesReceived = 0;
    uint32_t messagesSent     = 0;
};

struct MqttSubscription {
    String  name;
    String  topic;
    uint8_t qos = 0;
};

class EiMqtt {
public:
  bool evtLoop();
  bool startup();
  bool setup();
  bool configure(const MqttConfig& cfg);
  bool mqttPubMsg(const String& topic, uint8_t qos, boolean retain, const char* message, int from);
  bool mqttPubMsg(const String& topic, uint8_t qos, boolean retain, const String& message, int from);
  bool setMaxSubCnt(uint16_t maxCnt);
  bool addSubscription(const String& name, const String& topic, uint8_t qos);
  bool connected() const;
private:
  MqttConfig _config;
  MqttState  _state;
  MqttStats  _stats;
  AsyncMqttClient _client;
  String _configFileName = "";
  MqttSubscription* _subscriptions = nullptr;
  uint16_t _maxSubCnt = 0;
  uint16_t _subCnt    = 0;

  void applyConfiguration();
  void connect();
  void configureLastWill();
  void sendHeartbeat();
  void configToJson(JsonDocument& doc) const;
  bool jsonToConfig(const JsonDocument& doc);
  bool readCfgFromDisk();
  bool writeCfgToDisk();
  void onMqttConnect(bool sessionPresent);
  void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
  void onMqttSubscribe(uint16_t packetId, uint8_t qos);
  void onMqttUnsubscribe(uint16_t packetId);
  void dumpConfiguration() const;
  void onMqttMessage(char* topic,
                     char* payload,
                     AsyncMqttClientMessageProperties properties,
                     size_t len,
                     size_t index,
                     size_t total);
  void onMqttPublish(uint16_t packetId);
  String disconnectReasonToString(AsyncMqttClientDisconnectReason reason) const;
  void logInfo(const char* function,
               int lineNum,
               const String& message) const;
  
  void logError(const char* function,
                int lineNum,
                const String& message) const;
  bool addSubscriptions();
  void dumpConfig() const;
  
};

extern EiMqtt mqtt;

