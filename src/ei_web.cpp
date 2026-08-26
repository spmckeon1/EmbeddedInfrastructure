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
  TRACE();
  web.startup();
}

/*-----    SETUP THE WEB SYSTEM   -----*/

bool Web::setup() {
  logInfo(LS, ET::WEB, "Web setup() is running.");
  if (!startWebSocket())
    return false;
  ElegantOTA.begin(&_server);
  eiEvents.on(EiEvent::WifiConnected, onWifiConnected);
  return true;
}

/*-----    DO THE WEB STARTUP ACTIONS   -----*/

bool Web::startup() {
  logInfo(LS, ET::WEB, "Web startup() is running.");
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
  case WS_EVT_DISCONNECT:
    // Optional logging
    break;
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

