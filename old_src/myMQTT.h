#ifndef _MY_MQTT_H_                                                             // if _MQTT_H_ is not defined then (skips the file if it is defined)
#define _MY_MQTT_H_                                                             // define it

#include <AsyncMqttClient.h>
#include <Ticker.h>
#include <commonItems_ESP32.h>

#define CFG_ELEMENT_COUNT 4                                                     // number of elements in the MQTT configuration file
#define TIME_BTW_REMINDER_ALERTS 3600000                                        // milliseconds between repeat alert msgs being sent
#define MQTT_INCOMING_GCC "global/mqtt/cfg/chg/data"                            // mqtt incoming global configuration change msg
#define MQTT_INC_JSON_GCC "global/mqtt/json/cfg/chg/data"                       // Inbound global JSON configuration changes
#define TIME_TO_WAIT_BEFORE_TAGGING_DONE 50

#define AT_MOST_ONCE 0
#define AT_LEAST_ONCE 1
#define EXACTLY_ONCE 2
#define MQTT_DEFAULT_HB "DIASBLED"                                               // default content of mqttHB

extern const String mqttInfoFname;                                              // name of the MQTT information file
extern bool mqttHbActive;                                                       // save the state of doing mqtt heartbeats as false
extern String mqttHbTop;                                                        // topic for mqtt heartbeat msgs

extern AsyncMqttClient mqttClient;
extern bool sendNrAliveMsgs;
extern String ctrlTestMsgTopic;                                                 // topic for test msgs unless main changes this
extern String nodeRedAliveTopic;                                                // default alive test msg from node red
extern bool connectedToMQTT;                                                    // at boot not connected yet

extern Ticker mqttReconnectTimer;                                               // declaring the MQTT reconnect timer
extern Ticker mqttAliveTestMsg;
extern unsigned long lastNrAliveMsgSent;                                        // milliseconds the last MQTT alive message was sent
extern unsigned long lastMqttAliveMsgRcv;                                       // milliseconds the last MQTT alive message was received
extern unsigned long lostMQTT_connectionTimeOut;                                // delay from lost MQTT server connection to reboot of the ESP32
extern unsigned long aliveMsgWaitTimeout;                                       // how long to wait before declaring a node red connect issue

extern bool wBrtToDisk;                                                         // is a disk write for a changed brightness queued
extern unsigned long wBrtToDiskStartTime;                                       // time the disk brightness change was made
extern LogInData logInD1;                                                       // must be declared by the main app
extern bool booting;                                                            // main app file sets this to false when the server has finished booting
extern String LWT_offline;


// THE FOLLOWING MUST BE DEFINED IN THE MAIN C++ FILE
extern void sendNodeRedAppData(int calledBy);
extern void doAppMQTT_subscriptions();
extern void doMqttAppCmds(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total, source who);
extern const_dataPtr constDataPtr;                                              // constants data struct pointer from main

// nyMQTT.cpp forward declarations
extern void printHostDataStruct(MQTT_hostPtr hd);
extern String mqttCfgDataToStr(MQTT_hostPtr h);
extern void writeMqttDataToDisk(MQTT_hostPtr h, String fn, int from);
extern void mqttVarsToStr(String &jsonOutput, MQTT_hostPtr h);
extern String readMqttDataFromDisk(String fn, MQTT_hostPtr h);
extern void mqttSetup(MQTT_hostPtr hostData);
extern void procNR_msgsAgain();
extern void setMQTT_ConnectDone();
extern void onMqttConnect(bool sessionPresent);
extern String getMQTTDISCReason(AsyncMqttClientDisconnectReason x);
extern void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
extern void onMqttSubscribe(uint16_t packetId, uint8_t qos);
extern void onMqttUnsubscribe(uint16_t packetId);
extern void printIncomingMQTT_msg(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total, String fromFN, int fromLN);
extern String MqttPayloadToStr(char* payload, int len);
extern void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
extern void onMqttPublish(uint16_t packetId);
extern void connectToMqtt();
extern void mqttPublishMsg(const String& topic, int QoS, boolean retain, const char* message, int from);
extern void mqttPublishMsg(const String& topic, int QoS, boolean retain, const String& message, int from);
extern void sendMqttAliveMsg();
extern void runMqttHeartbeat();
extern void receivedAliveNR_testMsg();
extern void hdlOverdueNR_aliveMsg();

 #endif /* _MY_MQTT_H_ */
