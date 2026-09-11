
#include <Arduino.h>
#include <UUID.h>
#include <esp_system.h>

#include <ei_appFramework.h>

#include <ei_logging.h>
#include <ei_mqtt.h>
#include <ei_network.h>
#include <ei_storage.h>
#include <ei_system.h>
#include <ei_time.h>

AppFramework appFramework;

AppDirPolicy appDirs;

AppFname appFnames {
    appDirs.dataDir + "/bootTime.json",
    appDirs.dataDir + "/deviceId.json"
};

AppIDs appIDs;
MqttLwtPolicy appMqttLwtPolicy;
MqttHeartbeatPolicy mqttHbPolicy;

AppLibraryConfig appLibraryConfig;

bool AppFramework::startup() {
  if(!getDeviceId()) return false;
  return true;
}

/*-----  GET OR CREATE THE DEVICE ID  -----*/

bool AppFramework::getDeviceId() {
    JsonDocument doc;
  if (storage.readJsonFile(appFnames.deviceId.c_str(), doc, __LINE__)) {                            // Try to retrieve an existing device ID.
    const char* id = doc["deviceId"];
    if (id && strlen(id) > 0) {
      appIDs.deviceId = String(id);
      logInfo( LS, "DEVICE_ID", "Using existing device ID: " + appIDs.deviceId);
      return true;
    }
  }
  UUID uuid;                                                                                        // No usable device ID exists so Create one.
  appIDs.deviceId = uuid.toCharArray();
  JsonDocument newDoc;
  newDoc["deviceId"] = appIDs.deviceId;
  Storage::WriteResult result = storage.writeJsonFile(appFnames.deviceId.c_str(), newDoc, __LINE__);
  if (result != Storage::WriteResult::Success) {
    logError(LS, "DEVICE_ID", "Failed to save device ID");
    appIDs.deviceId = "";
    return false;
  }
  logInfo(LS, "DEVICE_ID", "Created device ID: " + appIDs.deviceId);
  return true;
}

/*-----  PROCESS AN INCOMING MESSAGE  -----*/

void AppFramework::processMsg(const JsonDocument& doc) {
  String route = doc["route"].as<String>();
  String command = doc["command"].as<String>();
  
  if (route == "appFramework/setup" && command == "SETUP") {
    serializeJson(doc, Serial);
    Serial.println();

    initSetupPage(doc);
    return;
  }
  if (route == "appFramework/global/cfg/update" && command == "PUT") {
    serializeJson(doc, Serial);
    Serial.println();

    JsonDocument networkDoc;
    networkDoc["data"] = doc["data"]["network"];
    network.configureFromJson(networkDoc);
    
    JsonDocument mqttDoc;
    mqttDoc["data"] = doc["data"]["mqtt"];
    mqtt.configureFromJson(mqttDoc);
    
    JsonDocument timeDoc;
    timeDoc["data"] = doc["data"]["time"];
    eiTime.configureFromJson(timeDoc);

    return;
  }

  logError(LS, ET::MQTT, "AppFramework did not handle route '" + route + "' with command '" + command + "'.");
}


void AppFramework::initSetupPage(const JsonDocument& doc) {
  logInfo(LS, ET::WEB, "Processing Web SETUP request.");

  JsonDocument response;

  response["receiver"] = "web";
  response["owner"] = "library";
  response["route"] = "appFramework/setup";
  response["command"] = "SETUP";

  JsonObject data = response["data"].to<JsonObject>();

  // Page information
  data["pageTitle"] = String(appIDs.pageTitle) + " Setup";
  data["pageHeader"] = String(appIDs.pageHeader) + " Setup";

  // WiFi
  JsonDocument wifiMsg = network.getWifiConfigMsg();
  JsonObject wifiData = data["wifi"].to<JsonObject>();
  wifiData["ssid"] = wifiMsg["data"]["ssid"];
  wifiData["password"] = wifiMsg["data"]["password"];
  // MQTT
  JsonDocument mqttMsg;
  mqtt.configToJson(mqttMsg);
  JsonObject mqttData = data["mqtt"].to<JsonObject>();
  mqttData["host"] = mqttMsg["host"];
  mqttData["port"] = mqttMsg["port"];
  mqttData["brokerUser"] = mqttMsg["brokerUser"];
  mqttData["brokerPwd"] = mqttMsg["brokerPwd"];
  // File destinations
  JsonArray fileDestinations = data["fileDestinations"].to<JsonArray>();
  storage.buildDirectoryList(fileDestinations, "/");
  
  serializeJson(response, Serial);
  Serial.println();

  eiSystem.routeOutboundMsg(response);
  TRACE();
}

/*-----    WRAP MQTT MSG WITH THE REQUIRED FORMTTING   -----*/

String AppFramework::buildJsonAppMqttMsg(const String& route, const String& command, const JsonObjectConst& data) {
  JsonDocument doc;

  doc["owner"] = "application";
  doc["route"] = route;
  doc["command"] = command;

  JsonObject outData = doc["data"].to<JsonObject>();
  outData.set(data);

  String json;
  serializeJson(doc, json);

  return json;
}

