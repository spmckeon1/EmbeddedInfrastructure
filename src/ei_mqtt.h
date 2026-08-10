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

enum QoS {
    QOS0 = 0,
    QOS1 = 1,
    QOS2 = 2
};

enum Retain {
    FORGET = 0,
    RETAIN = 1
};

struct MqttConfig {
  String host;
  uint16_t port = 1883;
  String brokerUser;
  String brokerPwd;
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

class Topic {
public:
  constexpr explicit Topic(const char* value)
      : _value(value) {}
  constexpr const char* c_str() const {
    return _value;
  }
  constexpr bool isEmpty() const {
    return (_value == nullptr) || (_value[0] == '\0');
  }
private:
  const char* _value;
};

class EiMqtt {
public:

  bool evtLoop();
  bool startup();
  bool setup();
  bool mqttPubMsg(const Topic& topic, QoS qos, Retain retain, const char* message, int from);
  bool mqttPubMsg(const Topic& topic, QoS qos, Retain retain, const String& message, int from);
  bool setMaxSubCnt(uint16_t maxCnt);
  bool addSubscription(const String& name, const String& topic, uint8_t qos);
  bool connected() const;
  bool configure(const MqttConfig& cfg);
  bool configureFromJson(const JsonDocument& doc);
  const MqttConfig& config() const;
  void processMsg(const JsonDocument& doc);
  bool addAppMQTTSubscriptions(const String& name, const String& topic, uint8_t qos);

private:
  enum class Owner {
    Unknown,
    Library,
    Application
  };

  MqttConfig _config;
  MqttState  _state;
  MqttStats  _stats;
  String _heartbeatPayload;
  static const Topic HEARTBEAT_TOPIC;
  AsyncMqttClient _client;
  String _configFileName = "";
  MqttSubscription* _subscriptions = nullptr;
  uint16_t _maxSubCnt = 0;
  uint16_t _subCnt    = 0;

  void buildHeartbeatPayload();
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
  void processInboundMsg(const JsonDocument& doc);
  void missingField(const String& field, const String& json);
  void onMqttMessage(char* topic,
                     char* payload,
                     AsyncMqttClientMessageProperties properties,
                     size_t len,
                     size_t index,
                     size_t total);
  void onMqttPublish(uint16_t packetId);
  String disconnectReasonToString(AsyncMqttClientDisconnectReason reason) const;
  bool addSubscriptions();
  void dumpConfig() const;
  Owner ownerFromString(const char* s);
  const char* ownerToString(Owner owner);

};

extern EiMqtt mqtt;

extern void appHandleMsg(const JsonDocument& doc);
