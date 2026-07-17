/* TO DO
      scrub all occurence of 'MQTT_payloadToStr' out and replace with 'String Payload = String((char*)payload, length);'
 
*/
/*
 2024-08-19 - Changed gblCfgChg() to being called by this library in function onMqttMessage() instead of have the apps main file call it.
              All apps using this librrary will need to remove it from their code and just let this library take care of it.
 
 2025-01-25 - Depreciated the below line from onMqttSubscribe().  There does not appear to be a reason for it.
                sendWS_msg(FN, "Subscribe acknowledged.  packetId: " + String(int(packetId)) + ", qos: " + String(qos) + "\n", NULL);
 
 2025-06-06 - Removed the if(!booting) test from receive MQTT msg function.  The thought is that
                the app now has an ignore incoming topics function and this should take care of this issue
                this was originally intended to fix.  Initial testing,. on the RO system server, indicates this will not be a problem.

 */
#include <ArduinoTrace.h>
#include <ESPAsyncWebServer.h>
#include <Ticker.h>

#define USE_LITTLEFS

#include <commonItems_ESP32.h>
#include <mySd.h>
#include <myText.h>
#include <myMQTT.h>

const String mqttInfoFname = appDir + "/mqttConfig.json";                          // name of the MQTT information file
AsyncMqttClient mqttClient;
bool sendNrAliveMsgs = false;
bool connectedToMQTT = false;                                                   // at boot not connected yet
unsigned long lastNrAliveMsgSent = 0;                                           // milliseconds the last MQTT alive message was sent
unsigned long lastMqttAliveMsgRcv = 0;                                          // milliseconds the last MQTT alive message was received
String ctrlTestMsgTopic = "NOT TESTING";                                        // topic for test msgs unless main changes this
String nodeRedAliveTopic = "to/server/___/alive/msg";                           // default alive test msg from node red
unsigned long aliveMsgWaitTimeout = 120000;                                     // how long to wait before declaring a node red connect issue
bool rebootDueToLostMQTT_connection = false;                                    // reboot the ESP32 due to lost MQTT server connection
unsigned long lastErrorMsgLogged = 0;                                           // remember when the last error was logged
unsigned long lostMQTT_connectionTimeOut = 1800000;                             // delay from lost MQTT server connection to reboot of the ESP32
Ticker markMqttReconnectComplete;                                               // delays the listening to of MQTT msgs after mqtt server reconnect
String LWT_offline = "system/to/nr/status";                                  // used by the MQTT server to implement last will and testiment. .ino must customize this to its desired string
String mqttHbTop = MQTT_DEFAULT_HB;                                             // topic used to send heartbeat messages to mode red.  Daeaukted yp oDISABLED and waiting for the app to set
bool mqttHbActive = false;                                                      // save the state of doing mqtt heartbeats as false

/*---------------  LOG MQTT_hostPtr STRUCT  ---------------*/

void printHostDataStruct(MQTT_hostPtr hd) {
  mySP("MQTT struct contents:"
  "\n    portNo = " + String(hd->portNo) +
  "\n    host = " + hd->host.toString() +
  "\n    brokerUser = " + hd->brokerUser +
  "\n    brokerPwd = " + hd->brokerPwd + "\n", FN, LN);
}

/*---------------  MQTT CFG DATA TO STRING  ---------------*/

String mqttCfgDataToStr(MQTT_hostPtr h) {
  String s =  h->host.toString() + "|" +                                        // MQTT servers IP address
              String(h->portNo) + "|" +                                         // MQTT server's port number
              h->brokerUser + "|" +                                             // MQTT server broker user name
              h->brokerPwd;                                                     // MQTT server broker password
  return s;
}

/*---------------  WRITE MQTT CONNECT DATA TO DISK  ---------------*/

void writeMqttDataToDisk(MQTT_hostPtr h, String fn, int from) {
  // 1. Generate the clean JSON string dynamically by passing your live struct pointer 'h'!
  String jsonStr = "";
  mqttVarsToStr(jsonStr, h); // ✅ Fix: Added 'h' parameter here
  
  // 2. Pass the JSON text straight to your standardized write routine
  int n = myFileSys.writeFile(fn.c_str(), jsonStr.c_str(), LN);
  
  // Debug log your raw JSON layout to make sure it looks correct
  if(true) {
    mySP(jsonStr + "\n", FN, LN);
  }
  
  mySP("Wrote " + String(n) + " bytes to MQTT cfg file disk at location '" +
        fn + "'.\n", FN, LN);
}

/*--------------- OVERLOADED VERSION FOR SAVING LIVE DATA ---------------*/

void mqttVarsToStr(String &jsonOutput, MQTT_hostPtr h) {
  JsonDocument doc;
  
  // ✅ Dynamically extracts from your active struct pointer
  doc["mqttServer"] = h->host.toString();
  doc["mqttPort"]   = h->portNo;
  doc["mqttUser"]   = h->brokerUser;
  doc["mqttPass"]   = h->brokerPwd;

  char buffer[128];
  serializeJson(doc, buffer, sizeof(buffer));
  jsonOutput = buffer;
}

/*---------------  READ MQTT CONNECT DATA FROM DISK  ---------------*/

String readMqttDataFromDisk(String fn, MQTT_hostPtr h) {
  // 1. If the file is missing, log it and exit early
  if(!myFileSys.getFS().exists(fn.c_str())) {
    mySP("The file '" + fn + "' does not exist\n", FN, LN);
    return "The file '" + fn + "' does not exist\n";
  }

  // 2. Read the file into string 's' using your existing library function
  String s = myFileSys.readFile(fn.c_str(), true);
  
  if (s == "") {
    mySP("ERROR: File '" + fn + "' is empty.\n", FN, LN);
    return "ERROR: File was empty\n";
  }

  // 3. Parse the JSON text cleanly
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, s);

  if (error) {
    mySP("JSON Parsing failed for " + fn + ": " + String(error.c_str()) + "\n", FN, LN);
    return "JSON Parsing failed\n";
  }

  // 4. Modern Version 7 syntax (Zero warnings, fast fallback handling!)
  h->host.fromString(doc["mqttServer"] | h->host.toString());
  h->portNo     = doc["mqttPort"]   | h->portNo;
  h->brokerUser = doc["mqttUser"]   | h->brokerUser;
  h->brokerPwd  = doc["mqttPass"]   | h->brokerPwd;

  if(false) printHostDataStruct(h);

  // 5. Return the configuration string back to the system just like the old version did
  return mqttCfgDataToStr(h);
}

/*---------------  ON MQTT SETUP  ---------------*/

void mqttSetup(MQTT_hostPtr hd) {
  // 1. Run your lazy engine using your true global sketch pointer 'mqttHostPtr'!
  // ✅ Empty brackets '[]()' satisfy the raw 'String (*)()' function pointer contract perfectly!
  if (myFileSys.ensureFileExistsLazy("/mqttConfig.json", []() {
    String out;
    mqttVarsToStr(out, mqttHostPtr); // Uses your true global sketch pointer directly!
    return out;
  }, LN) == FILE_ALREADY_EXISTS)
  {
    readMqttDataFromDisk(mqttInfoFname, hd);
  }
  if(false) printHostDataStruct(hd);
  mySP("Setting up MQTT.  Host is:'" + hd->host.toString() +
       "', Port is: '" + String(hd->portNo) + "', Broker user is: '"
       + hd->brokerUser + "'\n", FN, LN);
  
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(hd->host, hd->portNo);
  mqttClient.setCredentials(hd->brokerUser.c_str(), hd->brokerPwd.c_str());
  mqttClient.setKeepAlive(10);                                                        // ⏱️ Optimizes offline detection for local hardware
  
  // ==========================================================
  // 📡 BULLETPROOF LAST WILL REGISTRATION
  // ==========================================================
  if (LWT_offline.length() > 0) {
    mqttClient.setWill(
                       LWT_offline.c_str(),
                       1,
                       true,
                       "Offline",
                       7
                       );
    mySP("Registered Last Will & Testament protect lane on topic: " +
         LWT_offline + "\n", FN, LN);
  }
  if(mqttHbTop != MQTT_DEFAULT_HB) mqttHbActive = true;                             // if the heart beat topic has been changed then set the testing to active
  mySP("MQT heartbeat topic set to: " + mqttHbTop + ".  Heartbeats are set to: " +  // log the setting
       (String(mqttHbActive) == MQTT_DEFAULT_HB?"DISABLED":"ACTIVE") + "\n", FN, LN);
}

/*---------------  START PROCESSING NODE RED MSGS AGAIN  ---------------*/

void procNR_msgsAgain() {
  booting = false;                                                                    // time to start processing node red msgs again
}

/*---------------  SET MQTT CONNECT DONE  ---------------*/

void setMQTT_ConnectDone() {
  mqttRunning = true;
  mySP(">> CONNECT DONE HANDSHAKE ACTIVE <<\n", FN, LN);
  logToMQTTServer = true;
  uint32_t payloadStart = millis();                             // Trace exactly how long this function takes to process!
  sendNodeRedAppData(LN);
  mySP("-> sendNodeRedAppData completed execution in: " +
       String(millis() - payloadStart) + " ms\n", FN, LN);
}

/*---------------  ON MQTT CONNECT  ---------------*/

void onMqttConnect(bool sessionPresent) {
  if (connectedToMQTT) {
    mySP("Ignored duplicate MQTT connect notice. State "
         "is already connected.\n", FN, LN);
    return;
  }
  connectedToMQTT = true;                                                     // lock down the connection state tokens immediately
  whereToSendLogEntryTo = MQTT_SERVER;
  dumpRamLogsToMqtt();                                                        // Instantly dump all offline historical traces accumulated
  mySP("Received MQTT connection notice. Session " +
       String(sessionPresent ? "is" : "is not") + " present.\n", FN, LN);
  markMqttReconnectComplete.once_ms(TIME_TO_WAIT_BEFORE_TAGGING_DONE, setMQTT_ConnectDone);
  runMqttHeartbeat();
//  mqttClient.publish(LWT_offline.c_str(), 1, true, "Online");
  mySP("Published 'Online' status packet to: " + LWT_offline + "\n", FN, LN);
  doAppMQTT_subscriptions();
  if(ctrlTestMsgTopic == "NOT TESTING") {
    sendNrAliveMsgs = false;
    mySP("sendNrAliveMsgs has been forced to: FALSE.\n", FN, LN);
  }
}

/*---------------  GET MQTT DISCONNECT REASON  ---------------*/

String getMQTTDISCReason(AsyncMqttClientDisconnectReason x) {
  switch(x) {
    case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
      return "TCP_DISCONNECTED";
      break;
    case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
      return "MQTT_UNACCEPTABLE_PROTOCOL_VERSION";
      break;
    case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:
      return "MQTT_IDENTIFIER_REJECTED";
      break;
    case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:
      return "MQTT_SERVER_UNAVAILABLE";
      break;
    case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:
      return "MQTT_MALFORMED_CREDENTIALS";
      break;
    case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:
      return "MQTT_NOT_AUTHORIZED";
      break;
    case AsyncMqttClientDisconnectReason::ESP8266_NOT_ENOUGH_SPACE:
      return "ESP8266_NOT_ENOUGH_SPACE";
      break;
    case AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT:
      return "TLS_BAD_FINGERPRINT";
      break;
    defaut: return "UNKNOWN REASON";
  }
  return "-1";
}

/*---------------  ON MQTT DISCONNECT  ---------------*/

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  connectedToMQTT = false;
  mqttRunning = false;
  logToMQTTServer = false;

  // Cast the reason to a clean, universal integer
  int errorCode = (int)reason;
  
  mySP("\n⚠️ [MQTT DISCONNECT] Reason Code: " + String(errorCode) + "\n", FN, LN);

  switch(errorCode) {
    case 0: mySP(" -> TCP_DISCONNECTED (Low-level routing or socket drop)\n", FN, LN); break;
    case 1: mySP(" -> UNACCEPTABLE_PROTOCOL_VERSION\n", FN, LN); break;
    case 2: mySP(" -> IDENTIFIER_REJECTED (Client ID issue or conflict)\n", FN, LN); break;
    case 3: mySP(" -> SERVER_UNAVAILABLE (Broker rejected port 1883)\n", FN, LN); break;
    case 4: mySP(" -> NOT_AUTHORIZED (Bad credentials)\n", FN, LN); break;
    default: mySP(" -> Internal library error code.\n", FN, LN); break;
  }
}

/*---------------  ON MQTT SUBSCRIBE  ---------------*/

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  static int lastPacketID = 0;                                                  // MQTT starts packet IDs at 1
  static time_t lastMs = 0;
  if(lastPacketID == packetId && millis() - lastMs < 50) return;                // don't handle duplicates
  
  mySP("Subscribe acknowledged.  packetId: " + String(int(packetId)) + ", qos: " + String(qos) + "\n", FN, LN);
  lastMs = millis();
  lastPacketID = int(packetId);
}

/*---------------  ON MQTT UNSUBSCRIBE  ---------------*/

void onMqttUnsubscribe(uint16_t packetId) {
  mySP("Unsubscribe acknowledged.  packetId: " + String(packetId) + "\n", FN, LN);
}

/*---------------  PRINT INCOMING MQTT MESSAGE  ---------------*/

void printIncomingMQTT_msg(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total, String fromFN, int fromLN) {
  int msgIDLen = len + 1;
  char msgID[len + 1];
  nullTermCharArray(payload, len, msgID, msgIDLen);
  Serial.println("*********************************Received MQTT message topic '" + String(topic) +
                 "'\n topic: '" + String(topic) +
                 "'\n payload: '" + String(payload, len) +
                 "'\n QoS is: '" + String(properties.qos) +
                 "'\n dup: '" + String(properties.dup) +
                 "\n len: '" + String(len) +
                 "'\n retain: '" + String(properties.retain) +
                 "'\n index: '" + String(index) +
                 "'\n total: '" + String(total) +
                 "\n FUNCTION: " + String(fromFN) +
                 "\n LINE #: " + String(fromLN)
  );
}

/*---------------  PAYLOAD TO STRING  ---------------*/

String MqttPayloadToStr(char* payload, int len) {
  int msgIDLen = len + 1;
  char msgID[len + 1];
  nullTermCharArray(payload, len, msgID, msgIDLen);
  return String(msgID);
}

/*---------------  ON MQTT MESSAGE  ---------------*/

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  static String lastTopic = "";
  static String lastPayload = "";
  static unsigned long lastMs = 0;
  
  unsigned long currentMs = millis();
  String currentTopic = String(topic);
  String currentPayload = MQTT_payloadToStr(payload, len);
  
  // Checks if this exact message is a rapid back-to-back echo
  if (lastMs > 0 && (currentMs - lastMs < 1000) &&      // Widen to 1000ms to safely catch broker lag
      currentTopic == lastTopic &&
      currentPayload == lastPayload) {
    return;                                             // It's a back-to-back duplicate echo! Kill it instantly.
  }
  lastTopic = currentTopic;
  lastPayload = currentPayload;
  lastMs = currentMs;

  // 2. ROUTE THE MESSAGE (Safe to return now because history is already locked in RAM)
  if (currentTopic == MQTT_INC_JSON_GCC) {              // if incoming global cfg JSON formated channge then handle it
    gblCfgChgJson(String(payload, len), mqttHostPtr);   // else if using the legy then the .ino file needs to handle it
    return;
  }
  else {
    doMqttAppCmds(topic, payload, properties, len, index, total, NODERED);
    return;
  }
}

/*---------------  ON MQTT PUBLISH  ---------------*/

void onMqttPublish(uint16_t packetId) {
  if(false) {                                                                   // debug
    mySP("Publish acknowledged, packetId: " + String(packetId) + "\n", FN, LN); // log the action
  }
}

/*---------------  CONNECT TO THE MQTT SERVER  ---------------*/

void connectToMqtt() {
  startMQTT = false;                                                            // start action has been completed, don't want it calleed multiple tiimes
  mySP("Attempting Connection to MQTT server...\n", FN, LN);
  if(false)printHostDataStruct(mqttHostPtr);
  mqttClient.connect();
}

/*---------------  MQTT PUBLISH MESSAGE  ---------------*/
/* Basically, use the publish() method on the mqttClient object to publish data on a topic. The publish() method accepts the following arguments, in order:
*
* MQTT topic (const char*)
* QoS (uint8_t): quality of service – it can be 0, 1 or 2
* retain flag (bool): retain flag
* payload (const char*) – in this case, the payload corresponds to the sensor reading
* The QoS (quality of service) is a way to guarantee that the message is delivered. It can be one of the following levels:

* 0: the message will be delivered once or not at all. The message is not acknowledged. There is no possibility of duplicated messages;
* 1: the message will be delivered at least once, but may be delivered more than once;
* 2: the message is always delivered exactly once;
* https://www.ibm.com/docs/en/ibm-mq/8.0?topic=concepts-qualities-service-provided-by-mqtt-client
*/

void mqttPublishMsg(const String& topic, int QoS, boolean retain, const char* message, int from) {
  if(topic.length() < 1) return;
  uint16_t packetIdPub1 = mqttClient.publish(topic.c_str(), QoS, retain, message);
}

// The helper function that catches standard Arduino Strings and handles them safely
void mqttPublishMsg(const String& topic, int QoS, boolean retain, const String& message, int from) {
  mqttPublishMsg(topic, QoS, retain, message.c_str(), from);
}

/*---------------    SEND MQTT HEARTBEAT   ---------------*/

void runMqttHeartbeat() {
  mqttPublishMsg(mqttHbTop, 0, false, "ping", LN);                      // send a heartbeat to node red
//  DUMP(mqttHbTop);
}
/*---------------  SEND TO MQTT SERVER AN ALIVE TEST MSG  ---------------*/

void sendMqttAliveMsg() {
  int minLastSent = -1;                                                         // minute the last msg was sent
  mqttPublishMsg(ctrlTestMsgTopic, 2, false, String(myTZ.now()), LN);                // publish a message back to the MQTT server with the current UTC unix time
  if(false && MINUTE.toInt() % 15 == 0 && MINUTE.toInt() != minLastSent) {       // if debugging and this is A 15 MINUTE INTERVAL and the msg has not already been sent
    mySP("Sent alive msg to node-red.  topic = " + ctrlTestMsgTopic +          // log the eevent
         ", payload = " + String(myTZ.now()) + ".\n", FN, LN);
    minLastSent = MINUTE.toInt();
  }
  if(false) mySP("Sent test msg to the MQTT system.\n", FN, LN);
}

/*---------------    RECEIVED NODE-RED ALIVE TEST MSG   ---------------*/

void receivedAliveNR_testMsg() {
  lastMqttAliveMsgRcv = millis();                                               // remember when the last alive msg was received
  if(lastErrorMsgLogged) {                                                      // if an error msg was written to the log
    mySP("Node-Red connectivity has been re-established.\n", FN, LN);           // log the recovery
    lastErrorMsgLogged = 0;                                                     // and set the trigger back to zero
  }
}

/*---------------    OVERDUE NODE-RED ALIVE MESSAGE   ---------------*/

void hdlOverdueNR_aliveMsg() {
  if(suli(millis(), lastErrorMsgLogged, LN, true) > 300000) {                                                           // if it has been long enough to repeat the error log message
    mySP("ERROR: The server has lost Node-Red connectivity.  Please attempt repair.\n", FN, LN);              // do so
    lastErrorMsgLogged = millis();                                                                            // and remember when this was done
  }
  if(suli(millis(),lastMqttAliveMsgRcv, LN, true) > lostMQTT_connectionTimeOut && !rebootAskedforAt) {                  // if the timer for the reboot to attempt to restore connectivity has been reached
    mySP("Rebooting server in an attempt to reestablish Node-Red connectivity.\n", FN, LN);                   // log the actiion
    String str = "Lost Node-Red connectivity for more than '" + msToDHMS(lostMQTT_connectionTimeOut) + "'.\n";// prepare the msg to be written to disk
    requestReboot(str, false);                                                                                // request the server reboot
  }
}

