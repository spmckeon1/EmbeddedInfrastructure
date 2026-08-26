
#include <Arduino.h>
#include <ei_appFramework.h>

#include <ei_logging.h>
#include <ei_mqtt.h>
#include <ei_network.h>
#include <ei_system.h>

AppFramework appFramework;

AppDirPolicy appDirs;
AppFnamePolicy appFnames {appDirs.dataDir + "/bootTime.json"};
AppIDs appIDs;
MqttLwtPolicy appMqttLwtPolicy;
MqttHeartbeatPolicy mqttHbPolicy;

void AppFramework::processMsg(const JsonDocument& doc) {
  String route = doc["route"].as<String>();
  String command = doc["command"].as<String>();
  
  if (route == "web/setup" && command == "SETUP") {
    initSetupPage(doc);
    return;
  }
}


void AppFramework::initSetupPage(const JsonDocument& doc) {
  logInfo(LS, ET::WEB, "Processing Web SETUP request.");

  JsonDocument response;

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
  eiSystem.routeOutboundMsg(response);
  TRACE();
}
