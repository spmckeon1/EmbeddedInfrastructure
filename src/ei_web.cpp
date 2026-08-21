//
//  web.c
//  
//
//  Created by Stephen McKeon on 7/21/26.
//

// EmbeddedInfrastructure intentionally supports one active file upload.
// Reject additional uploads until the current upload completes.

#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1

#include <ArduinoTrace.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include "ei_web.h"

#include <ei_logging.h>
#include <ei_mqtt.h>
#include <ei_network.h>
#include <ei_types.h>
#include <ei_network.h>

AsyncWebServer server(80);
//AsyncWebSocket ws("/ws");

Web web;

static void onWsEvent(
    AsyncWebSocket* server,
    AsyncWebSocketClient* client,
    AwsEventType type,
    void* arg,
    uint8_t* data,
    size_t len)
{
    web.onWsEvent(server, client, type, arg, data, len);
}

/*-----    WHEN WIFI STARTS   -----*/

static void onWifiConnected() {
  TRACE();
  web.startup();
}

/*-----    SETUP THE WEB SYSTEM   -----*/

bool Web::setup() {
  if (!startWebSocket())
    return false;
  ElegantOTA.begin(&_server);
  eiEvents.on(EiEvent::WifiConnected, onWifiConnected);
  return true;
}

/*-----    DO THE WEB STARTUP ACTIONS   -----*/

bool Web::startup() {
  if(!startWebServer()) return false;
  return true;
}

/*-----    DO THE Web EVENT LOOP   -----*/

void Web::evtLoop() {
  ElegantOTA.loop();
    // Nothing required at the moment.
    // Future:
    //   - websocket housekeeping
    //   - upload timeouts
    //   - client cleanup
}

/*-----  PROCESS A INCOMING WEB TEXT MESSAGE  -----*/

void Web::processWsMessage(uint8_t* data, size_t len) {
    String s((char*)data, len);
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, s);
    if (error) {
        logError(LS, ET::WEB, "Invalid WebSocket JSON: " + String(error.c_str()));
        return;
    }
    if (!doc["owner"].is<const char*>()) {
        logError(LS, ET::WEB, "WebSocket message missing required field 'owner'.");
        return;
    }
    if (!doc["route"].is<const char*>()) {
        logError( LS, ET::WEB, "WebSocket message missing required field 'route'.");
        return;
    }
    if (!doc["command"].is<const char*>()) {
        logError(LS, ET::WEB, "WebSocket message missing required field 'command'.");
        return;
    }
    if (!doc["data"].is<JsonObject>()) {
        logError(LS, ET::WEB,"WebSocket message missing required field 'data'.");
        return;
    }
    eiSystem.processExternalMsg(doc, Source::WEB);
}

/*-----  PROCESS A INCOMING WEB BINARY MESSAGE  -----*/

void Web::processWsBinary(uint8_t* data, size_t len) {
  logInfo(LS, ET::WEB, "Received WebSocket binary data: " + String(len) + " bytes.");
  storage.processBinary(data, len);
}

/*-----    HANDLE THE WEB EVENT   -----*/

void Web::onWsEvent(AsyncWebSocket* server,
                      AsyncWebSocketClient* client,
                      AwsEventType type,
                      void* arg,
                      uint8_t* data,
                      size_t len)
{
  switch (type) {
    case WS_EVT_DATA: {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->opcode == WS_TEXT) {
            processWsMessage(data, len);
        }
        else if (info->opcode == WS_BINARY) {
          logInfo(LS, ET::WEB, "Received WebSocket binary data.");
          processWsBinary(data, len);
        }
        break;
    }    case WS_EVT_CONNECT:
      // Optional logging
      break;

    case WS_EVT_DISCONNECT:
        // Optional logging
        break;

    default:
        break;
    }
}

/*-----    SEND A MESSAGE TO A WEB PAGE   -----*/

void Web::sendWS_msg(const String& message, AsyncWebSocketClient* client) {
  if (client != nullptr)
    client->text(message);
  else
    _ws.textAll(message);
}

/*-----    PUBLISH A WEB MESSAGE   -----*/

void Web::webPubMsg(const JsonDocument& doc) {
  String message;
  serializeJson(doc, message);
  sendWS_msg(message, nullptr);
}

/*-----    DISPATCH AN INCOMNG WIFI SETUP EVENT   -----*/

bool Web::hdlWiFiSetupEvent(String s, AsyncWebSocketClient* client) {
  logInfo(LS, ET::WEB,"Incoming string: " + s);
  if (handleConfigurationUpdate(s))      return true;
  if (handleIncomingFile(s))             return true;
  if (handleDownloadLocation(s, client)) return true;
  if (handleFileSizeRequest(s, client))  return true;
}

/*-----    HANDLE THE WIFI SETUP CONFIGURATION EVENT   -----*/

bool Web::handleConfigurationUpdate(String s) {
  if (s.indexOf("cfgDataJSON:") == -1) return false;
  int jsonStartPos = s.indexOf("cfgDataJSON:") + 12;
  String jsonPayload = s.substring(jsonStartPos);
  logInfo(LS, ET::WEB, "Isolated JSON Text Block: '" + jsonPayload);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonPayload);
  if (error) {
    logError(LS, ET::WEB, "ERROR: Web form JSON parsing failed! Reason: " + String(error.c_str()));
    return true;
  }
  if (!network.configureFromJson(doc)) return true;
  if (!mqtt.configureFromJson(doc)) return true;
  logInfo(LS, ET::WEB, "Web configuration successfully updated.");
  return true;
}

/*-----    HANDLE THE INCOMING FLE   -----*/

bool Web::handleIncomingFile(String s) {
  if (s.indexOf("incomingFile") == -1) return false;
  String ts = s.substring(0, 25);
  if (ts.indexOf("incomingFileComplete") != -1) {
    Serial.println("file download is complete.");
    logInfo(LS, ET::WEB, "Downloaded '" + _incomingFilePath +
            "' - '" +
            String(storage.getFileSize(_incomingFilePath.c_str())) +
            " bytes");
    _incomingFilePath = "";
  } else {
    _incomingFilePath = s.substring(s.indexOf(":") + 1,
                       s.indexOf("|"));
    storage.appendFile(
      _incomingFilePath.c_str(),
      s.substring(s.indexOf("|") + 1).c_str());
  }

  return true;
}

/*-----    HANDLE THE DOWNLOAD LOCATION EVENT   -----*/

bool Web::handleDownloadLocation(String s, AsyncWebSocketClient* client) {
  if (s.indexOf("downloadLocation:") == -1) return false;
  _downloadLocation = s.substring(s.lastIndexOf(":") + 1);
  logInfo(LS, ET::WEB, "Download location = '" + _downloadLocation + "'");
  sendWS_msg("DownLoadLocRec:" + _downloadLocation, client);
  return true;
}


bool Web::handleFileSizeRequest(String s, AsyncWebSocketClient* client) {
  if (s.indexOf("fileSizePlease:") == -1) return false;
  client->text("requestedFileSizeIs:" + storage.getFileSize(_incomingFilePath.c_str()));
  return true;
}


void Web::processSetupMsg(const JsonDocument& doc) {
  logInfo(LS, ET::WEB, "Processing Web SETUP request.");

  JsonDocument response;

  response["owner"] = "library";
  response["route"] = "web/setup";
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
  webPubMsg(response);
  TRACE();
}
/*-----    HANDLE A NEW WIFI SETUP WEB PAGE   -----*/

void Web::initNewWiFiPg(String s, AsyncWebSocketClient *client) {
  String outboundData = "";
  String CUID = s.substring(s.lastIndexOf(":") + 1);                                              // split the clients UID out of the string
  String ip = client->remoteIP().toString();                                                      // get its IP address
  logInfo(LS, ET::WEB, "A new WiFi setup web page has joined.  ClientId:'"+String(client->id())+           // log the new web page that joined
              "', Type :'WiFi setup', IP Address:'"+ ip + "', CUID:'" + CUID + "'");
  sendWS_msg("serverip:" + network.getIPAddress(), client);                                       // send the client the servers ip address
  sendWS_msg("clientip:" + ip, client);                                                           // send the client its ip address
  sendWS_msg(String("pgHeader:") + appIDs.pageHeader, client);                                    // send the page header
//  sendWS_msg("listDir:" + storage.readFile(allDirectoriesToFile.c_str(), true), client);
  gatherWiFiSetupData(outboundData);
  sendWS_msg("initWebDataJSON:" + outboundData, client);                                          // send the web page the info needed to setup the data fields
 }

/*---------------  GATHER WIFI SETUP DATA  ---------------*/

void Web::gatherWiFiSetupData(String& jsonOutput) {
    JsonDocument doc;

    const NetworkConfig& networkCfg = network.config();
    const MqttConfig& mqttCfg = mqtt.config();

    doc["networkSsid"] = networkCfg.ssid;
    doc["networkPass"] = networkCfg.password;

    doc["mqttServer"] = mqttCfg.host;
    doc["mqttPort"]   = mqttCfg.port;
    doc["mqttUser"]   = mqttCfg.brokerUser;
    doc["mqttPass"]   = mqttCfg.brokerPwd;

    serializeJson(doc, jsonOutput);
}

/*---------------  START THE WEB SERVER  ---------------*/

bool Web::startWebServer() {
  _server.on("/setup", AsyncWebRequestMethod::HTTP_GET,
      [this](AsyncWebServerRequest *request) {
          request->send_P(200, "text/html",webPgSetup);
      });
  _server.on("/post", AsyncWebRequestMethod::HTTP_POST,
      [](AsyncWebServerRequest *request) { },
      [this](AsyncWebServerRequest *request,
             const String& filename,
             size_t index,
             uint8_t *data,
             size_t len,
             bool final)
      {
          handlePostFile(request, filename, index, data, len, final);
      });
  _server.onNotFound(
      [this](AsyncWebServerRequest *request)
      {
          if (tryServeStaticFile(request))
              return;

          request->send(404, "text/plain", "404 - File Not Found");
      });
  
  logInfo(LS, ET::WEB, "Starting web server...");
  _server.begin();                                                // Start accepting HTTP requests.
  logInfo(LS, ET::WEB, "Web server started.");
  return true;
}

/*---------------  START THE WEB SOCKET  ---------------*/

bool Web::startWebSocket()
{
    logInfo(LS, ET::WEB, "Starting WebSocket...");

    _ws.onEvent(::onWsEvent);
    _server.addHandler(&_ws);

    logInfo(LS, ET::WEB, "WebSocket started.");

    return true;
}
/*---------------  PUBLIC: LETR A REQUESTER KNOW IF A DOWNLOAD IS OCCURING  ---------------*/

bool Web::downloadingFile() const
{
    return _downloadingFile;
}

/*---------------  HANDLE THE POST FILE DOWNLOAD ACTIONS  ---------------*/

void Web::handlePostFile(AsyncWebServerRequest* request,
                         const String& filename,
                         size_t index,
                         uint8_t* data,
                         size_t len,
                         bool final)
{
  static File file;
  if (index == 0) {
    if (_downloadingFile) {
      logWarn(LS, ET::WEB, "Rejecting upload. Another upload is already in progress.");
      request->send(409);
      return;
    }
    _downloadingFile = true;
    _incomingFilePath = _downloadLocation + "/" + filename;
    storage.deleteFile(_incomingFilePath.c_str(), LN);
    logInfo(LS, ET::WEB, "Downloading '" + filename + "' to '" + _incomingFilePath + "'.");
    file = storage.getFS().open(_incomingFilePath.c_str(), FILE_WRITE);
    if (!file) {
      _downloadingFile = false;
      request->send(500);
      return;
    }
  }
  if (!file.write(data, len)) {
    logError(LS, ET::WEB, "Failed writing '" + _incomingFilePath + "'.");
  }
  if (final) {
    file.close();
    logInfo(LS, ET::WEB, "Download complete. File size = " + String(storage.getFileSize(_incomingFilePath.c_str())));
    _incomingFilePath = "";
    _downloadingFile = false;
    request->send(200);
  }
}


/*---------------  IF THE WEB SERVER RECEIVES A REQUEST FOR A NON-DEFINED
                   ROUTE THE SEE IF STORAGE HAS A FILE THAT MATCHES IT  ---------------*/

bool Web::tryServeStaticFile(AsyncWebServerRequest *request) {
  String path = request->url();
  logInfo(LS, ET::WEB, "Static file request: '" + path + "' from " + request->client()->remoteIP().toString());
  if (!storage.exists(path)) return false;
  request->send(storage.getFS(), path, getContentType(path));
  return true;
}

const char *Web::getContentType(const String &path) const {
    if (path.endsWith(".html")) return "text/html";
    if (path.endsWith(".css"))  return "text/css";
    if (path.endsWith(".js"))   return "application/javascript";
    if (path.endsWith(".json")) return "application/json";
    if (path.endsWith(".txt"))  return "text/plain";
    if (path.endsWith(".png"))  return "image/png";
    if (path.endsWith(".jpg"))  return "image/jpeg";
    if (path.endsWith(".gif"))  return "image/gif";
    if (path.endsWith(".ico"))  return "image/x-icon";

    return "application/octet-stream";
}

/*-----  PROCESS AN INCOMING MSG  -----*/

void Web::processMsg(const JsonDocument& doc) {
    String route = doc["route"].as<String>();
    String command = doc["command"].as<String>();

    if (route == "web/setup" && command == "SETUP") {
        processSetupMsg(doc);
        return;
    }
}
/*-----  ADD THE NEW CLIENT TRACKING INFORMATION  -----*/

WebClient* Web::addClient(AsyncWebSocketClient* client) {
  WebClient* wc = findFreeClient();                       // Find an unused slot
  if (wc == nullptr)
    return nullptr;                                       // No room for another client
  wc->client        = client;
  wc->clientId      = client->id();
  wc->ip            = client->remoteIP();
  wc->connectedAt   = eiTime.now();
  wc->lastActivity  = wc->connectedAt;

    // pgName intentionally left blank until registration

    return wc;
}

/*-----  FIND THE FIRST EMPTY _clients STRUCT AND RETURN IT  -----*/

WebClient* Web::findFreeClient() {

    for (uint8_t i = 0; i < MAX_WEB_CLIENTS; i++) {
        if (_clients[i].client == nullptr)
            return &_clients[i];
    }

    return nullptr;
}

/*-----  FIND THE CLIENT DATA BY USING THE CLIENT ID AND RETURN ITS STRUCT  -----*/

WebClient* Web::findClient(AsyncWebSocketClient* client) {

    for (uint8_t i = 0; i < MAX_WEB_CLIENTS; i++) {
        if (_clients[i].client == client)
            return &_clients[i];
    }

    return nullptr;
}

/*-----  RETURN THE USED CLIENT STRUCT TO ITS ORIGINAL EMPTY STATE  -----*/

void Web::clearClient(WebClient* wc) {
  if (wc == nullptr)
    return;
  wc->client        = nullptr;
  wc->clientId      = 0;
  wc->pgName        = "";
  wc->ip            = IPAddress();
  wc->connectedAt   = 0;
  wc->lastActivity  = 0;
}

/*-----  PUT THE PAGE NAME INTO THE CLIENT RECORD  -----*/

bool Web::setClientPage(AsyncWebSocketClient* client, const String& pgName) {
    WebClient* wc = findClient(client);
    if (wc == nullptr)
        return false;
    wc->pgName = pgName;
    return true;
}

