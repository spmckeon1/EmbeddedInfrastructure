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
#include <ei_utilities.h>

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
  web.startup();
}

/*-----    SETUP THE WEB SYSTEM   -----*/

bool Web::setup() {
  logInfo(LS, ET::WEB, "Web setup() is running.");
  if (!startWebSocket())
    return false;
  ElegantOTA.begin(&_server);
//  ElegantOTA.setTitle(appIDs.appName.c_str());
  eiEvents.on(EiEvent::WifiConnected, onWifiConnected);
  return true;
}

/*-----    DO THE WEB STARTUP ACTIONS   -----*/

bool Web::startup() {
  logInfo(LS, ET::WEB, "Web startup() is running.");
  if(!startWebServer()) return false;
  _maxWebClients = appLibraryConfig.maxWebClients;
  if (_maxWebClients == 0) {
    logError(LS, ET::WEB, "Maximum Web clients is zero.");
    return false;
  }
  DUMP(appLibraryConfig.maxWebClients);
  _clients = new WebClient[_maxWebClients];
  DUMP(_maxWebClients);
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

/*-----    HANDLE THE WEB EVENT   -----*/

void Web::onWsEvent(AsyncWebSocket* server,
                    AsyncWebSocketClient* client,
                    AwsEventType type,
                    void* arg,
                    uint8_t* data,
                    size_t len)
{
  switch (type) {
    case WS_EVT_CONNECT: {
      TRACE();
      uint32_t clientId = client->id();

      registerClient(client);

      JsonDocument doc;

      doc["owner"] = "library";
      doc["route"] = "web";
      doc["command"] = "CLIENT_ID";
      doc["data"]["clientId"] = clientId;

      String msg;
      serializeJson(doc, msg);

      sendWS_msg(msg, client);

      logInfo(LS, ET::WEB,
              "Web client connected. Client ID: " + String(clientId));

      break;
    }
    case WS_EVT_DATA: {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->opcode == WS_TEXT) {
        JsonDocument doc;
        if (!validateTxtMsg(data, len, doc))
            break;
        eiSystem.processExternalMsg(doc);
      }
      else if (info->opcode == WS_BINARY) {
        logInfo(LS, ET::WEB, "Received WebSocket binary data: " + String(len) + " bytes.");
        storage.processBinary(data, len);
      }
      break;
    }
    case WS_EVT_DISCONNECT: {
      TRACE();
      WebClient* wc = findClient(client);
      if (wc != nullptr) {
        logInfo(LS, ET::WEB, "Web client disconnected. Client ID: " + String(wc->clientId));
        clearClient(wc);
      }
      break;
    }
    default:
      break;
    }
}

/*--------------- VALIDATE AN INCOMING MESSAGE ---------------*/

bool Web::validateTxtMsg(uint8_t* data, size_t len, JsonDocument& doc) {
  String json((char*)data, len);
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    logError(LS, ET::WEB, "Received invalid WebSocket JSON: " + String(error.c_str()) + ". Message: " + json);
    return false;
  }
  if (!verifyRecMsg(doc, json, ET::WEB))
    return false;
  doc["source"] = Text::sourceToStr(_source);
  return true;
}

/*--------------- VERIFY AN INCOMING MESSAGE HAS THE REQUIRED ELEMENTS ---------------*/

bool Web::verifyRecMsg(const JsonDocument& doc, const String& json, const char* eventType) {
  if (!doc[ET::WEB, "owner"].is<const char*>()) {
    Json::missingField(eventType, "owner", json);
    return false;
  }
  if (!doc["route"].is<const char*>()) {
    Json::missingField(ET::WEB, "route", json);
    return false;
  }
  if (!doc["command"].is<const char*>()) {
    Json::missingField(ET::WEB, "command", json);
    return false;
  }
  if (doc["data"].isNull()) {
    Json::missingField(ET::WEB, "data", json);
    return false;
  }
  return true;
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

/*---------------  START THE WEB SERVER  ---------------*/

bool Web::startWebServer() {
  _server.on("/", [this](AsyncWebServerRequest *request) {
  const String mainHTMLFile = "/html/main.html";
  if (storage.exists(mainHTMLFile)) {
    logInfo(LS, ET::WEB, "Dispatching main HTML file '" + mainHTMLFile + "'");
    request->send(storage.getFS(), mainHTMLFile, "text/html");
  }
  else {
    logError(LS, ET::WEB, "Main HTML file '" + mainHTMLFile + "' does not exist");
    request->send(404, "text/plain", "Main HTML file not found");
  }

  });
  
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
  for (uint8_t i = 0; i < _maxWebClients; i++) {
    if (_clients[i].client == client)
      return &_clients[i];
  }
  return nullptr;
}

/*-----  RETURN THE USED CLIENT STRUCT TO ITS ORIGINAL EMPTY STATE  -----*/

void Web::clearClient(WebClient* wc) {
  TRACE();
  if (wc == nullptr)
    return;
  wc->client        = nullptr;
  wc->clientId      = 0;
  wc->pgName        = "";
  wc->ip            = IPAddress();
  wc->connectedAt   = 0;
  wc->lastActivity  = 0;
}

/*-----  PROCESS AN INBOUND MSG  -----*/

bool Web::processMsg(const JsonDocument& doc) {
  const char* command = doc["command"] | "";

  if (strcmp(command, "CONNECT") == 0) {
    processConnect(doc);
    return true;
  }

  return false;
}

/*-----  REGISTER A NEW WEB CLIENT  -----*/

void Web::registerClient(AsyncWebSocketClient* client) {
  DUMP(_maxWebClients);
  for (uint8_t i = 0; i < _maxWebClients; i++) {
    if (_clients[i].client == nullptr) {
      DUMP(i);
      _clients[i].client = client;
      _clients[i].clientId = client->id();
      _clients[i].ip = client->remoteIP();
      _clients[i].connectedAt = millis();
      _clients[i].lastActivity = millis();

      logInfo(LS, ET::WEB, "Web client connected. Client ID: " + String(_clients[i].clientId));

      return;
    }
  }

  logError(LS, ET::WEB, "Web client table full. Client ID " + String(client->id()) + " rejected.");

  client->close();
}

/*-----  ***  -----*/

void Web::processConnect(const JsonDocument& doc) {
  uint32_t clientId = doc["data"]["clientId"] | 0;
  const char* page = doc["data"]["page"] | "";
  for (uint8_t i = 0; i < _maxWebClients; i++) {
    if (_clients[i].clientId == clientId) {
      _clients[i].pgName = page;
      logInfo(LS, ET::WEB, "Web client " + String(clientId) + " registered page '" + String(page) + "'");
      return;
    }
  }
  logError(LS, ET::WEB, "CONNECT received for unknown Web client ID " + String(clientId));
}

/*-----  PUBIC: IS PAGE CONNECTED  -----*/

bool Web::isPageConnected(const String& page) {
    for (uint8_t i = 0; i < _maxWebClients; i++) {
        if (_clients[i].client != nullptr &&
            _clients[i].pgName == page) {
            return true;
        }
    }
    return false;
}
